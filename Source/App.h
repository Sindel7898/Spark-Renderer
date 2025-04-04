#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#define GLM_FORCE_RADIANS

#include "ShaderHelper.h"
#include "VkBootstrap.h"
#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Model.h"
#include "SkyBox.h"

class Window;
class Camera;
class MeshLoader;
class BufferManager;
class VulkanContext;
class FramesPerSecondCounter;

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
	double LasttimeStamp = 0.0f;

	void Initialisation();

	void createDepthTextureImage();
	void createDescriptorSetLayout();
	void recreateSwapChain();


	void createDescriptorPool();


	void createCommandPool();

	void Run();
	void CalculateFps(FramesPerSecondCounter& fpsCounter);
	void Draw();

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
	std::unique_ptr<Model, ModelDeleter> model3 = nullptr;
	std::unique_ptr<Model, ModelDeleter> model4 = nullptr;
	std::unique_ptr<Model, ModelDeleter> model5 = nullptr;
	std::unique_ptr<Model, ModelDeleter> model6= nullptr;
	std::unique_ptr<Model, ModelDeleter> model7 = nullptr;
	std::unique_ptr<Model, ModelDeleter> model8 = nullptr;

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


	//vk::DescriptorSetLayout DescriptorSetLayout;
	vk::DescriptorPool DescriptorPool;
	////////////////////////////
	ImageData DepthTextureData;
	vk::ImageView DepthImageView;	
};