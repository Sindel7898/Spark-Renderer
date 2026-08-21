#include "Window.h"
#include <stdexcept>
#include "stb_image.h"


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

    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);

    //window = glfwCreateWindow(Width, Height, WindowName.c_str(), nullptr, nullptr);
    Width = mode->width * 0.7;
    Height = mode->height * 0.7;

    window = glfwCreateWindow(Width, Height, WindowName.c_str(), nullptr, nullptr);

    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    int iconHeight = 0; 
    int iconWidth = 0;
    int texchannels = 0;

    unsigned char* pixels = stbi_load("../Textures/WindowLogo/PlaceHolder.JPG", &iconWidth, &iconHeight, &texchannels, STBI_rgb_alpha);

    if (pixels) {
        GLFWimage image;
        image.height = iconHeight;
        image.width  = iconWidth;
        image.pixels = pixels;

        glfwSetWindowIcon(window, 1, &image);
        stbi_image_free(pixels);
    }

#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(window);

    BOOL isTransparent = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &isTransparent, sizeof(isTransparent));

#endif
}


Window::~Window()
{
    CleanUp();
}

void Window::CleanUp()
{
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}
