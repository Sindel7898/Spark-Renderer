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


struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct Vertex {
	glm::vec2 position;
	glm::vec3 color;
	glm::vec2 texCoord;


	static vk::VertexInputBindingDescription GetBindingDescription() {
		vk::VertexInputBindingDescription  bindingdescription{};
		
		bindingdescription.binding = 0;
		bindingdescription.stride = sizeof(Vertex);
		bindingdescription.inputRate = vk::VertexInputRate::eVertex;

		return bindingdescription;
	}

	static std::array< vk::VertexInputAttributeDescription, 3> GetAttributeDescription() {

		std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions{};

		attributeDescriptions[0].binding  = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

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
	void createTextureImage();
	void createDescriptorSetLayout();
	void recreateSwapChain();
	void createVertexBuffer();
	void createIndexBuffer();

	void createUniformBuffer();

	void createDescriptorPool();

	void createDescriptorSets();

	void CopyBuffer(vk::Buffer Buffer1, vk::Buffer Buffer2, VkDeviceSize size);

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

	 const std::vector<Vertex> vertices = {

	   {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	   {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	   {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	   {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
	 
	 };

	 const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0
	 };


	 vk::DescriptorSetLayout DescriptorSetLayout;
	 vk::DescriptorPool DescriptorPool;
	 std::vector<vk::DescriptorSet> DescriptorSets;
	 vk::Buffer VertexBuffer;
	 vk::Buffer IndexBuffer;

	 std::vector<BufferData> uniformBuffers;
	 std::vector<void*> uniformBuffersMappedMem;

	 vk::Image TextureImage;
	 vk::ImageView TextureImageView;
	 vk::Sampler TextureSampler;

};

