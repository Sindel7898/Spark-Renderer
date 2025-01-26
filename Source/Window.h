#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <string>

class Window
{
public:
    Window(int W, int H, std::string WN);
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() { return glfwWindowShouldClose(window); };
    GLFWwindow* GetWindow() { return window; };

private:

    void initWindow();

    const int Width;
    const int Height;
    std::string WindowName;
    GLFWwindow* window;

};

