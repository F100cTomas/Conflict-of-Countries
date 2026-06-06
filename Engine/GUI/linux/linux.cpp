#include "linux.hpp"
#include "coc.hpp"
#include "stable/xdg-shell/xdg-shell.h"
#include "unstable/xdg-decoration/xdg-decoration-unstable-v1.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
namespace Engine {
static bool                         initialized         = false;
struct wl_display*                  display             = nullptr;
struct wl_registry*                 registry            = nullptr;
struct wl_compositor*               compositor          = nullptr;
struct wl_shm*                      shm                 = nullptr;
struct xdg_wm_base*                 wm_base             = nullptr;
struct zxdg_decoration_manager_v1*  decoration_manager  = nullptr;
struct wl_surface*                  surface             = nullptr;
struct xdg_surface*                 surface_xdg         = nullptr;
struct xdg_toplevel*                toplevel            = nullptr;
struct zxdg_toplevel_decoration_v1* toplevel_decoration = nullptr;
struct wl_buffer*                   buffer              = nullptr;
Engine::Engine() {
	if (initialized) {
		std::fprintf(stderr, "Double initialization\n");
		std::abort();
	}
	initialized = true;
	display     = wl_display_connect(nullptr);
	if (display == nullptr) {
		std::fprintf(stderr, "Unable to establish wayland connection\n");
		std::abort();
	}
	registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, nullptr);
	wl_display_roundtrip(display);
	if (compositor == nullptr || shm == nullptr || wm_base == nullptr || decoration_manager == nullptr) {
		std::fprintf(stderr, "Failed to initialize interface from wl_registry\n");
		std::abort();
	}
	surface     = wl_compositor_create_surface(compositor);
	surface_xdg = xdg_wm_base_get_xdg_surface(wm_base, surface);
	xdg_surface_add_listener(surface_xdg, &surface_xdg_listener, nullptr);
	toplevel            = xdg_surface_get_toplevel(surface_xdg);
	toplevel_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(decoration_manager, toplevel);
	xdg_toplevel_set_title(toplevel, "Conflict of Countries");
	wl_surface_commit(surface);
}
Engine::~Engine() {
	wl_buffer_destroy(buffer);
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
	while (wl_display_dispatch(display))
		;
}
void registry_global([[maybe_unused]] void* data, [[maybe_unused]] struct wl_registry* wl_registry, uint32_t name,
                     const char* interface, uint32_t version) {
	std::printf("%d %s %d\n", name, interface, version);
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
}
void registry_global_remove([[maybe_unused]] void* data, [[maybe_unused]] struct wl_registry* wl_registry,
                            [[maybe_unused]] uint32_t name) {
	std::fprintf(stderr, "wl_registry listener: global_remove not implemented\n");
	std::abort();
}
void wm_base_ping([[maybe_unused]] void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial) {
	xdg_wm_base_pong(xdg_wm_base, serial);
}
static int32_t abs(int32_t x) {
	if (x < 0)
		return -x;
	return x;
}
static bool line(int32_t x, int32_t y) {
	x -= 1920 / 2;
	y -= 1080 / 2;
	return abs(-x + 2 * y + 4) < 8;
}
void surface_xdg_configure([[maybe_unused]] void* data, [[maybe_unused]] struct xdg_surface* xdg_surface,
                           [[maybe_unused]] uint32_t serial) {
	if (buffer != nullptr)
		return;
	xdg_surface_ack_configure(xdg_surface, serial);
	int fd = memfd_create("Conflict of Countries", MFD_CLOEXEC);
	ftruncate(fd, 1920 * 1080 * 4);
	uint32_t* raw =
	    reinterpret_cast<uint32_t*>(mmap(nullptr, 1920 * 1080 * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
	struct wl_shm_pool* shm_pool = wl_shm_create_pool(shm, fd, 1920 * 1808 * 4);
	buffer                       = wl_shm_pool_create_buffer(shm_pool, 0, 1920, 1080, 1920 * 4, WL_SHM_FORMAT_XRGB8888);
	wl_shm_pool_destroy(shm_pool);
	wl_surface_attach(surface, buffer, 0, 0);
	for (uint32_t y = 0; y < 1080; y++)
		for (uint32_t x = 0; x < 1920; x++)
			raw[1920 * y + x] = line(x, y) ? 0x00000000 : 0x00FFFFFF;
	wl_surface_damage_buffer(surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(surface);
}
} // namespace Engine
