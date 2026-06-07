#include "win.hpp"
#include "coc.hpp"
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <wchar.h>
namespace Engine {
namespace {
constexpr const wchar_t CLASS_NAME[]      = L"ConflictOfCountries";
constexpr DWORD         MY_STYLE          = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
constexpr DWORD         MY_EXTENDED_STYLE = WS_EX_APPWINDOW;
} // namespace
HINSTANCE      hInstance = NULL;
int            nCmdShow  = SW_SHOW;
HBITMAP        hBitmap   = NULL;
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
Engine::Engine() {
	if (hInstance == NULL) {
		std::fprintf(stderr, "hInstance is NULL\n");
		std::abort();
	}
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
	if(RegisterClassExW(&wc) == 0) {
		std::fprintf(stderr, "RegisterClassExW failed!\n");
		std::abort();
	}
	RECT rect = {0, 0, 1920, 1080};
	AdjustWindowRectEx(&rect, MY_STYLE, FALSE, MY_EXTENDED_STYLE);
	HWND hWnd =
	    CreateWindowExW(MY_EXTENDED_STYLE, CLASS_NAME, L"Conflict of Countries", MY_STYLE, CW_USEDEFAULT,
	                    CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	BITMAPINFO bmi{};
	bmi.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth         = 1920;
	bmi.bmiHeader.biHeight        = -1080;
	bmi.bmiHeader.biPlanes        = 1;
	bmi.bmiHeader.biBitCount      = 32;
	bmi.bmiHeader.biCompression   = BI_RGB;
	bmi.bmiHeader.biSizeImage     = 0;
	bmi.bmiHeader.biXPelsPerMeter = 0;
	bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmiHeader.biClrUsed       = 0;
	bmi.bmiHeader.biClrImportant  = 0;
	uint32_t* raw;
	hBitmap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&raw), NULL, 0);
	for (uint32_t y = 0; y < 1080; y++)
		for (uint32_t x = 0; x < 1920; x++)
			raw[1920 * y + x] = line(x, y) ? 0x00000000 : 0x00FFFFFF;
}
Engine::~Engine() {}
void Engine::mainloop() {
	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_DESTROY: PostQuitMessage(0); return 0;
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC         hdc = BeginPaint(hwnd, &ps);
		HDC         bm  = CreateCompatibleDC(hdc);
		HGDIOBJ     old = SelectObject(bm, hBitmap);
		BitBlt(hdc, 0, 0, 1920, 1080, bm, 0, 0, SRCCOPY);
		SelectObject(bm, old);
		DeleteDC(bm);
		EndPaint(hwnd, &ps);
	}
		return 0;
	default: break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
} // namespace Engine
