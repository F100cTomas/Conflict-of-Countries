#include "common/common.hpp"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
void ExampleOSSpecificFunction() {
	MessageBoxW(NULL, L"Hello from Windows", L"Hello", MB_ICONINFORMATION);
}