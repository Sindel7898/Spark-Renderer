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
#include "BufferManager.h"
#include "VulkanContext.h"
#include"MeshLoader.h"
#include "Camera.h"
#include "Model.h"
#include "SkyBox.h"

class Model; 

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
	void CreateSkyboxGraphicsPipeline();

	void createCommandBuffer();

	void updateUniformBuffer(uint32_t currentImage);

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

	vk::ShaderModule createShaderModule(const std::vector<char>& code);

	void destroy_swapchain();
	void destroy_DepthImage();

	bool framebufferResized = false;
private:

	std::unique_ptr<Window> window = nullptr;
	std::shared_ptr<VulkanContext> vulkanContext = nullptr;
	std::shared_ptr<BufferManager> bufferManger = nullptr;
	std::unique_ptr<MeshLoader> meshloader = nullptr;
	std::shared_ptr<Camera> camera = nullptr;
	std::unique_ptr<Model, ModelDeleter> model = nullptr;
	std::unique_ptr<Model, ModelDeleter> model2 = nullptr;

	std::unique_ptr<SkyBox, SkyBoxDeleter> skyBox = nullptr;


#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else 
	const bool enableValidationLayers = true;
#endif

	vk::PipelineLayout         pipelineLayout = nullptr;
	vk::Pipeline               graphicsPipeline = nullptr;
	vk::Pipeline               SkyBoxgraphicsPipeline = nullptr;

	vk::CommandPool            commandPool = nullptr;

	vkb::Instance VKB_Instance;

	std::vector< vk::Semaphore> imageAvailableSemaphores;
	std::vector< vk::Semaphore> renderFinishedSemaphores;
	std::vector< vk::Fence> inFlightFences;
	std::vector< vk::CommandBuffer> commandBuffers;

	uint32_t currentFrame = 0;


	vk::DescriptorSetLayout DescriptorSetLayout;
	vk::DescriptorPool DescriptorPool;
	std::vector<vk::DescriptorSet> DescriptorSets;
	vk::Buffer VertexBuffer;
	vk::Buffer SkyBoxBuffer;

	vk::Buffer IndexBuffer;

	std::vector<BufferData> uniformBuffers;
	std::vector<void*> uniformBuffersMappedMem;



	//////////////////////////////
	ImageData CubeMapImageData;
	vk::Image CubeMapTextureImage;
	vk::ImageView CubeMapTextureImageView;
	vk::Sampler CubeMapTextureSampler;
	////////////////////////////
	ImageData DepthTextureData;
	vk::ImageView DepthImageView;


	BufferData VertexBufferData;
	BufferData SkyBoxVertexBufferData;

	BufferData IndexBufferData;


	
};