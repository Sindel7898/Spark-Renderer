#include "Window.h"
#include <stdexcept>


#ifdef _WIN32
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")
#endif
class App;

Window::Window(int W, int H, std::string WN) : Width(W),Height(H),WindowName(WN)
{

    if (!glfwInit()) {

        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(Width, Height, WindowName.c_str(), nullptr, nullptr);

#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(window);

    BOOL isTransparent = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &isTransparent, sizeof(isTransparent));

#endif

    if (!window) {

        throw std::runtime_error("Failed to create GLFW window");
        delete this;
    }
}


Window::~Window()
{
	if (window) {
		glfwDestroyWindow(window);
	}
	glfwTerminate();
}
