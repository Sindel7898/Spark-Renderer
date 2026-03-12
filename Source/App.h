#pragma once
#define GLM_FORCE_RADIANS

#include "ShaderHelper.h"
#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Lighting_RTX.h"
#include "SSAO_FullScreenQuad.h"
#include "FXAA_FullScreenQuad.h"
#include "SSR_FullScreenQuad.h"
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
#include "ReSTIR_DI.h"


#define TRACY_ENABLE
#include "TracyVulkan.hpp"

class MeshLoader;
class FramesPerSecondCounter;
class Light;
class SkyBox;
class Model;
class NvdiaDLSS_Intergration;



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

	void DestroyTLAS();

	void DestroyBuffers();

	void SwapchainResizeCallback(GLFWwindow* window, int width, int height);

	void createGBuffer();

	void UpdateTextureID();

	void CreateGraphicsPipeline();

	uint32_t alignedSize(uint32_t value, uint32_t alignment);

	void createShaderBindingTable();

	void DestroyShaderBindingTable();

	void createCommandBuffer();

	void updateUniformBuffer(uint32_t currentImage);

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

	void destroy_DepthImage();

	void destroy_GbufferImages();

	void LoadAllObjects();
	void UpdateRayTracingDescriptors();
	void SpawnLights(int NumOfLights);
	void SwitchScene(int Index);

	int currentSceneIndex = 2;
	std::vector<std::string> SceneNames = { "Cornell", "Sponza" ,"Alt Cornell" };

	bool framebufferResized = false;
	int DefferedDecider = 3;

	bool bWireFrame = false;
	bool bUseDLSS = false;

	//Drawables
	std::unique_ptr<Lighting_RTX>                  lighting_RTX;
	std::unique_ptr<SSA0_FullScreenQuad>           ssao_FullScreenQuad;
	std::unique_ptr<FXAA_FullScreenQuad>           fxaa_FullScreenQuad;
	std::unique_ptr<SSGI>                          SSGI_FullScreenQuad;
	std::unique_ptr<CombinedResult_FullScreenQuad> Combined_FullScreenQuad;
	std::unique_ptr<DynamicDiffuse_RTGI>           dynamicDiffuse_RTGI;
	std::unique_ptr<ReSTIR_DI>                     Restir_DI;


	VkDescriptorSet FinalRenderTextureId;
	VkDescriptorSet SSGITextureId;
	VkDescriptorSet ReSTIR_DITextureId;
	VkDescriptorSet DDGI_Radiance;
	VkDescriptorSet DDGIIrradianceAtlasID;
	VkDescriptorSet Sampled_GI_ID;
	VkDescriptorSet DDGIIVisibilityAtlasID;


	std::vector<std::shared_ptr<Model>> SponzaSceneModels;
	std::vector<std::shared_ptr<Model>> CornelSceneModels;
	std::vector<std::shared_ptr<Model>> AltCornelSceneModels;

	std::vector<Model*> Models;
	std::vector<std::unique_ptr<Light>> lights;
	std::vector<Drawable*> UserInterfaceItems;

	NvdiaDLSS_Intergration DLSS_Intergration;
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
	vk::PipelineLayout         BluredRTreflectionsPipelineLayout = nullptr;
	vk::PipelineLayout         CombinedImagePipelineLayout = nullptr;
	vk::PipelineLayout         Gamma_Corrected_IMGUI_PipelineLayout = nullptr;
	vk::PipelineLayout         DDGIProbepipelineLayout = nullptr;
	vk::PipelineLayout         GridComputePipelineLayout = nullptr;
	vk::PipelineLayout         RT_DDGIPipelineLayout = nullptr;
	vk::PipelineLayout         IrradianceComputePipelineLayout = nullptr;
	vk::PipelineLayout         SampleDDGIComputePipelineLayout = nullptr;
	vk::PipelineLayout         ProbeStatusPipelineLayout = nullptr;

	vk::PipelineLayout         ReSTIR_Temporal_RT_PipelineLayout = nullptr;
	vk::PipelineLayout         ReSTIR_Spatial_RT_PipelineLayout = nullptr;



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
	vk::Pipeline               BluredRTreflectionPipeline = nullptr;
	vk::Pipeline               CombinedImagePassPipeline = nullptr;
	vk::Pipeline               Gamma_Corrected_IMGUI_PassPipeline = nullptr;
	vk::Pipeline               GridComputePassPipeline = nullptr;
	vk::Pipeline               RT_DDGIPassPipeline = nullptr;
	vk::Pipeline               IrradianceComputePassPipeline = nullptr;
	vk::Pipeline               SampleDDGIComputePassPipeline = nullptr;
	vk::Pipeline               ProbeStatusComputePassPipeline = nullptr;
	vk::Pipeline               ReSTIR_Temporal_RT_PassPipeline = nullptr;
	vk::Pipeline               ReSTIR_SPATIAL_RT_PassPipeline = nullptr;


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

	//RT Acceleration Structures
	BufferData TLAS_Buffer;
	BufferData TLAS_SCRATCH_Buffer;
	BufferData TLAS_InstanceData;
	vk::AccelerationStructureKHR TLAS;


	//RT Bind Tables

	BufferData Lighting_raygenShaderBindingTableBuffer;
	BufferData Lighting_missShaderBindingTableBuffer;
	BufferData Lighting_hitShaderBindingTableBuffer;


	BufferData	DDGI_raygenShaderBindingTableBuffer;
	BufferData	DDGI_missShaderBindingTableBuffer;
	BufferData	DDGI_hitShaderBindingTableBuffer;


	BufferData	ReSTIR_DI_Temporal_raygenShaderBindingTableBuffer;
	BufferData	ReSTIR_DI_Temporal_missShaderBindingTableBuffer;
	BufferData	ReSTIR_DI_Temporal_hitShaderBindingTableBuffer;

	BufferData	ReSTIR_DI_Spatial_raygenShaderBindingTableBuffer;
	BufferData	ReSTIR_DI_Spatial_missShaderBindingTableBuffer;
	BufferData	ReSTIR_DI_Spatial_hitShaderBindingTableBuffer;

	////DEBUGS
	vk::DebugUtilsLabelEXT Gbuffer_Label;
	vk::DebugUtilsLabelEXT SSAO_Label;
	vk::DebugUtilsLabelEXT RTShadows_Label;
	vk::DebugUtilsLabelEXT DirectLighting_Label;
	vk::DebugUtilsLabelEXT SSGI_Label;
	vk::DebugUtilsLabelEXT FXAA_Label;
	vk::DebugUtilsLabelEXT RTReflections_Label;
	vk::DebugUtilsLabelEXT DDGI_Grid_Generation_Label;
	vk::DebugUtilsLabelEXT DDGI_Trace_Ray_Label;
	vk::DebugUtilsLabelEXT DDGI_Directions_Generation_Label;
	vk::DebugUtilsLabelEXT DDGI_Calculate_Irradiance_Label;
	vk::DebugUtilsLabelEXT DDGI_Update_Probe_Status_Label;
	vk::DebugUtilsLabelEXT DDGI_Sample_From_PorbeLabel;
	vk::DebugUtilsLabelEXT ReSTIR_Label;
	vk::DebugUtilsLabelEXT RayReconstruction;

	///Tracy
	//TracyVkCtx tracyVkContext;

	float TESTTIMER = 0;
	int   currentTestIndex = 0;
};