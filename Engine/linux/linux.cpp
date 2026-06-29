#include "linux.hpp"
#include "coc.hpp"
#include "common/common.hpp"
#include "stable/xdg-shell/xdg-shell.h"
#include "staging/fractional-scale/fractional-scale-v1.h"
#include "unstable/xdg-decoration/xdg-decoration-unstable-v1.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <unistd.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
namespace Engine {
// if false, required wayland objects will be null
static bool initialized = false;
/*
 * Wayland objects
 */
// required (crash if initialization fails)
struct wl_display*    display     = nullptr;
struct wl_registry*   registry    = nullptr;
struct wl_compositor* compositor  = nullptr;
struct xdg_wm_base*   wm_base     = nullptr;
struct wl_seat*       seat        = nullptr;
struct wl_surface*    surface     = nullptr;
struct xdg_surface*   surface_xdg = nullptr;
struct xdg_toplevel*  toplevel    = nullptr;
// optional (must check null)
struct zxdg_decoration_manager_v1*     decoration_manager       = nullptr;
struct zxdg_toplevel_decoration_v1*    toplevel_decoration      = nullptr;
struct wp_fractional_scale_manager_v1* fractional_scale_manager = nullptr;
struct wp_fractional_scale_v1*         fractional_scale         = nullptr;
/*
 * Window information
 */
uint32_t scale = 1, scale120 = 120;
bool     events_queued = false;
bool     event_close = false, event_resize = false;
// resizing information
uint32_t width = 0, height = 0;
uint32_t width_queued = 1920, height_queued = 1080;
void     error(const char* msg) {
  std::fprintf(stderr, "%s\n", msg);
  std::abort();
}
/*
 * Public Engine functions
 */
// Engine initialization
Engine::Engine() {
	// check double initialization
	if (initialized)
		error("Double initialization");
	initialized = true;
	// initialize Vulkan
	init_vulkan();
	// connect to Wayland
	display = wl_display_connect(nullptr);
	if (display == nullptr)
		error("Failed to establish wayland connection");
	registry = wl_display_get_registry(display);
	if (registry == nullptr)
		error("Failed to create a Wayland registry");
	wl_registry_add_listener(registry, &registry_listener, nullptr);
	// initialize Wayland interfaces
	wl_display_roundtrip(display);
	if (compositor == nullptr || wm_base == nullptr || seat == nullptr)
		error("Failed to initialize interface from wl_registry\n");
	// create window
	surface = wl_compositor_create_surface(compositor);
	wl_surface_add_listener(surface, &surface_listener, nullptr);
	surface_xdg = xdg_wm_base_get_xdg_surface(wm_base, surface);
	xdg_surface_add_listener(surface_xdg, &surface_xdg_listener, nullptr);
	toplevel = xdg_surface_get_toplevel(surface_xdg);
	xdg_toplevel_add_listener(toplevel, &toplevel_listener, nullptr);
	xdg_toplevel_set_title(toplevel, "Conflict of Countries");
	if (decoration_manager != nullptr) {
		toplevel_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(decoration_manager, toplevel);
	}
	if (fractional_scale_manager != nullptr) {
		fractional_scale = wp_fractional_scale_manager_v1_get_fractional_scale(fractional_scale_manager, surface);
		wp_fractional_scale_v1_add_listener(fractional_scale, &fractional_scale_listener, nullptr);
	}
	// give window to Vulkan
	VkWaylandSurfaceCreateInfoKHR wayland_surface_info{};
	wayland_surface_info.sType   = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
	wayland_surface_info.display = display;
	wayland_surface_info.surface = surface;
	if (vkCreateWaylandSurfaceKHR(instance, &wayland_surface_info, nullptr, &surface_khr) != VK_SUCCESS)
		error("Failed to initialize Vulkan surface");
	// window ready
	wl_surface_commit(surface);
}
// Engine deinitialization
Engine::~Engine() {
	// deinitialize Vulkan
	deinit_vulkan();
// Wayland deinitialization helper macro
#define DESTROY_WAYLAND_OBJECT(obj, type) \
	{                                       \
		type##_destroy(obj);                  \
		obj = nullptr;                        \
	}
	// destroy optional Wayland objects
	if (fractional_scale != nullptr)
		DESTROY_WAYLAND_OBJECT(fractional_scale, wp_fractional_scale_v1);
	if (fractional_scale_manager != nullptr)
		DESTROY_WAYLAND_OBJECT(fractional_scale_manager, wp_fractional_scale_manager_v1);
	if (toplevel_decoration != nullptr)
		DESTROY_WAYLAND_OBJECT(toplevel_decoration, zxdg_toplevel_decoration_v1);
	if (decoration_manager != nullptr)
		DESTROY_WAYLAND_OBJECT(decoration_manager, zxdg_decoration_manager_v1);
	// destroy required Wayland objects
	DESTROY_WAYLAND_OBJECT(toplevel, xdg_toplevel);
	DESTROY_WAYLAND_OBJECT(surface_xdg, xdg_surface);
	DESTROY_WAYLAND_OBJECT(surface, wl_surface);
	DESTROY_WAYLAND_OBJECT(seat, wl_seat)
	DESTROY_WAYLAND_OBJECT(wm_base, xdg_wm_base);
	DESTROY_WAYLAND_OBJECT(compositor, wl_compositor);
	DESTROY_WAYLAND_OBJECT(registry, wl_registry);
	// disconnect from Wayland
	wl_display_disconnect(display);
	// mark deinitiallized
	initialized = false;
}
void Engine::mainloop() {
	const int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if (epoll_fd == -1)
		error("Failed to create epoll instance");
	const int wayland_fd = wl_display_get_fd(display);
	{
		epoll_event event = {.events = EPOLLIN, .data = {.fd = wayland_fd}};
		epoll_ctl(epoll_fd, EPOLL_CTL_ADD, wayland_fd, &event);
	}
	while (true) {
		while (wl_display_prepare_read(display) != 0)
			wl_display_dispatch_pending(display);
		wl_display_flush(display);
		constexpr int events_buffer_size = 16;
		epoll_event   events_buffer[16]  = {};
		const int     n                  = epoll_wait(epoll_fd, events_buffer, events_buffer_size, -1);
		if (n == -1) {
			wl_display_cancel_read(display);
			continue;
		}
		bool wayland_ready = false;
		for (int i = 0; i < n; i++) {
			if (events_buffer[i].data.fd == wayland_fd)
				wayland_ready = true;
		}
		if (wayland_ready)
			wl_display_read_events(display);
		else
			wl_display_cancel_read(display);
		wl_display_dispatch_pending(display);
		if (events_queued) {
			events_queued = false;
			if (event_close) {
				event_close = false;
				break;
			}
			if (event_resize) {
				event_resize = false;
				width        = width_queued;
				height       = height_queued;
				resize(width * scale, height * scale);
				draw(width * scale, height * scale);
			}
		}
	}
	close(epoll_fd);
}
} // namespace Engine
