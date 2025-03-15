#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include "Window.h"
#include "ShaderHelper.h"
#include "VkBootstrap.h"
#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <chrono>
#include <glm/glm.hpp>

struct Vertex {
	glm::vec2 position;
	glm::vec3 color;


	static vk::VertexInputBindingDescription GetBindingDescription() {
		vk::VertexInputBindingDescription  bindingdescription{};
		
		bindingdescription.binding = 0;
		bindingdescription.stride = sizeof(Vertex);
		bindingdescription.inputRate = vk::VertexInputRate::eVertex;

		return bindingdescription;
	}

	static std::array< vk::VertexInputAttributeDescription, 2> GetAttributeDescription() {

		std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding  = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
};

class App
{
public:

	std::chrono::time_point<std::chrono::high_resolution_clock> lastFrameTime;
	float deltaTime = 0.0f;
	float fps = 0.0f;

	void Initialisation();
	void InitMemAllocator();

	void recreateSwapChain();
	void create_swapchain();
	void createVertexBuffer();
	void createIndexBuffer();

	void CopyBuffer(vk::Buffer Buffer1, vk::Buffer Buffer2, VkDeviceSize size);

	void createCommandPool();

	void Run();
	void Draw();
	void StartFrame();

	void createSyncObjects();



	void DestroySyncObjects();

	void CleanUp();

	void SwapchainResizeCallback(GLFWwindow* window, int width, int height);

	// Vulkan Init Tasks
	void InitVulkan();
	void SelectGPU_CreateDevice();
	void createSurface();
	void CreateGraphicsPipeline();

	void createCommandBuffer();

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

	vk::ShaderModule createShaderModule(const std::vector<char>& code);

	void destroy_swapchain();


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

	 const std::vector<Vertex> vertices = {

		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	 
	 };

	 const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0
	 };

	 vk::Buffer VertexBuffer;
	 vk::Buffer IndexBuffer;

};

