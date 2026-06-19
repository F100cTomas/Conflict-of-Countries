#pragma once
#ifdef _WIN32
#include <windows.h>
namespace Engine {
extern HINSTANCE hInstance;
extern int       nCmdShow;
} // namespace Engine
#endif
namespace Engine {
void error(const char* msg);
class Engine {
public:
	Engine();
	~Engine();
	void mainloop();
};
} // namespace Engine
