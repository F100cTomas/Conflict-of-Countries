#include "decoration_fallback.hpp"
#include "coc.hpp"
#include "common/common.hpp"
#include "linux/linux.hpp"
#include <cstdio>
#include <dlfcn.h>
#include <libdecor-0/libdecor.h>
#include <wayland-client-protocol.h>
namespace Engine {
DecorationFallback::DecorationFallback() {
	lib = dlopen("libdecor-0.so.0", RTLD_NOW | RTLD_LOCAL);
	if (lib == nullptr)
		error("Could not load dynamic library");
	// libdecor_unref
	libdecor_unref = reinterpret_cast<void (*)(struct libdecor* context)>(dlsym(lib, "libdecor_unref"));
	if (libdecor_unref == nullptr)
		error("failed to load libdecor_unref");
	// libdecor_new
	libdecor_new = reinterpret_cast<struct libdecor* (*)(struct wl_display * display, struct libdecor_interface * iface)>(
	    dlsym(lib, "libdecor_new"));
	if (libdecor_new == nullptr)
		error("failed to load libdecor_new");
	// libdecor_dispatch
	libdecor_dispatch = reinterpret_cast<int (*)(struct libdecor* context, int timeout)>(dlsym(lib, "libdecor_dispatch"));
	if (libdecor_dispatch == nullptr)
		error("failed to load libdecor_dispatch");
	// libdecor_decorate
	libdecor_decorate =
	    reinterpret_cast<struct libdecor_frame* (*)(struct libdecor * context, struct wl_surface * surface,
	                                                struct libdecor_frame_interface * iface, void* user_data)>(
	        dlsym(lib, "libdecor_decorate"));
	if (libdecor_decorate == nullptr)
		error("failed to load libdecor_decorate");
	// libdecor_frame_unref
	libdecor_frame_unref = reinterpret_cast<void (*)(struct libdecor_frame* frame)>(dlsym(lib, "libdecor_frame_unref"));
	if (libdecor_frame_unref == nullptr)
		error("failed to load libdecor_frame_unref");
	// libdecor_frame_set_title
	libdecor_frame_set_title = reinterpret_cast<void (*)(struct libdecor_frame* frame, const char* title)>(
	    dlsym(lib, "libdecor_frame_set_title"));
	if (libdecor_frame_set_title == nullptr)
		error("failed to load libdecor_frame_set_title");
	// libdecor_frame_commit
	libdecor_frame_commit =
	    reinterpret_cast<void (*)(struct libdecor_frame* frame, struct libdecor_state* state,
	                              struct libdecor_configuration* configuration)>(dlsym(lib, "libdecor_frame_commit"));
	if (libdecor_frame_commit == nullptr)
		error("failed to load libdecor_frame_commit");
	// libdecor_frame_map
	libdecor_frame_map = reinterpret_cast<void (*)(struct libdecor_frame* frame)>(dlsym(lib, "libdecor_frame_map"));
	if (libdecor_frame_map == nullptr)
		error("failed to load libdecor_frame_map");
	// libdecor_state_new
	libdecor_state_new =
	    reinterpret_cast<struct libdecor_state* (*)(int width, int height)>(dlsym(lib, "libdecor_state_new"));
	if (libdecor_state_new == nullptr)
		error("failed to load libdecor_state_new");
	// libdecor_state_free
	libdecor_state_free = reinterpret_cast<void (*)(struct libdecor_state* state)>(dlsym(lib, "libdecor_state_free"));
	if (libdecor_state_free == nullptr)
		error("failed to load libdecor_state_free");
	// libdecor_configuration_get_content_size
	libdecor_configuration_get_content_size =
	    reinterpret_cast<bool (*)(struct libdecor_configuration* configuration, struct libdecor_frame* frame, int* width,
	                              int* height)>(dlsym(lib, "libdecor_configuration_get_content_size"));
	if (libdecor_configuration_get_content_size == nullptr)
		error("failed to load libdecor_frame_map");
	// libdecor
	decor = libdecor_new(display, &decor_listener);
	if (decor == nullptr)
		error("Failed to initialize libdecor");
	decor_frame = libdecor_decorate(decor, surface, &decor_frame_listener, this);
	libdecor_frame_set_title(decor_frame, "Conflict of Countries");
	libdecor_frame_map(decor_frame);
}
DecorationFallback::~DecorationFallback() {
	if (decor_state != nullptr)
		libdecor_state_free(decor_state);
	libdecor_frame_unref(decor_frame);
	libdecor_unref(decor);
	dlclose(lib);
}
#ifdef NDEBUG
#define LOG(...)
#else
#define LOG(...) std::fprintf(stderr, __VA_ARGS__)
#endif
void decor_error([[maybe_unused]] struct libdecor* context, [[maybe_unused]] enum libdecor_error error,
                 const char* message) {
	LOG("libdecor.error(%s)\n", message);
}
void decor_frame_configure(struct libdecor_frame* frame, struct libdecor_configuration* configuration,
                           void* user_data) {
	DecorationFallback* fallback = reinterpret_cast<DecorationFallback*>(user_data);
	int                 w = 0, h = 0;
	fallback->libdecor_configuration_get_content_size(configuration, frame, &w, &h);
	LOG("libdecor.configuration(%i, %i)\n", w, h);
	if (w > 0)
		width_queued = w;
	if (h > 0)
		height_queued = h;
	if ((events_queued && event_resize) || (width_queued == width && height_queued == height)) {
		fallback->libdecor_frame_commit(frame, fallback->decor_state, configuration);
		return;
	}
	if (fallback->decor_state != nullptr)
		fallback->libdecor_state_free(fallback->decor_state);
	fallback->decor_state = fallback->libdecor_state_new(width_queued, height_queued);
	fallback->libdecor_frame_commit(frame, fallback->decor_state, configuration);
	events_queued = true;
	event_resize  = true;
	width         = width_queued;
	height        = height_queued;
	resize(width * scale, height * scale);
	draw(width * scale, height * scale);
}
void decor_frame_close([[maybe_unused]] struct libdecor_frame* frame, [[maybe_unused]] void* user_data) {
	events_queued = true;
	event_close   = true;
}
void decor_frame_commit([[maybe_unused]] struct libdecor_frame* frame, [[maybe_unused]] void* user_data) {
	LOG("libdecor.commit()\n");
	wl_surface_commit(surface);
}
void decor_frame_dismiss_popup([[maybe_unused]] struct libdecor_frame* frame, const char* seat_name,
                               [[maybe_unused]] void* user_data) {
	LOG("libdecor.dismiss_popup(%s)\n", seat_name);
}
} // namespace Engine
