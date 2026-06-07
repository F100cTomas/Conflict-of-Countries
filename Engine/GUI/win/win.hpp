#pragma once
#include "coc.hpp"
#include <windows.h>
namespace Engine {
extern HBITMAP hBitmap;
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
}