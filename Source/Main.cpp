#include <iostream>
#include "App.h"

int main() {
   
    App VulkanApp;
    VulkanApp.InitVulkan();
    VulkanApp.Run();

    return 0;
}