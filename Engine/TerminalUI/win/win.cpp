#include "common/common.hpp"
#include <windows.h>
void ExampleOSSpecificFunction() {
	MessageBoxW(NULL, L"Hello from Windows", L"Hello", MB_ICONINFORMATION);
}