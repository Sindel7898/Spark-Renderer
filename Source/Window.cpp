#include "Window.h"

Window::Window(int W, int H, std::string WN) : Width(W),Height(H),WindowName(WN)
{
	initWindow();
}

Window::~Window()
{
	glfwDestroyWindow(window);
	glfwTerminate();

}

void Window::initWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(Width, Height, WindowName.c_str(), nullptr, nullptr);
}
