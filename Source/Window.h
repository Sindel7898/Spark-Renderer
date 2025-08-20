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
    bool shouldClose() { return glfwWindowShouldClose(window); };
    GLFWwindow* GetWindow() { return window; };

     void CleanUp();
     int GetWindowWidth()  { return Width; };
     int GetWindowHeight() { return Height; };

private:
    int Width;
    int Height;
    std::string WindowName;
    GLFWwindow* window;
};

static inline void WindowDeleter(Window* window) {
    if (window) {
        window->CleanUp();
        delete window;
    }
};