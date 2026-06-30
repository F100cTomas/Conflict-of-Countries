#pragma once
#include "cursor_shape_fallback.hpp"
#include "decoration_fallback.hpp"
#include "stable/xdg-shell/xdg-shell.h"
#include "staging/cursor-shape/cursor-shape-v1.h"
#include "staging/fractional-scale/fractional-scale-v1.h"
#include "unstable/xdg-decoration/xdg-decoration-unstable-v1.h"
#include <cstdint>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-util.h>
namespace Engine {
/*
 * Wayland objects
 */
// required (crash if initialization fails)
extern struct wl_display*    display;
extern struct wl_registry*   registry;
extern struct wl_compositor* compositor;
extern struct wl_shm*        shm;
extern struct xdg_wm_base*   wm_base;
extern struct wl_seat*       seat;
extern struct wl_pointer*    pointer;
extern struct wl_surface*    surface;
// if decoration_fallback is not null, these will be null:
extern struct xdg_surface*  surface_xdg;
extern struct xdg_toplevel* toplevel;
// optional (must check null)
extern struct zxdg_decoration_manager_v1*     decoration_manager;
extern struct zxdg_toplevel_decoration_v1*    toplevel_decoration;
extern struct wp_cursor_shape_manager_v1*     cursor_shape_manager;
extern struct wp_cursor_shape_device_v1*      cursor_shape_device;
extern struct wp_fractional_scale_manager_v1* fractional_scale_manager;
extern struct wp_fractional_scale_v1*         fractional_scale;
// fallback for missing extensions
extern DecorationFallback*  decoration_fallback;
extern CursorShapeFallback* cursor_shape_fallback;
/*
 * Window information
 */
// linux file descriptor of epoll object
extern int epoll_fd;
// information for DPI awareness
extern uint32_t scale, scale120;
// information for events handled outside callback
extern bool events_queued;
extern bool event_close, event_resize;
// resizing information
extern uint32_t width, height;
extern uint32_t width_queued, height_queued;
/*
 * Wayland callback Functions (wayland_callbacks.cpp)
 */
// wl_registry
void registry_global(void* data, struct wl_registry* wl_registry, uint32_t name, const char* interface,
                     uint32_t version);
void registry_global_remove(void* data, struct wl_registry* wl_registry, uint32_t name);
inline struct wl_registry_listener registry_listener = {.global        = registry_global,
                                                        .global_remove = registry_global_remove};
// xdg_wm_base
void                               wm_base_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial);
inline struct xdg_wm_base_listener wm_base_listener = {.ping = wm_base_ping};
// wl_seat
void                           seat_capabilities(void* data, struct wl_seat* wl_seat, uint32_t capabilities);
void                           seat_name(void* data, struct wl_seat* wl_seat, const char* name);
inline struct wl_seat_listener seat_listener = {.capabilities = seat_capabilities, .name = seat_name};
// wl_pointer
void pointer_enter(void* data, struct wl_pointer* pointer, uint32_t serial, struct wl_surface* surface,
                   wl_fixed_t surface_x, wl_fixed_t surface_y);
void pointer_leave(void* data, struct wl_pointer* pointer, uint32_t serial, struct wl_surface* surface);
void pointer_motion(void* data, struct wl_pointer* pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y);
void pointer_button(void* data, struct wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button,
                    uint32_t state);
void pointer_axis(void* data, struct wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
void pointer_frame(void* data, struct wl_pointer* pointer);
void pointer_axis_source(void* data, struct wl_pointer* pointer, uint32_t axis_source);
void pointer_axis_stop(void* data, struct wl_pointer* pointer, uint32_t time, uint32_t axis);
void pointer_axis_discrete(void* data, struct wl_pointer* pointer, uint32_t axis, int32_t discrete);
void pointer_axis_value120(void* data, struct wl_pointer* pointer, uint32_t axis, int32_t value120);
void pointer_axis_relative_direction(void* data, struct wl_pointer* pointer, uint32_t axis, uint32_t direction);
inline struct wl_pointer_listener pointer_listener = {.enter                   = pointer_enter,
                                                      .leave                   = pointer_leave,
                                                      .motion                  = pointer_motion,
                                                      .button                  = pointer_button,
                                                      .axis                    = pointer_axis,
                                                      .frame                   = pointer_frame,
                                                      .axis_source             = pointer_axis_source,
                                                      .axis_stop               = pointer_axis_stop,
                                                      .axis_discrete           = pointer_axis_discrete,
                                                      .axis_value120           = pointer_axis_value120,
                                                      .axis_relative_direction = pointer_axis_relative_direction};
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
// wp_fractional_scale_v1
void fractional_scale_preferred_scale(void* data, struct wp_fractional_scale_v1* fractional_scale, uint32_t scale);
inline struct wp_fractional_scale_v1_listener fractional_scale_listener = {.preferred_scale =
                                                                               fractional_scale_preferred_scale};
} // namespace Engine
