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
	void recreateSwapChain();
	void create_swapchain();
	void CreateFramebuffers();

	void createCommandPool();

	void Run();
	void Draw();
	void StartFrame();

	void createSyncObjects();



	void DestroySyncObjects();

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

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

	bool framebufferResized = false;
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

	 vk::Format                 swapchainformat;

	 vk::Extent2D swapchainExtent      = { 0, 0 };

	 std::vector<VkImage>     swapchainImages     = {};
	 std::vector<VkImageView> swapchainImageViews = {};
	 std::vector<VkFramebuffer> swapChainFramebuffers;

	 vkb::Instance VKB_Instance;

	 vk::Queue graphicsQueue;
	 uint32_t  graphicsQueueFamilyIndex;

	 vk::Queue presentQueue;

	 std::vector< vk::Semaphore> imageAvailableSemaphores;
	 std::vector< vk::Semaphore> renderFinishedSemaphores;
	 std::vector< vk::Fence> inFlightFences;
	 std::vector< vk::CommandBuffer> commandBuffers;

	 const int MAX_FRAMES_IN_FLIGHT = 2;
	 uint32_t currentFrame = 0;

};

