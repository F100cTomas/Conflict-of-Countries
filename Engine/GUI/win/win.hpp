#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
namespace Engine {
extern HINSTANCE hInstance;
extern int nCmdShow;
extern HBITMAP hBitmap;
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
}