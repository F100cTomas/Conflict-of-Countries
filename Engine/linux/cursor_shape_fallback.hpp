#pragma once
#include "staging/cursor-shape/cursor-shape-v1.h"
#include <cstdint>
#include <unistd.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
namespace Engine {
class CursorShapeFallback {
	// timer
	int      timer_fd = -1;
	uint32_t interval = UINT32_MAX;
	// library
	void* lib                                                                                        = nullptr;
	struct wl_cursor_theme* (*wl_cursor_theme_load)(const char* name, int size, struct wl_shm* shm)  = nullptr;
	void (*wl_cursor_theme_destroy)(struct wl_cursor_theme* theme)                                   = nullptr;
	struct wl_cursor* (*wl_cursor_theme_get_cursor)(struct wl_cursor_theme* theme, const char* name) = nullptr;
	struct wl_buffer* (*wl_cursor_image_get_buffer)(struct wl_cursor_image* image)                   = nullptr;
	// wayland-cursor objects
	struct wl_cursor_theme*         theme   = nullptr;
	struct wl_cursor*               cursor  = nullptr;
	struct wl_surface*              surface = nullptr;
	uint32_t                        index   = 0;
	wp_cursor_shape_device_v1_shape current_shape;
	uint32_t                        serial;

public:
	CursorShapeFallback();
	~CursorShapeFallback();
	void       set_cursor_shape(wp_cursor_shape_device_v1_shape shape);
	inline int fd() const {
		return timer_fd;
	}
	inline void save_serial(uint32_t new_serial) {
		serial = new_serial;
	}
	inline void advance_frame() {
		index = (index + 1) % cursor->image_count;
		set_cursor_shape(current_shape);
	}
	inline void remove_timer() {
		if (timer_fd != -1)
			close(timer_fd);
		timer_fd = -1;
		interval = UINT32_MAX;
	}
};
} // namespace Engine
