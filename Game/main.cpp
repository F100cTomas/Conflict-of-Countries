#include "coc.hpp"
#ifdef _WIN32
#include <windows.h>
int WINAPI wWinMain(HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance,
                   [[maybe_unused]] LPWSTR lpCmdLine, int nCmdShow) {
	Engine::hInstance = hInstance;
	Engine::nCmdShow = nCmdShow;
#else
int main([[maybe_unused]] int argc, [[maybe_unused]] const char* const argv[]) {
#endif
	Engine::Engine engine{};
	engine.mainloop();
	return 0;
}
