#include "Window.h"
#include <stdexcept>

Window::Window(int W, int H, std::string WN) : Width(W),Height(H),WindowName(WN)
{

    if (!glfwInit()) {

        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(Width, Height, WindowName.c_str(), nullptr, nullptr);

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
