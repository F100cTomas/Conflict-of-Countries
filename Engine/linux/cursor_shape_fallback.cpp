#include "cursor_shape_fallback.hpp"
#include "coc.hpp"
#include "linux.hpp"
#include "staging/cursor-shape/cursor-shape-v1.h"
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
namespace Engine {
CursorShapeFallback::CursorShapeFallback() {
	lib = dlopen("libwayland-cursor.so.0", RTLD_NOW | RTLD_LOCAL);
	if (lib == nullptr)
		error("Could not load dynamic library");
	// wl_cursor_theme_load
	wl_cursor_theme_load = reinterpret_cast<struct wl_cursor_theme* (*)(const char* name, int size, struct wl_shm* shm)>(
	    dlsym(lib, "wl_cursor_theme_load"));
	if (wl_cursor_theme_load == nullptr)
		error("Failed to load wl_cursor_theme_load");
	// wl_cursor_theme_destroy
	wl_cursor_theme_destroy =
	    reinterpret_cast<void (*)(struct wl_cursor_theme* theme)>(dlsym(lib, "wl_cursor_theme_destroy"));
	if (wl_cursor_theme_destroy == nullptr)
		error("Failed to load wl_cursor_theme_destroy");
	// wl_cursor_theme_get_cursor
	wl_cursor_theme_get_cursor =
	    reinterpret_cast<struct wl_cursor* (*)(struct wl_cursor_theme * theme, const char* name)>(
	        dlsym(lib, "wl_cursor_theme_get_cursor"));
	if (wl_cursor_theme_get_cursor == nullptr)
		error("Failed to load wl_cursor_theme_get_cursor");
	// wl_cursor_image_get_buffer
	wl_cursor_image_get_buffer =
	    reinterpret_cast<struct wl_buffer* (*)(struct wl_cursor_image * image)>(dlsym(lib, "wl_cursor_image_get_buffer"));
	if (wl_cursor_image_get_buffer == nullptr)
		error("Failed to load wl_cursor_image_get_buffer");
	// wl_cursor_theme
	theme = wl_cursor_theme_load(nullptr, 24, shm);
	if (theme == nullptr)
		error("Failed to load theme");
	surface = wl_compositor_create_surface(compositor);
	if (surface == nullptr)
		error("Failed to create surface");
}
CursorShapeFallback::~CursorShapeFallback() {
	wl_surface_destroy(surface);
	wl_cursor_theme_destroy(theme);
	dlclose(lib);
}
namespace {
const char* const  shape_default[]       = {"left_ptr", "default", nullptr};
const char* const  shape_context_menu[]  = {"context_menu", "left_ptr", nullptr};
const char* const  shape_help[]          = {"question_arrow", "help", nullptr};
const char* const  shape_pointer[]       = {"pointer", "hand2", "hand1", nullptr};
const char* const  shape_progress[]      = {"left_ptr_watch", "watch", nullptr};
const char* const  shape_wait[]          = {"watch", "wait", nullptr};
const char* const  shape_cell[]          = {"cell", "crosshair", nullptr};
const char* const  shape_crosshair[]     = {"crosshair", "cross", nullptr};
const char* const  shape_text[]          = {"text", "xterm", nullptr};
const char* const  shape_vertical_text[] = {"vertical-text", "text", "xterm", nullptr};
const char* const  shape_alias[]         = {"alias", "link", nullptr};
const char* const  shape_copy[]          = {"copy", "dnd-copy", nullptr};
const char* const  shape_move[]          = {"move", "fleur", nullptr};
const char* const  shape_no_drop[]       = {"no-drop", "dnd-none", nullptr};
const char* const  shape_not_allowed[]   = {"not-allowed", "crossed_circle", "forbidden", nullptr};
const char* const  shape_grab[]          = {"grab", "openhand", nullptr};
const char* const  shape_grabbing[]      = {"grabbing", "closedhand", nullptr};
const char* const  shape_e_resize[]      = {"e-resize", "right_side", nullptr};
const char* const  shape_n_resize[]      = {"n-resize", "top_side", nullptr};
const char* const  shape_ne_resize[]     = {"ne-resize", "top_right_corner", nullptr};
const char* const  shape_nw_resize[]     = {"nw-resize", "top_left_corner", nullptr};
const char* const  shape_s_resize[]      = {"s-resize", "bottom_side", nullptr};
const char* const  shape_se_resize[]     = {"se-resize", "bottom_right_corner", nullptr};
const char* const  shape_sw_resize[]     = {"sw-resize", "bottom_left_corner", nullptr};
const char* const  shape_w_resize[]      = {"w-resize", "left_side", nullptr};
const char* const  shape_ew_resize[]     = {"ew-resize", "sb_h_double_arrow", "h_double_arrow", nullptr};
const char* const  shape_ns_resize[]     = {"ns-resize", "sb_v_double_arrow", "v_double_arrow", nullptr};
const char* const  shape_nesw_resize[]   = {"nesw-resize", "fd_double_arrow", nullptr};
const char* const  shape_nwse_resize[]   = {"nwse-resize", "bd_double_arrow", nullptr};
const char* const  shape_col_resize[]    = {"col-resize", "split_h", "sb_h_double_arrow", nullptr};
const char* const  shape_row_resize[]    = {"row-resize", "split_v", "sb_v_double_arrow", nullptr};
const char* const  shape_all_scroll[]    = {"all_scroll", "fleur", nullptr};
const char* const  shape_zoom_in[]       = {"zoom-in", nullptr};
const char* const  shape_zoom_out[]      = {"zoom-out", nullptr};
const char* const  shape_dnd_ask[]       = {"dnd-ask", "question_arrow", nullptr};
const char* const  shape_all_resize[]    = {"size_all", "fleur", nullptr};
const char* const* shape_names(wp_cursor_shape_device_v1_shape shape) {
	switch (shape) {
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT: return shape_default;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_CONTEXT_MENU: return shape_context_menu;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_HELP: return shape_help;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER: return shape_pointer;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_PROGRESS: return shape_progress;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_WAIT: return shape_wait;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_CELL: return shape_cell;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_CROSSHAIR: return shape_crosshair;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_TEXT: return shape_text;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_VERTICAL_TEXT: return shape_vertical_text;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_ALIAS: return shape_alias;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_COPY: return shape_copy;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_MOVE: return shape_move;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NO_DROP: return shape_no_drop;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NOT_ALLOWED: return shape_not_allowed;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_GRAB: return shape_grab;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_GRABBING: return shape_grabbing;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_E_RESIZE: return shape_e_resize;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_N_RESIZE: return shape_n_resize;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NE_RESIZE: return shape_ne_resize;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NW_RESIZE: return shape_nw_resize;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_S_RESIZE: return shape_s_resize;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_SE_RESIZE: return shape_se_resize;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_SW_RESIZE: return shape_sw_resize;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_W_RESIZE: return shape_w_resize;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_EW_RESIZE: return shape_ew_resize;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NS_RESIZE: return shape_ns_resize;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NESW_RESIZE: return shape_nesw_resize;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NWSE_RESIZE: return shape_nwse_resize;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_COL_RESIZE: return shape_col_resize;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_ROW_RESIZE: return shape_row_resize;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_ALL_SCROLL: return shape_all_scroll;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_ZOOM_IN: return shape_zoom_in;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_ZOOM_OUT: return shape_zoom_out;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DND_ASK: return shape_dnd_ask;
	case wp_cursor_shape_device_v1_shape::WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_ALL_RESIZE: return shape_all_resize;
	default: error("Unknown cursor shape"); break;
	}
	return nullptr;
}
} // namespace
void CursorShapeFallback::set_cursor_shape(wp_cursor_shape_device_v1_shape shape) {
	if (cursor == nullptr)
		goto find_cursor_name;
	if (shape == current_shape)
		goto apply_cursor;
	{
		const char* const* names       = shape_names(shape);
		const char*        cursor_name = nullptr;
		for (uint32_t i = 0; names[i] != nullptr; i++) {
			cursor_name = names[i];
			if (std::strcmp(cursor_name, cursor->name) == 0)
				break;
			struct wl_cursor* tmp_cursor = wl_cursor_theme_get_cursor(theme, cursor_name);
			if (tmp_cursor == nullptr)
				continue;
			cursor = tmp_cursor;
			index  = 0;
			break;
		}
		goto apply_cursor;
	}
find_cursor_name: {
	index                          = 0;
	const char* const* names       = shape_names(shape);
	const char*        cursor_name = nullptr;
	for (uint32_t i = 0; names[i] != nullptr; i++) {
		cursor_name = names[i];
		cursor      = wl_cursor_theme_get_cursor(theme, cursor_name);
		if (cursor != nullptr)
			goto apply_cursor;
	}
	return;
}
apply_cursor:
	wl_surface_attach(surface, wl_cursor_image_get_buffer(cursor->images[index]), 0, 0);
	wl_surface_damage_buffer(surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(surface);
	wl_pointer_set_cursor(pointer, serial, surface, cursor->images[index]->hotspot_x, cursor->images[index]->hotspot_y);
	current_shape = shape;
	if (cursor->image_count > 1) {
		if (timer_fd == -1) {
			if (epoll_fd == -1)
				error("Failed to find event loop");
			timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
			if (timer_fd == -1)
				error("Failed to create timer");
			epoll_event event = {.events = EPOLLIN, .data = {.fd = timer_fd}};
			if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &event) != 0)
				error("epoll_ctl failed");
		}
		if (cursor->images[index]->delay == interval)
			return;
		interval = cursor->images[index]->delay;
		std::fprintf(stderr, "interval: %d\n", interval);
		itimerspec spec = {
		    .it_interval = {.tv_sec = 0, .tv_nsec = 1'000'000 * interval},
		    .it_value    = {.tv_sec = 0, .tv_nsec = 1'000'000 * interval},
		};
		if (timerfd_settime(timer_fd, 0, &spec, nullptr) != 0)
			error("timerfd_settime failed");
	} else if (timer_fd != -1) {
		remove_timer();
	}
}
} // namespace Engine
