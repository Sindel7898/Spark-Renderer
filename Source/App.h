#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include "Window.h"
#include "ShaderHelper.h"
#include "VkBootstrap.h"
#include <stdexcept>
#include <vulkan/vulkan.hpp>

constexpr unsigned int FRAME_OVERLAP = 2;

class App
{
public:

	void Initialisation();
	void create_swapchain();
	void Run();
	void MainLoop();
	void Draw();



	void CleanUp();

	// Vulkan Init Tasks
	void InitVulkan();
	void SelectGPU_CreateDevice();
	void createSurface();
	void CreateGraphicsPipeline();

	vk::ShaderModule createShaderModule(const std::vector<char>& code);

	void destroy_swapchain();
	void getqueue();

 private:

	 Window* window = nullptr;


#ifdef NDEBUG
	 const bool enableValidationLayers = false;
#else 
	 const bool enableValidationLayers = true;
#endif

     vk::Device                 LogicalDevice          = nullptr;
	 vk::PhysicalDevice         PhysicalDevice  = nullptr;
	 vk::Instance               VulkanInstance  = nullptr;
	 vk::SurfaceKHR             surface         = nullptr;
	 vk::SwapchainKHR           swapChain       = nullptr;
	 vk::DebugUtilsMessengerEXT Debug_Messenger = nullptr;


	 vk::Extent2D swapchainExtent      = { 0, 0 };

	 std::vector<VkImage>     swapchainImages     = {};
	 std::vector<VkImageView> swapchainImageViews = {};

	 vkb::Instance VKB_Instance;
	 vkb::Device   VKB_Device;

	 vk::Queue graphicsQueue;
	 uint32_t  graphicsQueueFamily;
};

