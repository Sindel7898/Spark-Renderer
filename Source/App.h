#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#define GLM_FORCE_RADIANS

#include "Window.h"
#include "ShaderHelper.h"
#include "VkBootstrap.h"
#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include "BufferManager.h"
#include "VulkanContext.h"
#include"MeshLoader.h"


struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

class App
{
public:

	std::chrono::time_point<std::chrono::high_resolution_clock> lastFrameTime;
	float deltaTime = 0.0f;
	float fps = 0.0f;

	void Initialisation();
	void createTextureImage();
	void createDepthTextureImage();
	void createDescriptorSetLayout();
	void recreateSwapChain();
	void createVertexBuffer();
	void createIndexBuffer();

	void createUniformBuffer();

	void createDescriptorPool();

	void createDescriptorSets();

	void createCommandPool();

	void Run();
	void Draw();
	void StartFrame();

	void createSyncObjects();



	void DestroySyncObjects();

	void DestroyBuffers();


	void CleanUp();

	void SwapchainResizeCallback(GLFWwindow* window, int width, int height);

	void CreateGraphicsPipeline();

	void createCommandBuffer();

	void updateUniformBuffer(uint32_t currentImage);

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

	vk::ShaderModule createShaderModule(const std::vector<char>& code);

	void destroy_swapchain();


	bool framebufferResized = false;
 private:

	 std::unique_ptr<Window> window = nullptr;
	 std::unique_ptr<VulkanContext> vulkanContext = nullptr;
	 std::unique_ptr<BufferManager> bufferManger = nullptr;
	 std::unique_ptr<MeshLoader> meshloader = nullptr;


#ifdef NDEBUG
	 const bool enableValidationLayers = false;
#else 
	 const bool enableValidationLayers = true;
#endif

	 vk::PipelineLayout         pipelineLayout  = nullptr;
	 vk::Pipeline               graphicsPipeline = nullptr;
	 vk::CommandPool            commandPool      = nullptr;

	 vkb::Instance VKB_Instance;

	 std::vector< vk::Semaphore> imageAvailableSemaphores;
	 std::vector< vk::Semaphore> renderFinishedSemaphores;
	 std::vector< vk::Fence> inFlightFences;
	 std::vector< vk::CommandBuffer> commandBuffers;

	 const int MAX_FRAMES_IN_FLIGHT = 2;
	 uint32_t currentFrame = 0;

	// const std::vector<Vertex> vertices = {
 //  {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	//{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	//{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	//{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

	//{{-0.5f, -0.5f, -0.2f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	//{{0.5f, -0.5f, -0.2f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	//{{0.5f, 0.5f, -0.2f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	//{{-0.5f, 0.5f, -0.2f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	// 
	// };

	/* const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	 };*/


	 vk::DescriptorSetLayout DescriptorSetLayout;
	 vk::DescriptorPool DescriptorPool;
	 std::vector<vk::DescriptorSet> DescriptorSets;
	 vk::Buffer VertexBuffer;
	 vk::Buffer IndexBuffer;

	 std::vector<BufferData> uniformBuffers;
	 std::vector<void*> uniformBuffersMappedMem;

	
	 ImageData MeshTextureData;
	 vk::Image TextureImage;
	 vk::ImageView TextureImageView;
	 vk::Sampler TextureSampler;
     ////////////////////////////
	 ImageData DepthTextureData;
	 vk::ImageView DepthImageView;


	 BufferData VertexBufferData;
	 BufferData IndexBufferData;

};

