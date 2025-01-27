#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include "Window.h"
#include "vector"
#include <stdexcept>
#include <iostream>
#include <optional>
#include "VkBootstrap.h"

class App
{
public:

	void Initialisation();
	void Run();
	void MainLoop();
	void createSurface();

	void CleanUp();

	// Vulkan Init Tasks
	void InitVulkan();
	void create_swapchain(uint32_t width, uint32_t height);

 private:

	 Window window{ 800,800,"Vulkan Window" };


#ifdef NDEBUG
	 const bool enableValidationLayers = false;
#else 
	 const bool enableValidationLayers = true;
#endif

	 VkDevice device;
	 VkPhysicalDevice PhysicalDevice;

	 VkInstance Instance;
	 VkSurfaceKHR surface;

	 VkSwapchainKHR swapChain;
	 VkFormat swapchainImageFormat;
	 VkExtent2D swapchainExtent;

	 std::vector<VkImage> swapchainImages;
	 std::vector<VkImageView> swapchainImageViews;

	 //VK BootStrap
	 vkb::Instance VKB_Instance;
	 vkb::Device VKB_Device;
	 VkDebugUtilsMessengerEXT Debug_Messenger;
};

