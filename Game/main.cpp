#include "common.hpp"
#include <cstdio>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
int WINAPI WinMain([[maybe_unused]] HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance,
                   [[maybe_unused]] LPSTR lpCmdLine, [[maybe_unused]] int nCmdShow) {
#else
int main([[maybe_unused]] int argc, [[maybe_unused]] const char* const argv[]) {
#endif
	std::printf("Hello World!\n");
	return 0;
}
