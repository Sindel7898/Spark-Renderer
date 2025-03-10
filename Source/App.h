#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include "Window.h"
#include "ShaderHelper.h"
#include "VkBootstrap.h"
#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <chrono>

class App
{
public:

	std::chrono::time_point<std::chrono::high_resolution_clock> lastFrameTime;
	float deltaTime = 0.0f;
	float fps = 0.0f;

	void Initialisation();
	void CreateRenderPass();
	void create_swapchain();
	void CreateFramebuffers();

	void createCommandPool();

	void Run();
	void Draw();
	void StartFrame();

	void createSyncObjects();



	void CleanUp();

	// Vulkan Init Tasks
	void InitVulkan();
	void SelectGPU_CreateDevice();
	void createSurface();
	void CreateGraphicsPipeline();

	void createCommandBuffer();

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

	vk::ShaderModule createShaderModule(const std::vector<char>& code);

	void destroy_swapchain();
	void destroy_frameBuffers();


 private:

	 Window* window = nullptr;


#ifdef NDEBUG
	 const bool enableValidationLayers = false;
#else 
	 const bool enableValidationLayers = true;
#endif

     vk::Device                 LogicalDevice   = nullptr;
	 vk::PhysicalDevice         PhysicalDevice  = nullptr;
	 vk::Instance               VulkanInstance  = nullptr;
	 vk::SurfaceKHR             surface         = nullptr;
	 vk::SwapchainKHR           swapChain       = nullptr;
	 vk::DebugUtilsMessengerEXT Debug_Messenger = nullptr;
	 vk::RenderPass             renderPass      = nullptr;
	 vk::PipelineLayout         pipelineLayout  = nullptr;
	 vk::Pipeline              graphicsPipeline = nullptr;
	 vk::CommandPool           commandPool      = nullptr;
	 vk::CommandBuffer            commandBuffer = nullptr;

	 vk::Format                 swapchainformat;

	 vk::Extent2D swapchainExtent      = { 0, 0 };

	 std::vector<VkImage>     swapchainImages     = {};
	 std::vector<VkImageView> swapchainImageViews = {};
	 std::vector<VkFramebuffer> swapChainFramebuffers;

	 vkb::Instance VKB_Instance;

	 vk::Queue graphicsQueue;
	 uint32_t  graphicsQueueFamilyIndex;

	 vk::Queue presentQueue;

	 //uint32_t  graphicsQueueFamily;



	 vk::Semaphore imageAvailableSemaphore;
	 vk::Semaphore renderFinishedSemaphore;
	 vk::Fence inFlightFence;
};

