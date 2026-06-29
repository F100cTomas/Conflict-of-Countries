#include "win.hpp"
#include "coc.hpp"
#include "common/common.hpp"
#include <cstdint>
#include <cstdio>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include <wchar.h>
#include <windows.h>
namespace Engine {
namespace {
constexpr const wchar_t CLASS_NAME[]      = L"ConflictOfCountries";
constexpr DWORD         MY_STYLE          = WS_OVERLAPPEDWINDOW;
constexpr DWORD         MY_EXTENDED_STYLE = WS_EX_APPWINDOW;
} // namespace
static bool initialized = false;
HINSTANCE   hInstance   = NULL;
int         nCmdShow    = SW_SHOW;
HBITMAP     hBitmap     = NULL;
uint32_t    old_width = 1920, old_height = 1080;
uint32_t    width = 1920, height = 1080;
void        error(const char* msg) {
	MessageBoxA(NULL, msg, "An error occured.", MB_ICONERROR | MB_OK);
	std::abort();
}
Engine::Engine() {
	if (initialized)
		error("Double initialization");
	initialized = true;
	init_vulkan();
	if (hInstance == NULL)
		error("hInstance is NULL");
	WNDCLASSEXW wc{};
	wc.cbSize        = sizeof(WNDCLASSEXW);
	wc.style         = 0;
	wc.lpfnWndProc   = WindowProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = NULL;
	wc.hCursor       = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = CLASS_NAME;
	wc.hIconSm       = NULL;
	if (RegisterClassExW(&wc) == 0)
		error("RegisterClassExW failed!");
	RECT rect = {0, 0, 1920, 1080};
	AdjustWindowRectEx(&rect, MY_STYLE, FALSE, MY_EXTENDED_STYLE);
	HWND hWnd =
	    CreateWindowExW(MY_EXTENDED_STYLE, CLASS_NAME, L"Conflict of Countries", MY_STYLE, CW_USEDEFAULT, CW_USEDEFAULT,
	                    rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hInstance, NULL);
	VkWin32SurfaceCreateInfoKHR win32_surface_info{};
	win32_surface_info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	win32_surface_info.hinstance = hInstance;
	win32_surface_info.hwnd      = hWnd;
	if (vkCreateWin32SurfaceKHR(instance, &win32_surface_info, nullptr, &surface_khr) != VK_SUCCESS)
		error("Failed to initialize Vulkan surface");
	resize(1920, 1080);
	ShowWindow(hWnd, nCmdShow);
}
Engine::~Engine() {
	deinit_vulkan();
	initialized = false;
}
void Engine::mainloop() {
	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	std::exit(0);
}
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_DESTROY: PostQuitMessage(0); return 0;
	case WM_PAINT: draw(old_width, old_height); return 0;
	case WM_SIZE: {
		width  = LOWORD(lParam);
		height = HIWORD(lParam);
		if (width == old_width || height == old_height)
			return 0;
		resize(width, height);
		old_width  = width;
		old_height = height;
	}
		return 0;
	default: break;
	}
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
} // namespace Engine
