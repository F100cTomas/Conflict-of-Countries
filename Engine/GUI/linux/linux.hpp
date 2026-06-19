#pragma once
#include "stable/xdg-shell/xdg-shell.h"
#include "unstable/xdg-decoration/xdg-decoration-unstable-v1.h"
#include <cstdint>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-util.h>
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
extern bool                                is_running, will_resize;
extern uint32_t                            old_width, old_height;
extern uint32_t                            width, height;
extern uint32_t                            scale;
// wl_registry
void registry_global(void* data, struct wl_registry* wl_registry, uint32_t name, const char* interface,
                     uint32_t version);
void registry_global_remove(void* data, struct wl_registry* wl_registry, uint32_t name);
inline struct wl_registry_listener registry_listener = {.global        = registry_global,
                                                        .global_remove = registry_global_remove};
// xdg_wm_base
void                               wm_base_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial);
inline struct xdg_wm_base_listener wm_base_listener = {.ping = wm_base_ping};
// wl_surface
void surface_enter(void* data, struct wl_surface* wl_surface, struct wl_output* output);
void surface_leave(void* data, struct wl_surface* wl_surface, struct wl_output* output);
void surface_preferred_buffer_scale(void* data, struct wl_surface* wl_surface, int32_t factor);
void surface_preferred_buffer_transform(void* data, struct wl_surface* wl_surface, uint32_t transform);
inline struct wl_surface_listener surface_listener = {.enter                      = surface_enter,
                                                      .leave                      = surface_leave,
                                                      .preferred_buffer_scale     = surface_preferred_buffer_scale,
                                                      .preferred_buffer_transform = surface_preferred_buffer_transform};
// xdg_surface
void                               surface_xdg_configure(void* data, struct xdg_surface* xdg_surface, uint32_t serial);
inline struct xdg_surface_listener surface_xdg_listener = {.configure = surface_xdg_configure};
// xdg_toplevel
void toplevel_configure(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height,
                        struct wl_array* states);
void toplevel_close(void* data, struct xdg_toplevel* xdg_toplevel);
void toplevel_configure_bounds(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height);
void toplevel_wm_capablities(void* data, struct xdg_toplevel* xdg_toplevel, struct wl_array* capabilities);
inline struct xdg_toplevel_listener toplevel_listener = {.configure        = toplevel_configure,
                                                         .close            = toplevel_close,
                                                         .configure_bounds = toplevel_configure_bounds,
                                                         .wm_capabilities  = toplevel_wm_capablities};
} // namespace Engine
