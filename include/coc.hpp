#pragma once
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
namespace Engine {
void windows_entry(HINSTANCE hInstance, int nCmdShow);
}
#endif
namespace Engine {
class Engine {
public:
	Engine();
	~Engine();
	void mainloop();
};
} // namespace Engine
