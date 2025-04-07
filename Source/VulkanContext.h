#pragma once
#include "VkBootstrap.h"
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <GLFW/glfw3.h>
#include "../Source/Window.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

class VulkanContext
{
public:

	VulkanContext(Window& Window);

	void InitVulkan();
	void SelectGPU_CreateDevice();
	void createSurface();
	void create_swapchain();
	vk::Format FindCompatableDepthFormat();

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else 
	const bool enableValidationLayers = true;
#endif

	vkb::Instance VKB_Instance;

	vk::Device                 LogicalDevice = nullptr;
	vk::PhysicalDevice         PhysicalDevice = nullptr;
	vk::Instance               VulkanInstance = nullptr;
	vk::DebugUtilsMessengerEXT Debug_Messenger = nullptr;
	vk::SurfaceKHR             surface = nullptr;

	vk::Queue                  graphicsQueue;
	vk::Queue                  presentQueue;
	uint32_t                   graphicsQueueFamilyIndex;

	vk::Format                 swapchainformat;
	vk::Extent2D               swapchainExtent = { 0, 0 };
	vk::SwapchainKHR           swapChain = nullptr;
	std::vector<VkImage>       swapchainImages = {};
	std::vector<VkImageView>   swapchainImageViews = {};
	std::vector<vk::SurfaceFormatKHR> SurfaceFormat;

	Window window; 

};

