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
#include "UserInterface.h"
#include "Lighting_FullScreenQuad.h"
#include "SSAO_FullScreenQuad.h"
#include "SSAOBlur_FullScreenQuad.h"
#include "FXAA_FullScreenQuad.h"
#include "Terrain.h"


class Window;
class Camera;
class MeshLoader;
class BufferManager;
class VulkanContext;
class FramesPerSecondCounter;
class Light;
struct GBuffer;

class App
{
public:

	std::chrono::time_point<std::chrono::high_resolution_clock> lastFrameTime;
	float deltaTime = 0.0f;
	double LasttimeStamp = 0.0f;
	
	App();
	~App();

	

	void createDepthTextureImage();
	void recreateSwapChain();
	void recreatePipeline();
	void destroyPipeline();



	void createDescriptorPool();


	void createCommandPool();

	void Run();

	void CalculateFps(FramesPerSecondCounter& fpsCounter);
	void Draw();

	void createSyncObjects();



	void DestroySyncObjects();

	void DestroyBuffers();

	void SwapchainResizeCallback(GLFWwindow* window, int width, int height);

	void createGBuffer();

	void CreateGraphicsPipeline();

	void createCommandBuffer();

	void updateUniformBuffer(uint32_t currentImage);

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

	vk::ShaderModule createShaderModule(const std::vector<char>& code);

	void destroy_DepthImage();

	void destroy_GbufferImages();

	bool framebufferResized = false;
	int DefferedDecider = 4;

	bool bWireFrame = false;
	//Drawables
	std::shared_ptr<Lighting_FullScreenQuad>     lighting_FullScreenQuad = nullptr;
	std::shared_ptr<SSA0_FullScreenQuad>         ssao_FullScreenQuad = nullptr;
	std::shared_ptr<SSAOBlur_FullScreenQuad>     ssaoBlur_FullScreenQuad = nullptr;
	std::shared_ptr<FXAA_FullScreenQuad>         fxaa_FullScreenQuad = nullptr;
	std::shared_ptr<Terrain>                     terrain = nullptr;
	VkDescriptorSet FinalRenderTextureId;
	VkDescriptorSet PositionRenderTextureId;
	VkDescriptorSet NormalTextureId;
	VkDescriptorSet AlbedoTextureId;
	VkDescriptorSet SSAOTextureId;

	std::shared_ptr<Camera>             camera = nullptr;
	std::vector<std::shared_ptr<Model>> Models;
	std::vector<std::shared_ptr<Light>> lights;
	std::vector<Drawable*> UserInterfaceItems;


private:

	std::shared_ptr<Window>             window = nullptr;
	std::shared_ptr<VulkanContext>      vulkanContext = nullptr;
	std::shared_ptr<BufferManager>      bufferManger = nullptr;
    std::shared_ptr<UserInterface>      userinterface = nullptr;
	
	std::shared_ptr<SkyBox> skyBox = nullptr;


#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else 
	const bool enableValidationLayers = true;
#endif

	vk::PipelineLayout         DeferedLightingPassPipelineLayout = nullptr;
	vk::PipelineLayout         FXAAPassPipelineLayout = nullptr;
	vk::PipelineLayout         LightpipelineLayout = nullptr;
	vk::PipelineLayout         SkyBoxpipelineLayout = nullptr;
	vk::PipelineLayout         geometryPassPipelineLayout = nullptr;
	vk::PipelineLayout         SSAOPipelineLayout = nullptr;
	vk::PipelineLayout         SSAOBlurPipelineLayout = nullptr;
	vk::PipelineLayout         TerrainGeometryPassPipelineLayout = nullptr;

	vk::Pipeline               DeferedLightingPassPipeline = nullptr;
	vk::Pipeline               FXAAPassPipeline = nullptr;
	vk::Pipeline               LightgraphicsPipeline = nullptr;
	vk::Pipeline               SkyBoxgraphicsPipeline = nullptr;
	vk::Pipeline               geometryPassPipeline = nullptr;
	vk::Pipeline               SSAOPipeline = nullptr;
	vk::Pipeline               SSAOBlurPipeline = nullptr;
	vk::Pipeline               TerrainGeometryPassPipeline = nullptr;

	vk::CommandPool            commandPool = nullptr;

	std::vector< vk::Semaphore> imageAvailableSemaphores;
	std::vector< vk::Semaphore> renderFinishedSemaphores;
	std::vector< vk::Fence> inFlightFences;
	std::vector< vk::CommandBuffer> commandBuffers;

	uint32_t currentFrame = 0;

	vk::DescriptorPool DescriptorPool;
	////////////////////////////
	ImageData DepthTextureData;



	bool bRecreateDepth = false; 

	bool binitiallayout = true;

	GBuffer gbuffer;

	ImageData LightingPassImageData;


};