#pragma once
#include "stable/xdg-shell/xdg-shell.h"
#include "unstable/xdg-decoration/xdg-decoration-unstable-v1.h"
#include <cstdint>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
namespace Engine {
extern struct wl_display*                  display;
extern struct wl_registry*                 registry;
extern struct wl_compositor*               compositor;
extern struct wl_shm*                      shm;
extern struct xdg_wm_base*                 wm_base;
extern struct zxdg_decoration_manager_v1*  decoration_manager;
extern struct wl_surface*                  surface;
extern struct xdg_surface*                 surface_xdg;
extern struct xdg_toplevel*                toplevel;
extern struct zxdg_toplevel_decoration_v1* toplevel_decoration;
extern struct wl_buffer*                   buffer;
void registry_global(void* data, struct wl_registry* wl_registry, uint32_t name, const char* interface,
                     uint32_t version);
void registry_global_remove(void* data, struct wl_registry* wl_registry, uint32_t name);
inline struct wl_registry_listener registry_listener = {.global        = registry_global,
                                                        .global_remove = registry_global_remove};
void                               wm_base_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial);
inline struct xdg_wm_base_listener wm_base_listener = {.ping = wm_base_ping};
void                               surface_xdg_configure(void* data, struct xdg_surface* xdg_surface, uint32_t serial);
inline struct xdg_surface_listener surface_xdg_listener = {.configure = surface_xdg_configure};
} // namespace Engine
