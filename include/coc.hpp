#pragma once
#ifdef _WIN32
#include <windows.h>
namespace Engine {
extern HINSTANCE hInstance;
extern int nCmdShow;
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
