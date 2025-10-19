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
#include "FXAA_FullScreenQuad.h"
#include "SSR_FullScreenQuad.h"
#include "RT_Shadows.h"
#include "SSGI.h"
#include "CombinedResult_FullScreenQuad.h"

#include "Window.h"
#include "VulkanContext.h"
#include "BufferManager.h"
#include "Camera.h"
#include "UserInterface.h"
#include "Pipeline_Manager.h"
#include "RT_Reflections.h"
#include "DynamicDiffuse_RTGI.h"


class MeshLoader;
class FramesPerSecondCounter;
class Light;
class SkyBox;
class Model;
//class TracyVkCtx;

struct GBuffer;

class App
{
public:

	std::chrono::time_point<std::chrono::high_resolution_clock> lastFrameTime;
	float deltaTime = 0.0f;
	double LasttimeStamp = 0.0f;

	App();
	void CreateDebugUtils();
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
	int DefferedDecider = 9;

	bool bWireFrame = false;
	//Drawables
	std::unique_ptr<Lighting_FullScreenQuad, decltype(&Lighting_FullScreenQuadDeleter)>
		lighting_FullScreenQuad{ nullptr, &Lighting_FullScreenQuadDeleter };

	std::unique_ptr<SSA0_FullScreenQuad, decltype(&SSA0_FullScreenQuadDeleter)>
		ssao_FullScreenQuad{ nullptr, &SSA0_FullScreenQuadDeleter };

	std::unique_ptr<FXAA_FullScreenQuad, decltype(&FXAA_FullScreenQuadDeleter)>
		fxaa_FullScreenQuad{ nullptr, &FXAA_FullScreenQuadDeleter };

	std::unique_ptr<SSR_FullScreenQuad, decltype(&SSR_FullScreenQuadDeleter)>
		ssr_FullScreenQuad{ nullptr, &SSR_FullScreenQuadDeleter };

	std::unique_ptr<RT_Shadows, decltype(&RT_ShadowsDeleter)>
		Raytracing_Shadows{ nullptr, &RT_ShadowsDeleter };

	std::unique_ptr<SSGI, decltype(&SSGIDeleter)>
		SSGI_FullScreenQuad{ nullptr, &SSGIDeleter };

	std::unique_ptr<CombinedResult_FullScreenQuad, decltype(&CombinedResult_FullScreenQuadDeleter)>
		Combined_FullScreenQuad{ nullptr, &CombinedResult_FullScreenQuadDeleter };

	std::unique_ptr<RT_Reflections, decltype(&RT_ReflectionsDeleter)>
		RT_Reflection{ nullptr, &RT_ReflectionsDeleter };

	std::unique_ptr<DynamicDiffuse_RTGI, decltype(&DynamicDiffuse_RTGIDeleter)>
		dynamicDiffuse_RTGI { nullptr, &DynamicDiffuse_RTGIDeleter };


	VkDescriptorSet FinalRenderTextureId;
	VkDescriptorSet LightingAndReflectionsRenderTextureId;
	VkDescriptorSet Shadow_TextureId;
	VkDescriptorSet PositionRenderTextureId;
	VkDescriptorSet NormalTextureId;
	VkDescriptorSet AlbedoTextureId;
	VkDescriptorSet SSAOTextureId;
	VkDescriptorSet SSGITextureId;
	VkDescriptorSet RT_ReflectionTextureId;
	VkDescriptorSet RT_BluredReflectionTextureId;
	VkDescriptorSet DDGIIrradianceAtlasID;


	std::vector<std::shared_ptr<Model>> Models;
	std::vector<std::shared_ptr<Light>> lights;
	std::vector<Drawable*> UserInterfaceItems;

private:

	Window          window;
	VulkanContext   vulkanContext;
	BufferManager   bufferManger;
	UserInterface   userinterface;
	PipelineManager pipelineManager;
public:
	Camera camera;
private:

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
	vk::PipelineLayout         BluredRTreflectionsPipelineLayout = nullptr;
	vk::PipelineLayout         CombinedImagePipelineLayout = nullptr;
	vk::PipelineLayout         DDGIProbepipelineLayout = nullptr;
	vk::PipelineLayout         GridComputePipelineLayout = nullptr;
	vk::PipelineLayout         RT_DDGIPipelineLayout = nullptr;


	vk::Pipeline               DeferedLightingPassPipeline = nullptr;
	vk::Pipeline               FXAAPassPipeline = nullptr;
	vk::Pipeline               LightgraphicsPipeline = nullptr;
	vk::Pipeline               DDGIProbePipeline = nullptr;
	vk::Pipeline               SkyBoxgraphicsPipeline = nullptr;
	vk::Pipeline               geometryPassPipeline = nullptr;
	vk::Pipeline               SSAOPipeline = nullptr;
	vk::Pipeline               SSAOBlurPipeline = nullptr;
	vk::Pipeline               SSRPipeline = nullptr;
	vk::Pipeline               RT_ShadowsPassPipeline = nullptr;
	vk::Pipeline               RT_ReflectionPassPipeline = nullptr;
	vk::Pipeline               SSGIPipeline = nullptr;
	vk::Pipeline               TA_SSGIPipeline = nullptr;
	vk::Pipeline               BluredSSGIPipeline = nullptr;
	vk::Pipeline               BluredRTreflectionPipeline = nullptr;
	vk::Pipeline               CombinedImagePassPipeline = nullptr;
	vk::Pipeline               GridComputePassPipeline = nullptr;
	vk::Pipeline               RT_DDGIPassPipeline = nullptr;


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


	//RT Acceleration Structures
	BufferData TLAS_Buffer;
	BufferData TLAS_SCRATCH_Buffer;
	BufferData TLAS_InstanceData;
	vk::AccelerationStructureKHR TLAS;


	//RT Bind Tables
	BufferData Shadow_raygenShaderBindingTableBuffer;
	BufferData Shadow_missShaderBindingTableBuffer;
	BufferData Shadow_hitShaderBindingTableBuffer;

	BufferData Reflection_raygenShaderBindingTableBuffer;
	BufferData Reflection_missShaderBindingTableBuffer;
	BufferData Reflection_hitShaderBindingTableBuffer;

	BufferData	DDGI_raygenShaderBindingTableBuffer;
	BufferData	DDGI_missShaderBindingTableBuffer;
	BufferData	DDGI_hitShaderBindingTableBuffer;

	////DEBUGS
	vk::DebugUtilsLabelEXT Gbuffer_Label;
	vk::DebugUtilsLabelEXT SSAO_Label;
	vk::DebugUtilsLabelEXT RTShadows_Label;
	vk::DebugUtilsLabelEXT DirectLighting_Label;
	vk::DebugUtilsLabelEXT SSGI_Label;
	vk::DebugUtilsLabelEXT FXAA_Label;
	vk::DebugUtilsLabelEXT RTReflections_Label;


	///Tracy
	//TracyVkCtx tracyVkContext;
};