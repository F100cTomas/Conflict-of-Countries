#pragma once
#include <libdecor-0/libdecor.h>
namespace Engine {
class DecorationFallback {
	void* lib                                                                                              = nullptr;
	void (*libdecor_unref)(struct libdecor* context)                                                       = nullptr;
	struct libdecor* (*libdecor_new)(struct wl_display* display, struct libdecor_interface* iface)         = nullptr;
	int (*libdecor_dispatch)(struct libdecor* context, int timeout)                                        = nullptr;
	struct libdecor_frame* (*libdecor_decorate)(struct libdecor* context, struct wl_surface* surface,
	                                            struct libdecor_frame_interface* iface, void* user_data)   = nullptr;
	void (*libdecor_frame_unref)(struct libdecor_frame* frame)                                             = nullptr;
	void (*libdecor_frame_set_title)(struct libdecor_frame* frame, const char* title)                      = nullptr;
	void (*libdecor_frame_commit)(struct libdecor_frame* frame, struct libdecor_state* state,
	                              struct libdecor_configuration* configuration)                            = nullptr;
	void (*libdecor_frame_map)(struct libdecor_frame* frame)                                               = nullptr;
	struct libdecor_state* (*libdecor_state_new)(int width, int height)                                    = nullptr;
	void (*libdecor_state_free)(struct libdecor_state* state)                                              = nullptr;
	bool (*libdecor_configuration_get_content_size)(struct libdecor_configuration* configuration,
	                                                struct libdecor_frame* frame, int* width, int* height) = nullptr;
	// libdecor objects
	struct libdecor*       decor       = nullptr;
	struct libdecor_frame* decor_frame = nullptr;
	struct libdecor_state* decor_state = nullptr;

public:
	DecorationFallback();
	~DecorationFallback();
	inline void dispatch() {
		libdecor_dispatch(decor, 0);
	}

public:
	friend void decor_frame_configure(struct libdecor_frame* frame, struct libdecor_configuration* configuration,
	                                  void* user_data);
};
// libdecor
void                             decor_error(struct libdecor* context, enum libdecor_error error, const char* message);
inline struct libdecor_interface decor_listener = {.error     = decor_error,
                                                   .reserved0 = nullptr,
                                                   .reserved1 = nullptr,
                                                   .reserved2 = nullptr,
                                                   .reserved3 = nullptr,
                                                   .reserved4 = nullptr,
                                                   .reserved5 = nullptr,
                                                   .reserved6 = nullptr,
                                                   .reserved7 = nullptr,
                                                   .reserved8 = nullptr,
                                                   .reserved9 = nullptr};
// libdecor_frame
void decor_frame_configure(struct libdecor_frame* frame, struct libdecor_configuration* configuration, void* user_data);
void decor_frame_close(struct libdecor_frame* frame, void* user_data);
void decor_frame_commit(struct libdecor_frame* frame, void* user_data);
void decor_frame_dismiss_popup(struct libdecor_frame* frame, const char* seat_name, void* user_data);
inline struct libdecor_frame_interface decor_frame_listener = {.configure     = decor_frame_configure,
                                                               .close         = decor_frame_close,
                                                               .commit        = decor_frame_commit,
                                                               .dismiss_popup = decor_frame_dismiss_popup,
                                                               .reserved0     = nullptr,
                                                               .reserved1     = nullptr,
                                                               .reserved2     = nullptr,
                                                               .reserved3     = nullptr,
                                                               .reserved4     = nullptr,
                                                               .reserved5     = nullptr,
                                                               .reserved6     = nullptr,
                                                               .reserved7     = nullptr,
                                                               .reserved8     = nullptr,
                                                               .reserved9     = nullptr};
} // namespace Engine
