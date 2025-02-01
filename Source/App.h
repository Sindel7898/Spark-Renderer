#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include "Window.h"
#include "vector"
#include "VkBootstrap.h"
#include <stdexcept>
#include <iostream>

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
	void destroy_swapchain();

 private:

	 Window* window = nullptr;


#ifdef NDEBUG
	 const bool enableValidationLayers = false;
#else 
	 const bool enableValidationLayers = true;
#endif

	 VkDevice                 device          = nullptr;
	 VkPhysicalDevice         PhysicalDevice  = nullptr;
	 VkInstance               Instance        = nullptr;
	 VkSurfaceKHR             surface         = nullptr;
	 VkSwapchainKHR           swapChain       = nullptr;
	 VkDebugUtilsMessengerEXT Debug_Messenger = nullptr;


	 VkFormat   swapchainImageFormat = VK_FORMAT_UNDEFINED;
	 VkExtent2D swapchainExtent      = { 0, 0 };

	 std::vector<VkImage>     swapchainImages     = {};
	 std::vector<VkImageView> swapchainImageViews = {};

	 vkb::Instance VKB_Instance;

};

