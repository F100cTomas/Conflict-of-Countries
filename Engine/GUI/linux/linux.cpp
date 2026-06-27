#include "linux.hpp"
#include "coc.hpp"
#include "common/common.hpp"
#include "stable/xdg-shell/xdg-shell.h"
#include "staging/fractional-scale/fractional-scale-v1.h"
#include "unstable/xdg-decoration/xdg-decoration-unstable-v1.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
namespace Engine {
static bool                            initialized              = false;
struct wl_display*                     display                  = nullptr;
struct wl_registry*                    registry                 = nullptr;
struct wl_compositor*                  compositor               = nullptr;
struct wl_shm*                         shm                      = nullptr;
struct xdg_wm_base*                    wm_base                  = nullptr;
struct zxdg_decoration_manager_v1*     decoration_manager       = nullptr;
struct wp_fractional_scale_manager_v1* fractional_scale_manager = nullptr;
struct wl_surface*                     surface                  = nullptr;
struct xdg_surface*                    surface_xdg              = nullptr;
struct xdg_toplevel*                   toplevel                 = nullptr;
struct zxdg_toplevel_decoration_v1*    toplevel_decoration      = nullptr;
struct wp_fractional_scale_v1*         fractional_scale         = nullptr;
bool                                   is_running = true, will_resize = false;
uint32_t                               old_width = 0, old_height = 0;
uint32_t                               width = 1920, height = 1080;
uint32_t                               scale = 1, scale120 = 120;
void                                   error(const char* msg) {
  std::fprintf(stderr, "%s\n", msg);
  std::abort();
}
Engine::Engine() {
	if (initialized)
		error("Double initialization");
	initialized = true;
	init_vulkan();
	display = wl_display_connect(nullptr);
	if (display == nullptr)
		error("Failed to establish wayland connection");
	registry = wl_display_get_registry(display);
	if (registry == nullptr)
		error("Failed to create a Wayland registry");
	wl_registry_add_listener(registry, &registry_listener, nullptr);
	wl_display_roundtrip(display);
	if (compositor == nullptr || shm == nullptr || wm_base == nullptr || decoration_manager == nullptr)
		error("Failed to initialize interface from wl_registry\n");
	surface = wl_compositor_create_surface(compositor);
	wl_surface_add_listener(surface, &surface_listener, nullptr);
	surface_xdg = xdg_wm_base_get_xdg_surface(wm_base, surface);
	xdg_surface_add_listener(surface_xdg, &surface_xdg_listener, nullptr);
	toplevel = xdg_surface_get_toplevel(surface_xdg);
	xdg_toplevel_add_listener(toplevel, &toplevel_listener, nullptr);
	xdg_toplevel_set_title(toplevel, "Conflict of Countries");
	toplevel_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(decoration_manager, toplevel);
	if (fractional_scale_manager != nullptr) {
		fractional_scale = wp_fractional_scale_manager_v1_get_fractional_scale(fractional_scale_manager, surface);
		wp_fractional_scale_v1_add_listener(fractional_scale, &fractional_scale_listener, nullptr);
	}
	VkWaylandSurfaceCreateInfoKHR wayland_surface_info{};
	wayland_surface_info.sType   = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
	wayland_surface_info.display = display;
	wayland_surface_info.surface = surface;
	if (vkCreateWaylandSurfaceKHR(instance, &wayland_surface_info, nullptr, &surface_khr) != VK_SUCCESS)
		error("Failed to initialize Vulkan surface");
	wl_surface_commit(surface);
}
Engine::~Engine() {
	deinit_vulkan();
	zxdg_toplevel_decoration_v1_destroy(toplevel_decoration);
	xdg_toplevel_destroy(toplevel);
	xdg_surface_destroy(surface_xdg);
	wl_surface_destroy(surface);
	zxdg_decoration_manager_v1_destroy(decoration_manager);
	xdg_wm_base_destroy(wm_base);
	wl_shm_destroy(shm);
	wl_compositor_destroy(compositor);
	wl_registry_destroy(registry);
	wl_display_disconnect(display);
	initialized = false;
}
void Engine::mainloop() {
	while (is_running && wl_display_dispatch(display)) {
		if (will_resize) {
			old_width  = width;
			old_height = height;
			resize(width * scale, height * scale);
			draw(width * scale, height * scale);
			will_resize = false;
		}
	}
}
void registry_global([[maybe_unused]] void* data, [[maybe_unused]] struct wl_registry* wl_registry, uint32_t name,
                     const char* interface, [[maybe_unused]] uint32_t version) {
	// std::fprintf(stderr, "wl_registry.global(%d, %s, %d)\n", name, interface, version);
	if (std::strcmp(interface, wl_compositor_interface.name) == 0) {
		compositor =
		    reinterpret_cast<struct wl_compositor*>(wl_registry_bind(wl_registry, name, &wl_compositor_interface, 6));
		return;
	}
	if (std::strcmp(interface, wl_shm_interface.name) == 0) {
		shm = reinterpret_cast<struct wl_shm*>(wl_registry_bind(wl_registry, name, &wl_shm_interface, 2));
		return;
	}
	if (std::strcmp(interface, xdg_wm_base_interface.name) == 0) {
		wm_base = reinterpret_cast<struct xdg_wm_base*>(wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, 6));
		xdg_wm_base_add_listener(wm_base, &wm_base_listener, nullptr);
		return;
	}
	if (std::strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
		decoration_manager = reinterpret_cast<struct zxdg_decoration_manager_v1*>(
		    wl_registry_bind(wl_registry, name, &zxdg_decoration_manager_v1_interface, 1));
		return;
	}
	if (std::strcmp(interface, wp_fractional_scale_manager_v1_interface.name) == 0) {
		fractional_scale_manager = reinterpret_cast<struct wp_fractional_scale_manager_v1*>(
		    wl_registry_bind(wl_registry, name, &wp_fractional_scale_manager_v1_interface, 1));
		return;
	}
}
void registry_global_remove([[maybe_unused]] void* data, [[maybe_unused]] struct wl_registry* wl_registry,
                            [[maybe_unused]] uint32_t name) {
	error("wl_registry listener: global_remove not implemented\n");
}
void wm_base_ping([[maybe_unused]] void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial) {
	xdg_wm_base_pong(xdg_wm_base, serial);
}
void surface_enter([[maybe_unused]] void* data, [[maybe_unused]] struct wl_surface* wl_surface,
                   [[maybe_unused]] struct wl_output* output) {
	std::fprintf(stderr, "xdg_surface.wm_capabilities()\n");
}
void surface_leave([[maybe_unused]] void* data, [[maybe_unused]] struct wl_surface* wl_surface,
                   [[maybe_unused]] struct wl_output* output) {
	std::fprintf(stderr, "xdg_surface.wm_capabilities()\n");
}
void surface_preferred_buffer_scale([[maybe_unused]] void* data, struct wl_surface* wl_surface, int32_t factor) {
	std::fprintf(stderr, "xdg_surface.preferred_buffer_scale(%d)\n", factor);
	scale = factor;
	wl_surface_set_buffer_scale(wl_surface, factor);
}
void surface_preferred_buffer_transform([[maybe_unused]] void* data, struct wl_surface* wl_surface,
                                        uint32_t transform) {
	std::fprintf(stderr, "xdg_surface.preferred_buffer_transform(%d)\n", transform);
	wl_surface_set_buffer_transform(wl_surface, transform);
}
void surface_xdg_configure([[maybe_unused]] void* data, struct xdg_surface* xdg_surface, uint32_t serial) {
	std::fprintf(stderr, "xdg_surface.configure()\n");
	if (width == old_width && height == old_height)
		return;
	xdg_surface_ack_configure(xdg_surface, serial);
	will_resize = true;
}
void toplevel_configure([[maybe_unused]] void* data, [[maybe_unused]] struct xdg_toplevel* xdg_toplevel, int32_t width,
                        int32_t height, [[maybe_unused]] struct wl_array* states) {
	std::fprintf(stderr, "xdg_toplevel.configure(%d, %d)\n", width, height);
	if (width > 0)
		::Engine::width = width;
	if (height > 0)
		::Engine::height = height;
}
void toplevel_close([[maybe_unused]] void* data, [[maybe_unused]] struct xdg_toplevel* xdg_toplevel) {
	is_running = false;
}
void toplevel_configure_bounds([[maybe_unused]] void* data, [[maybe_unused]] struct xdg_toplevel* xdg_toplevel,
                               [[maybe_unused]] int32_t width, [[maybe_unused]] int32_t height) {
	std::fprintf(stderr, "xdg_toplevel.configure_bounds(%d, %d)\n", width, height);
}
void toplevel_wm_capablities([[maybe_unused]] void* data, [[maybe_unused]] struct xdg_toplevel* xdg_toplevel,
                             [[maybe_unused]] struct wl_array* capabilities) {
	std::fprintf(stderr, "xdg_toplevel.wm_capabilities()\n");
}
void fractional_scale_preferred_scale([[maybe_unused]] void*                          data,
                                      [[maybe_unused]] struct wp_fractional_scale_v1* fractional_scale,
                                      uint32_t                                        scale) {
	std::fprintf(stderr, "wp_fractional_scale_v1.preferred_scale(%d/120)\n", scale);
	scale120 = scale;
}
} // namespace Engine
