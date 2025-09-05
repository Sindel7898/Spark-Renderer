#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#define GLM_FORCE_RADIANS

#include "ShaderHelper.h"
#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Lighting_FullScreenQuad.h"
#include "SSAO_FullScreenQuad.h"


class UserInterface;
class Window;
class Camera;
class MeshLoader;
class BufferManager;
class VulkanContext;
class FramesPerSecondCounter;
class Light;
class RT_Shadows;
class CombinedResult_FullScreenQuad;
class SSGI;
class SkyBox;
class Model;
class PipelineManager;
class SSR_FullScreenQuad;
class FXAA_FullScreenQuad;
class RT_Reflections;

struct GBuffer;

class App
{
public:

	std::chrono::time_point<std::chrono::high_resolution_clock> lastFrameTime;
	float deltaTime = 0.0f;
	double LasttimeStamp = 0.0f;
	
	App();
	void createTLAS();
	void UpdateTLAS();
	void UpdateTLASInstanceBuffer();
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

	uint32_t alignedSize(uint32_t value, uint32_t alignment);

	void createShaderBindingTable();

	void DestroyShaderBindingTable();

	void createCommandBuffer();

	void updateUniformBuffer(uint32_t currentImage);

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

	void destroy_DepthImage();

	void destroy_GbufferImages();

	bool framebufferResized = false;
	int DefferedDecider = 8;

	bool bWireFrame = false;
	//Drawables
	std::shared_ptr<Lighting_FullScreenQuad>            lighting_FullScreenQuad = nullptr;
	std::shared_ptr<SSA0_FullScreenQuad>                ssao_FullScreenQuad = nullptr;
	std::shared_ptr<FXAA_FullScreenQuad>                fxaa_FullScreenQuad = nullptr;
	std::shared_ptr<SSR_FullScreenQuad>                 ssr_FullScreenQuad = nullptr;
	std::shared_ptr<RT_Shadows>                         Raytracing_Shadows = nullptr;
	std::shared_ptr<RT_Reflections>                     Raytracing_Reflections = nullptr;
	std::shared_ptr<SSGI>                               SSGI_FullScreenQuad = nullptr;
	std::shared_ptr<CombinedResult_FullScreenQuad>      Combined_FullScreenQuad = nullptr;
	std::shared_ptr<PipelineManager>                    pipelineManager = nullptr;

	VkDescriptorSet FinalRenderTextureId;
	VkDescriptorSet LightingAndReflectionsRenderTextureId;
	VkDescriptorSet Shadow_TextureId;
	VkDescriptorSet Reflection_TextureId;
	VkDescriptorSet PositionRenderTextureId;
	VkDescriptorSet NormalTextureId;
	VkDescriptorSet AlbedoTextureId;
	VkDescriptorSet SSAOTextureId;
	VkDescriptorSet SSGITextureId;


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
	vk::PipelineLayout         SSRPipelineLayout = nullptr;
	vk::PipelineLayout         RT_ShadowsPipelineLayout = nullptr;
	vk::PipelineLayout         RT_ReflectionPipelineLayout = nullptr;
	vk::PipelineLayout         SSGIPipelineLayout = nullptr;
	vk::PipelineLayout         TA_SSGIPipelineLayout = nullptr;
	vk::PipelineLayout         BluredSSGIPipelineLayout = nullptr;
	vk::PipelineLayout         CombinedImagePipelineLayout = nullptr;

	vk::Pipeline               DeferedLightingPassPipeline = nullptr;
	vk::Pipeline               FXAAPassPipeline = nullptr;
	vk::Pipeline               LightgraphicsPipeline = nullptr;
	vk::Pipeline               SkyBoxgraphicsPipeline = nullptr;
	vk::Pipeline               geometryPassPipeline = nullptr;
	vk::Pipeline               SSAOPipeline = nullptr;
	vk::Pipeline               SSAOBlurPipeline = nullptr;
	vk::Pipeline               SSRPipeline = nullptr;
	vk::Pipeline               RT_ShadowsPassPipeline = nullptr;
	vk::Pipeline               RT_ReflectionPipeline = nullptr;
	vk::Pipeline               SSGIPipeline = nullptr;
	vk::Pipeline               TA_SSGIPipeline = nullptr;
	vk::Pipeline               BluredSSGIPipeline = nullptr;
	vk::Pipeline               CombinedImagePassPipeline = nullptr;


	vk::CommandPool            commandPool = nullptr;

	std::vector< vk::Semaphore> presentCompleteSemaphores;
	std::vector< vk::Semaphore> renderCompleteSemaphores;
	std::vector< vk::Fence> waitFences;

	std::vector< vk::CommandBuffer> commandBuffers;

	uint32_t currentFrame = 0;

	vk::DescriptorPool DescriptorPool;
	////////////////////////////
	ImageData DepthTextureData;

	bool bRecreateDepth = false; 

	bool binitiallayout = true;

	GBuffer gbuffer;

	ImageData LightingPassImageData;
	ImageData ReflectionMaskImageData;

	BufferData TLAS_Buffer;
	BufferData TLAS_SCRATCH_Buffer;
	BufferData TLAS_InstanceData;
	vk::AccelerationStructureKHR TLAS;

	BufferData raygenShaderBindingTableBuffer;
	BufferData missShaderBindingTableBuffer;
	BufferData hitShaderBindingTableBuffer;


	BufferData Reflection_raygenShaderBindingTableBuffer;
	BufferData Reflection_missShaderBindingTableBuffer;
	BufferData Reflection_hitShaderBindingTableBuffer;
};