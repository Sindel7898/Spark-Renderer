#include <iostream>
#include "App.h"

int main() {
    App VulkanApp;
    try {
        VulkanApp.Initialisation();
        VulkanApp.Run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    VulkanApp.CleanUp(); // Ensure CleanUp is called
    return EXIT_SUCCESS;
}