#define _CRTDBG_MAP_ALLOC
#include "App.h"
#include <optional>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include "Window.h"
#include "Camera.h"
#include "BufferManager.h"
#include "VulkanContext.h"
#include "FramesPerSecondCounter.h"
#include "Light.h"
#include "CombinedResult_FullScreenQuad.h"
#include "SSGI.h"
#include <crtdbg.h>
#include "SkyBox.h"
#include "Model.h"
#include "UserInterface.h"
#include "Pipeline_Manager.h"
#include "FXAA_FullScreenQuad.h"
#include "SSR_FullScreenQuad.h"
#include "SSAO_FullScreenQuad.h"
#include "Lighting_RTX.h"
#include <pix.h>
#include "Tracy.hpp"
#include <random>

#define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)


App::App() : window(1920, 1080, "Spark Renderer"),
vulkanContext(window),
bufferManger(&vulkanContext),
camera(vulkanContext.swapchainExtent.width, vulkanContext.swapchainExtent.height, window.GetWindow()),
userinterface(&vulkanContext, &window, &bufferManger), pipelineManager(&vulkanContext)
{
	glfwSetWindowUserPointer(window.GetWindow(), this);

	createSyncObjects();
	createCommandPool();


	skyBox = std::shared_ptr<SkyBox>(new SkyBox(&vulkanContext, commandPool, &camera, &bufferManger), SkyBoxDeleter);

	createDescriptorPool();

	LoadAllObjects();

	bufferManger.CreateSharedBuffers(commandPool);

	SwitchScene(0);

	RT_Reflection = std::unique_ptr<RT_Reflections, decltype(&RT_ReflectionsDeleter)>(new RT_Reflections(&vulkanContext, commandPool, &camera, &bufferManger, skyBox.get()), RT_ReflectionsDeleter);
	lighting_RTX = std::unique_ptr<Lighting_RTX, decltype(&Lighting_RTXDeleter)>(new Lighting_RTX(&bufferManger, &vulkanContext, &camera, commandPool, skyBox.get()), Lighting_RTXDeleter);
	ssao_FullScreenQuad = std::unique_ptr<SSA0_FullScreenQuad, decltype(&SSA0_FullScreenQuadDeleter)>(new SSA0_FullScreenQuad(&bufferManger, &vulkanContext, &camera, commandPool), SSA0_FullScreenQuadDeleter);
	fxaa_FullScreenQuad = std::unique_ptr<FXAA_FullScreenQuad, decltype(&FXAA_FullScreenQuadDeleter)>(new FXAA_FullScreenQuad(&bufferManger, &vulkanContext, &camera, commandPool), FXAA_FullScreenQuadDeleter);
	Combined_FullScreenQuad = std::unique_ptr<CombinedResult_FullScreenQuad, decltype(&CombinedResult_FullScreenQuadDeleter)>(new CombinedResult_FullScreenQuad(&bufferManger, &vulkanContext, &camera, commandPool), CombinedResult_FullScreenQuadDeleter);
	SSGI_FullScreenQuad = std::unique_ptr<SSGI, decltype(&SSGIDeleter)>(new SSGI(&bufferManger, &vulkanContext, &camera, commandPool), SSGIDeleter);
	dynamicDiffuse_RTGI = std::unique_ptr<DynamicDiffuse_RTGI, decltype(&DynamicDiffuse_RTGIDeleter)>(new DynamicDiffuse_RTGI("../Textures/Sphere/scene.gltf", &vulkanContext, commandPool, &camera, &bufferManger, skyBox.get()), DynamicDiffuse_RTGIDeleter);
	Restir_DI = std::unique_ptr<ReSTIR_DI, decltype(&ReSTIR_DI_Deleter)>(new ReSTIR_DI(&vulkanContext, commandPool, &camera, &bufferManger, lighting_RTX.get(), SSGI_FullScreenQuad.get(), dynamicDiffuse_RTGI.get()), ReSTIR_DI_Deleter);

	createCommandBuffer();
	CreateGraphicsPipeline();
	createShaderBindingTable();
	createDepthTextureImage();
	createGBuffer();

	recreateSwapChain();
	CreateDebugUtils();
}

void App::LoadAllObjects()
{


	////Sponza SETUP
	auto Bunny = std::shared_ptr<Model>(new Model("../Textures/Bunny/scene.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);
	auto Dragon = std::shared_ptr<Model>(new Model("../Textures/Dragon/scene.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);
	auto Sponza = std::shared_ptr<Model>(new Model("../Textures/PBR_Sponza/Sponza.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);

	Bunny->Instances[0]->SetPostion(glm::vec3(-14.224, -0.329, 0.357));
	Bunny->Instances[0]->SetRotation(glm::vec3(-179.999, -33.858, -179.999));
	Bunny->Instances[0]->SetScale(glm::vec3(0.120, 0.120, 0.120));
	Bunny->Instances[0]->CubeMapReflectiveSwitch(false);
	Bunny->Instances[0]->ScreenSpaceReflectiveSwitch(false);

	Dragon->Instances[0]->SetPostion(glm::vec3(7.153, -0.523, -2.166));
	Dragon->Instances[0]->SetRotation(glm::vec3(179.997, 44.147, 179.993));
	Dragon->Instances[0]->SetScale(glm::vec3(0.120, 0.120, 0.120));
	Dragon->Instances[0]->CubeMapReflectiveSwitch(false);
	Dragon->Instances[0]->ScreenSpaceReflectiveSwitch(false);

	Sponza->Instances[0]->SetScale(glm::vec3(5.000, 5.000, 5.000));
	Sponza->Instances[0]->CubeMapReflectiveSwitch(false);
	Sponza->Instances[0]->ScreenSpaceReflectiveSwitch(false);

	Bunny->createDescriptorSets(DescriptorPool);
	Dragon->createDescriptorSets(DescriptorPool);
	Sponza->createDescriptorSets(DescriptorPool);

	SponzaSceneModels.push_back(std::move(Bunny));
	SponzaSceneModels.push_back(std::move(Dragon));
	SponzaSceneModels.push_back(std::move(Sponza));


	////////CORNEL SETUP////////////////////////////////////////
	auto Bunny2 = std::shared_ptr<Model>(new Model("../Textures/Bunny/scene.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);
	auto Dragon2 = std::shared_ptr<Model>(new Model("../Textures/Dragon/scene.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);
	auto CornelBox = std::shared_ptr<Model>(new Model("../Textures/CornelBox/Cornel.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);

	//auto model10 = std::shared_ptr<Model>(new Model("../Textures/Head/Untitled.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);
	Bunny2->Instances[0]->SetPostion(glm::vec3(-5.936, 3.043, -9.525));
	Bunny2->Instances[0]->SetRotation(glm::vec3(-179.998, -0.000, -180.000));
	Bunny2->Instances[0]->SetScale(glm::vec3(0.082, 0.082, 0.082));
	Bunny2->Instances[0]->CubeMapReflectiveSwitch(false);
	Bunny2->Instances[0]->ScreenSpaceReflectiveSwitch(false);


	Dragon2->Instances[0]->SetPostion(glm::vec3(6.087, 13.277, 3.776));
	Dragon2->Instances[0]->SetRotation(glm::vec3(-179.987, -49.785, 179.967));
	Dragon2->Instances[0]->SetScale(glm::vec3(0.082, 0.082, 0.082));
	Dragon2->Instances[0]->CubeMapReflectiveSwitch(false);
	Dragon2->Instances[0]->ScreenSpaceReflectiveSwitch(false);

	CornelBox->Instances[0]->SetPostion(glm::vec3(0, 0, 0));
	CornelBox->Instances[0]->SetScale(glm::vec3(1.5, 1.5, 1.5));
	CornelBox->Instances[0]->CubeMapReflectiveSwitch(false);
	CornelBox->Instances[0]->ScreenSpaceReflectiveSwitch(false);

	Bunny2->createDescriptorSets(DescriptorPool);
	Dragon2->createDescriptorSets(DescriptorPool);
	CornelBox->createDescriptorSets(DescriptorPool);

	CornelSceneModels.push_back(std::move(Bunny2));
	CornelSceneModels.push_back(std::move(Dragon2));
	CornelSceneModels.push_back(std::move(CornelBox));

	////////ALT CORNEL SETUP////////////////////////////////////////
	auto Bunny3 = std::shared_ptr<Model>(new Model("../Textures/Bunny2/scene.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);
	auto AltCornelBox = std::shared_ptr<Model>(new Model("../Textures/EmptyCornel/Cornel.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);

	//auto model10 = std::shared_ptr<Model>(new Model("../Textures/Head/Untitled.gltf", &vulkanContext, commandPool, &camera, &bufferManger), ModelDeleter);
	Bunny3->Instances[0]->SetPostion(glm::vec3(-1.185, -3.895, -1.218));
	Bunny3->Instances[0]->SetRotation(glm::vec3(-179.998, -0.000, -180.000));
	Bunny3->Instances[0]->SetScale(glm::vec3(0.190, 0.190, 0.190));
	Bunny3->Instances[0]->CubeMapReflectiveSwitch(false);
	Bunny3->Instances[0]->ScreenSpaceReflectiveSwitch(false);

	AltCornelBox->Instances[0]->SetPostion(glm::vec3(0, 0, 0));
	AltCornelBox->Instances[0]->SetScale(glm::vec3(1.5, 1.5, 1.5));
	AltCornelBox->Instances[0]->CubeMapReflectiveSwitch(false);
	AltCornelBox->Instances[0]->ScreenSpaceReflectiveSwitch(false);

	Bunny3->createDescriptorSets(DescriptorPool);
	AltCornelBox->createDescriptorSets(DescriptorPool);

	AltCornelSceneModels.push_back(std::move(Bunny3));
	AltCornelSceneModels.push_back(std::move(AltCornelBox));

}

void App::UpdateRayTracingDescriptors()
{
	if (lighting_RTX) {
		lighting_RTX->createDescriptorSetsBasedOnGBuffer(DescriptorPool, &gbuffer, &TLAS);
	}

	if (RT_Reflection) {
		RT_Reflection->createRaytracedDescriptorSets(DescriptorPool, TLAS, gbuffer, lighting_RTX->UniformBuffers);
	}

	bool ddgiRecreated = false;
	if (dynamicDiffuse_RTGI) {
		ddgiRecreated = dynamicDiffuse_RTGI->UpdateUniformBuffer(DescriptorPool, TLAS, lighting_RTX->UniformBuffers, gbuffer, true, lights.size());
	}

	if (Restir_DI) {
		Restir_DI->createDescriptorSetsBasedOnGBuffer(DescriptorPool, &TLAS);

		if (ddgiRecreated) {
			Restir_DI->createDescriptorDDGIATLAS(DescriptorPool);
		}
	}

	if (ddgiRecreated) {

		DDGIIrradianceAtlasID = ImGui_ImplVulkan_AddTexture(
			dynamicDiffuse_RTGI->IradianceImageAtlasImage.imageSampler,
			dynamicDiffuse_RTGI->IradianceImageAtlasImage.imageView,
			VK_IMAGE_LAYOUT_GENERAL
		);

		DDGIIVisibilityAtlasID = ImGui_ImplVulkan_AddTexture(
			dynamicDiffuse_RTGI->VisibilityImageAtlasImage.imageSampler,
			dynamicDiffuse_RTGI->VisibilityImageAtlasImage.imageView,
			VK_IMAGE_LAYOUT_GENERAL
		);

		Sampled_GI_ID = ImGui_ImplVulkan_AddTexture(
			dynamicDiffuse_RTGI->Probe_Sampled_GI_Image.imageSampler,
			dynamicDiffuse_RTGI->Probe_Sampled_GI_Image.imageView,
			VK_IMAGE_LAYOUT_GENERAL
		);
	}
}

void App::SwitchScene(int index)
{
	vulkanContext.LogicalDevice.waitIdle();

	Models.clear();
	UserInterfaceItems.clear();
	lights.clear(); 

	currentSceneIndex = index;

	if (index == 0)
	{
		for (auto& model : CornelSceneModels) {
			Models.push_back(model.get());
			UserInterfaceItems.push_back(model.get());
		}

		int LightCount = 8;

		lights.reserve(LightCount);

		std::random_device rd;

		std::mt19937 gen(rd());

		std::uniform_real_distribution<float> disXZ(-20, 20);
		std::uniform_real_distribution<float> disY(0, 40);
		std::uniform_real_distribution<float> disc(0, 1);

		for (int i = 0; i < LightCount; i++) {
			std::shared_ptr<Light> light = std::shared_ptr<Light>(new Light(&vulkanContext, commandPool, &camera, &bufferManger), LightDeleter);

			float randX = disXZ(gen);
			float randY = disY(gen);
			float randZ = disXZ(gen);

			light->SetPosition(glm::vec3(randX, randY, randZ));
			light->CastShadow = true;
			light->createDescriptorSets(DescriptorPool);

			float R = disc(gen);
			float G = disc(gen);
			float B = disc(gen);

			light->color = glm::vec3(R, G, B);

			lights.push_back(std::move(light));
		}

		if (dynamicDiffuse_RTGI)
		{
			dynamicDiffuse_RTGI->NumOfProbesX = 9;
			dynamicDiffuse_RTGI->NumOfProbesY = 9;
			dynamicDiffuse_RTGI->NumOfProbesZ = 9;
			dynamicDiffuse_RTGI->RaysPerProbe = 128;
			dynamicDiffuse_RTGI->ProbeOffset = glm::vec3(7.52, 8.62, 11.13);
			dynamicDiffuse_RTGI->GridLocation = glm::vec3(-24.67, -12.66, -45.61);
		}

		camera.SetPosition(glm::vec3{ -0.896284, 12.566, -37.7205 });
		camera.SetRotation(89.1, -2.5);
		DefferedDecider = 3;
	}

	else if (index == 1) 
	{
		for (auto& model : SponzaSceneModels) {
			Models.push_back(model.get());
			UserInterfaceItems.push_back(model.get());
		}

		int LightCount = 3;

		lights.reserve(LightCount);

		std::random_device rd;

		std::mt19937 gen(rd());

		std::uniform_real_distribution<float> disXZ(-20, 20);
		std::uniform_real_distribution<float> disY(0, 40);
		std::uniform_real_distribution<float> disc(0, 1);

		for (int i = 0; i < LightCount; i++) {
			std::shared_ptr<Light> light = std::shared_ptr<Light>(new Light(&vulkanContext, commandPool, &camera, &bufferManger), LightDeleter);

			float randX = disXZ(gen);
			float randY = disY(gen);
			float randZ = disXZ(gen);

			light->SetPosition(glm::vec3(randX, randY, randZ));
			light->CastShadow = true;
			light->createDescriptorSets(DescriptorPool);

			float R = disc(gen);
			float G = disc(gen);
			float B = disc(gen);

			light->color = glm::vec3(R, G, B);

			lights.push_back(std::move(light));
		}

		if (dynamicDiffuse_RTGI)
		{
			dynamicDiffuse_RTGI->NumOfProbesX = 16;
			dynamicDiffuse_RTGI->NumOfProbesY = 16;
			dynamicDiffuse_RTGI->NumOfProbesZ = 16;
			dynamicDiffuse_RTGI->RaysPerProbe = 128;
			dynamicDiffuse_RTGI->GridLocation = glm::vec3(-100, -4.37, -48.60);
			dynamicDiffuse_RTGI->ProbeOffset = glm::vec3(14.42, 7.40, 6.61);

		}

		camera.SetPosition(glm::vec3{ 32.9095, 15.871, -0.912267 });
		camera.SetRotation(-178.2, -13.4);
		DefferedDecider = 2;
	}
	else if (index == 2)
	{
		for (auto& model : AltCornelSceneModels) {
			Models.push_back(model.get());
			UserInterfaceItems.push_back(model.get());
		}

		int LightCount = 8;

		lights.reserve(LightCount);

		std::random_device rd;

		std::mt19937 gen(rd());

		std::uniform_real_distribution<float> disXZ(-20, 20);
		std::uniform_real_distribution<float> disY(0, 40);
		std::uniform_real_distribution<float> disc(0, 1);

		for (int i = 0; i < LightCount; i++) {
			std::shared_ptr<Light> light = std::shared_ptr<Light>(new Light(&vulkanContext, commandPool, &camera, &bufferManger), LightDeleter);

			float randX = disXZ(gen);
			float randY = disY(gen);
			float randZ = disXZ(gen);

			light->SetPosition(glm::vec3(randX, randY, randZ));
			light->CastShadow = true;
			light->createDescriptorSets(DescriptorPool);

			float R = disc(gen);
			float G = disc(gen);
			float B = disc(gen);

			light->color = glm::vec3(R, G, B);

			lights.push_back(std::move(light));
		}

		if (dynamicDiffuse_RTGI)
		{
			dynamicDiffuse_RTGI->NumOfProbesX = 9;
			dynamicDiffuse_RTGI->NumOfProbesY = 9;
			dynamicDiffuse_RTGI->NumOfProbesZ = 9;
			dynamicDiffuse_RTGI->RaysPerProbe = 128;
			dynamicDiffuse_RTGI->ProbeOffset = glm::vec3(7.52, 8.62, 11.13);
			dynamicDiffuse_RTGI->GridLocation = glm::vec3(-24.67, -12.66, -45.61);
		}

		camera.SetPosition(glm::vec3{ -0.896284, 12.566, -37.7205 });
		camera.SetRotation(89.1, -2.5);
		DefferedDecider = 3;
	}

	for (auto& l : lights) {
		UserInterfaceItems.push_back(l.get());
	}



	DestroyTLAS();
	createTLAS();

	UpdateRayTracingDescriptors();

	if (RT_ShadowsPassPipeline) {
		updateUniformBuffer(currentFrame); 
		vulkanContext.ResetTemporalAccumilation();
	}

}

void App::CreateDebugUtils()
{

	Gbuffer_Label.pLabelName                     = "Gbuffer Pass";
	SSAO_Label.pLabelName                        = "SSAO Pass";
	RTShadows_Label.pLabelName                   = "Raytraced Shadow Pass";
	DirectLighting_Label.pLabelName              = "DirectLighting Pass";
	SSGI_Label.pLabelName                        = "SSGI Pass";
	FXAA_Label.pLabelName                        = "FXAA Pass";
	RTReflections_Label.pLabelName               = "Raytraced Reflections Pass";
	DDGI_Grid_Generation_Label.pLabelName        = "DDGI_Grid_Generation_Label";
	DDGI_Trace_Ray_Label.pLabelName              = "DDGI_Trace_Ray_Label";
	DDGI_Directions_Generation_Label.pLabelName  = "DDGI_Directions_Generation_Label";
	DDGI_Calculate_Irradiance_Label.pLabelName   = "DDGI_Calculate_Irradiance_Label";
	DDGI_Sample_From_PorbeLabel.pLabelName       = "DDGI_Sample_From_PorbeLabel";
	DDGI_Update_Probe_Status_Label.pLabelName    = "DDGI_Update_Probe_Status_Label";
	ReSTIR_Label.pLabelName    = "ReSTIR_Label";

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	VkCommandBuffer initCmd;
	//vkAllocateCommandBuffers(device, &allocInfo, &initCmd);

	//tracyVkContext = TracyVkContext(physicalDevice, device, graphicsQueue, initCmd);

}

void App::createTLAS()
{
	size_t totalPrimitiveCount = 0;
	for (const auto& model : Models) {
		totalPrimitiveCount += model->BLAS_Datas.size();
	}

	// Create instance Buffer
	TLAS_InstanceData.BufferID = "Scene TLAS InstanceData Buffer";
	size_t totalSize = sizeof(vk::AccelerationStructureInstanceKHR) * totalPrimitiveCount;

	bufferManger.CreateBuffer(&TLAS_InstanceData, totalSize,
		vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
		vk::BufferUsageFlagBits::eShaderDeviceAddress, commandPool, vulkanContext.graphicsQueue);

	UpdateTLASInstanceBuffer();

	////////////////////////////////////GEOMETRY INFO /////////////////////////////////////////////////////////////////////
	//get instance buffer adddress
	vk::BufferDeviceAddressInfo InstanceInfo{};
	InstanceInfo.buffer = TLAS_InstanceData.buffer;

	vk::DeviceOrHostAddressConstKHR instanceDataDeviceAddresstance{};
	instanceDataDeviceAddresstance.deviceAddress = vulkanContext.LogicalDevice.getBufferAddress(InstanceInfo); // pass instance buffer address


	vk::AccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.geometryType = vk::GeometryTypeKHR::eInstances;
	accelerationStructureGeometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
	accelerationStructureGeometry.geometry.instances.sType = vk::StructureType::eAccelerationStructureGeometryInstancesDataKHR;
	accelerationStructureGeometry.geometry.instances.arrayOfPointers = vk::False;
	accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddresstance;

	// Get size info
	vk::AccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {};
	accelerationStructureBuildGeometryInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
	accelerationStructureBuildGeometryInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
	accelerationStructureBuildGeometryInfo.geometryCount = 1;
	accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;



	uint32_t primitive_count = totalPrimitiveCount;

	VkAccelerationStructureBuildGeometryInfoKHR TEMP_ACCELERATION_INFO = accelerationStructureBuildGeometryInfo;
	VkAccelerationStructureBuildSizesInfoKHR TEMP_ACCELERATION_STRUCTURE_BUILD_SIZE{};
	TEMP_ACCELERATION_STRUCTURE_BUILD_SIZE.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	TEMP_ACCELERATION_STRUCTURE_BUILD_SIZE.pNext = nullptr;

	vulkanContext.vkGetAccelerationStructureBuildSizesKHR(vulkanContext.LogicalDevice,
		VkAccelerationStructureBuildTypeKHR::VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&TEMP_ACCELERATION_INFO, &primitive_count, &TEMP_ACCELERATION_STRUCTURE_BUILD_SIZE);

	vk::AccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = TEMP_ACCELERATION_STRUCTURE_BUILD_SIZE;
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

												  ///////CREATE TLAS BUFFER////////
	TLAS_Buffer.BufferID = "Scene TLAS Buffer";

	bufferManger.CreateDeviceBuffer(&TLAS_Buffer,
		accelerationStructureBuildSizesInfo.accelerationStructureSize,
		vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
		vk::BufferUsageFlagBits::eShaderDeviceAddress,
		commandPool,
		vulkanContext.graphicsQueue);

	// Acceleration structure
	vk::AccelerationStructureCreateInfoKHR accelerationStructureCreate_info{};
	accelerationStructureCreate_info.buffer = TLAS_Buffer.buffer;
	accelerationStructureCreate_info.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
	accelerationStructureCreate_info.type = vk::AccelerationStructureTypeKHR::eTopLevel;

	VkAccelerationStructureCreateInfoKHR TEMP_ACCELERATION_STRUCTURE_CREATE_INFO = accelerationStructureCreate_info;
	VkAccelerationStructureKHR TEMP_TLAS;
	vulkanContext.vkCreateAccelerationStructureKHR(vulkanContext.LogicalDevice, &TEMP_ACCELERATION_STRUCTURE_CREATE_INFO, nullptr, &TEMP_TLAS);
	TLAS = TEMP_TLAS;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
										   ///////CREATE TLAS SCRATCH BUFFER////////

	TLAS_SCRATCH_Buffer.BufferID = "TLAS_ScratchBuffer Buffer";
	bufferManger.CreateDeviceBuffer(&TLAS_SCRATCH_Buffer,
		accelerationStructureBuildSizesInfo.buildScratchSize,
		vk::BufferUsageFlagBits::eStorageBuffer |
		vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
		vk::BufferUsageFlagBits::eShaderDeviceAddress,
		commandPool,
		vulkanContext.graphicsQueue);

	vk::BufferDeviceAddressInfo TLAS_ScratchBufferAdress;
	TLAS_ScratchBufferAdress.buffer = TLAS_SCRATCH_Buffer.buffer;

	accelerationStructureBuildGeometryInfo.dstAccelerationStructure = TLAS;
	accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = vulkanContext.LogicalDevice.getBufferAddress(TLAS_ScratchBufferAdress);
	accelerationStructureBuildGeometryInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

											   ///////BUILD TLAS ON THE GPU ////////

	vk::CommandBuffer cmd = bufferManger.CreateSingleUseCommandBuffer(commandPool);

	vk::AccelerationStructureBuildRangeInfoKHR BuildRangeInfo;
	BuildRangeInfo.firstVertex = 0;
	BuildRangeInfo.primitiveCount = primitive_count;
	BuildRangeInfo.primitiveOffset = 0;
	BuildRangeInfo.transformOffset = 0;

	VkAccelerationStructureBuildRangeInfoKHR tempRange = BuildRangeInfo;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &tempRange };

	VkAccelerationStructureBuildGeometryInfoKHR tempGeometryInfo = accelerationStructureBuildGeometryInfo;

	vulkanContext.vkCmdBuildAccelerationStructuresKHR(cmd, 1,
		&tempGeometryInfo,
		accelerationBuildStructureRangeInfos.data());

	bufferManger.SubmitAndDestoyCommandBuffer(commandPool, cmd, vulkanContext.graphicsQueue);
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

void App::UpdateTLAS()
{
	// 1. Update instance data on the GPU
	UpdateTLASInstanceBuffer();

	size_t totalPrimitiveCount = 0;
	for (const auto& model : Models) {
		totalPrimitiveCount += model->BLAS_Datas.size();
	}

	// 2. Reuse instance buffer device address
	vk::BufferDeviceAddressInfo instanceInfo{};
	instanceInfo.buffer = TLAS_InstanceData.buffer;

	vk::DeviceOrHostAddressConstKHR instanceDeviceAddress{};
	instanceDeviceAddress.deviceAddress = vulkanContext.LogicalDevice.getBufferAddress(instanceInfo);

	// 3. Setup geometry
	vk::AccelerationStructureGeometryKHR geometry{};
	geometry.geometryType = vk::GeometryTypeKHR::eInstances;
	geometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
	geometry.geometry.instances.sType = vk::StructureType::eAccelerationStructureGeometryInstancesDataKHR;
	geometry.geometry.instances.arrayOfPointers = VK_FALSE;
	geometry.geometry.instances.data = instanceDeviceAddress;

	// 4. Build geometry info with UPDATE mode
	vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{};
	buildInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
	buildInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace |
		vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate; // Must match initial build flags
	buildInfo.geometryCount = 1;
	buildInfo.pGeometries = &geometry;
	buildInfo.mode = vk::BuildAccelerationStructureModeKHR::eUpdate;
	buildInfo.srcAccelerationStructure = TLAS;
	buildInfo.dstAccelerationStructure = TLAS;

	// 5. Scratch buffer address
	vk::BufferDeviceAddressInfo scratchAddrInfo{};
	scratchAddrInfo.buffer = TLAS_SCRATCH_Buffer.buffer;
	buildInfo.scratchData.deviceAddress = vulkanContext.LogicalDevice.getBufferAddress(scratchAddrInfo);

	// 6. Build range info
	vk::AccelerationStructureBuildRangeInfoKHR buildRange{};
	buildRange.primitiveCount = static_cast<uint32_t>(totalPrimitiveCount);
	buildRange.primitiveOffset = 0;
	buildRange.firstVertex = 0;
	buildRange.transformOffset = 0;

	VkAccelerationStructureBuildRangeInfoKHR tempRange = buildRange;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> rangeInfos = { &tempRange };

	// 7. Record and submit command buffer
	vk::CommandBuffer cmd = bufferManger.CreateSingleUseCommandBuffer(commandPool);
	VkAccelerationStructureBuildGeometryInfoKHR tempBuildInfo = buildInfo;

	vulkanContext.vkCmdBuildAccelerationStructuresKHR(cmd, 1, &tempBuildInfo, rangeInfos.data());
	bufferManger.SubmitAndDestoyCommandBuffer(commandPool, cmd, vulkanContext.graphicsQueue);
}


void App::UpdateTLASInstanceBuffer()
{
	std::vector<vk::AccelerationStructureInstanceKHR> Instances;
	uint32_t ObjectID = 0;

	for (int i = 0; i < Models.size(); i++)
	{
		glm::mat4 modelInstanceTransform = Models[i]->Instances[0]->GetTransformationMatrix();

		for (int j = 0; j < Models[i]->BLAS_Datas.size(); j++)
		{
			vk::AccelerationStructureDeviceAddressInfoKHR blasinfo{};
			blasinfo.accelerationStructure = Models[i]->BLAS_Datas[j].BLAS;

			VkAccelerationStructureDeviceAddressInfoKHR Temp = blasinfo;

			glm::mat4 finalMatrix = modelInstanceTransform;

			VkTransformMatrixKHR transformMatrix = {
				finalMatrix[0][0], finalMatrix[1][0], finalMatrix[2][0], finalMatrix[3][0],
				finalMatrix[0][1], finalMatrix[1][1], finalMatrix[2][1], finalMatrix[3][1],
				finalMatrix[0][2], finalMatrix[1][2], finalMatrix[2][2], finalMatrix[3][2],
			};


			uint32_t globalPrimIndex = Models[i]->BLAS_Datas[j].GlobalPrimitiveIndex;

			uint32_t packedID = (ObjectID << 12) | (globalPrimIndex & 0xFFF);

			vk::AccelerationStructureInstanceKHR instance{};
			instance.transform = transformMatrix;
			instance.instanceCustomIndex = packedID;
			instance.mask = 0xFF;
			instance.instanceShaderBindingTableRecordOffset = 0;
			instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
			instance.accelerationStructureReference = vulkanContext.vkGetAccelerationStructureDeviceAddressKHR(vulkanContext.LogicalDevice, &Temp);

			Instances.push_back(instance);
		}
		ObjectID++;
	}
	bufferManger.CopyDataToBuffer(Instances.data(), TLAS_InstanceData);
}


void App::createDescriptorPool()
{
	vk::DescriptorPoolSize Uniformpoolsize;
	Uniformpoolsize.type = vk::DescriptorType::eUniformBuffer;
	Uniformpoolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 100;

	vk::DescriptorPoolSize Samplerpoolsize;
	Samplerpoolsize.type = vk::DescriptorType::eCombinedImageSampler;
	Samplerpoolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 300;

	vk::DescriptorPoolSize AccelerationStructurepoolsize;
	AccelerationStructurepoolsize.type = vk::DescriptorType::eAccelerationStructureKHR;
	AccelerationStructurepoolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

	vk::DescriptorPoolSize StorageImagepoolsize;
	StorageImagepoolsize.type = vk::DescriptorType::eStorageImage;
	StorageImagepoolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 20;

	vk::DescriptorPoolSize StorageBufferpoolsize;
	StorageBufferpoolsize.type = vk::DescriptorType::eStorageBuffer;
	StorageBufferpoolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 5;

	std::array<	vk::DescriptorPoolSize, 5> poolSizes{ Uniformpoolsize ,Samplerpoolsize,
													  AccelerationStructurepoolsize,StorageImagepoolsize,StorageBufferpoolsize };

	vk::DescriptorPoolCreateInfo poolInfo{};
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 1000;

	DescriptorPool = vulkanContext.LogicalDevice.createDescriptorPool(poolInfo, nullptr);

	if (skyBox) {
		skyBox->createDescriptorSets(DescriptorPool);
	}
}


void App::createDepthTextureImage()
{
	vk::Extent3D swapchainextent = vk::Extent3D(vulkanContext.swapchainExtent.width, vulkanContext.swapchainExtent.height, 1);


	DepthTextureData.ImageID = "Depth Texture";
	bufferManger.CreateImage(&DepthTextureData,swapchainextent, vulkanContext.FindCompatableDepthFormat(), vk::ImageUsageFlagBits::eDepthStencilAttachment |vk::ImageUsageFlagBits::eSampled);
	DepthTextureData.imageView = bufferManger.CreateImageView(&DepthTextureData, vulkanContext.FindCompatableDepthFormat(), vk::ImageAspectFlagBits::eDepth);
	DepthTextureData.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	vk::CommandBuffer commandBuffer = bufferManger.CreateSingleUseCommandBuffer(commandPool);

	ImageTransitionData DataToTransitionInfo;
	DataToTransitionInfo.oldlayout = vk::ImageLayout::eUndefined;
	DataToTransitionInfo.newlayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	DataToTransitionInfo.AspectFlag = vk::ImageAspectFlagBits::eDepth;
	//////////////////////////////////////////////////////////////////////////////
	DataToTransitionInfo.SourceAccessflag = vk::AccessFlagBits::eNone;
	DataToTransitionInfo.DestinationAccessflag = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	///////////////////////////////////////////////////////////////////////////////
	DataToTransitionInfo.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	DataToTransitionInfo.DestinationOnThePipeline = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;

	bufferManger.TransitionImage(commandBuffer, &DepthTextureData, DataToTransitionInfo);

	bufferManger.SubmitAndDestoyCommandBuffer(commandPool, commandBuffer, vulkanContext.graphicsQueue);

}


void App::createGBuffer()
{
	vulkanContext.ResetTemporalAccumilation();

	vk::Extent3D swapchainextent = vk::Extent3D(vulkanContext.swapchainExtent.width, vulkanContext.swapchainExtent.height, 1);

	gbuffer.Position.ImageID = "Gbuffer Position Texture";
	bufferManger.CreateImage(&gbuffer.Position,swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.Position.imageView = bufferManger.CreateImageView(&gbuffer.Position, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	gbuffer.Position.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	gbuffer.ViewSpacePosition.ImageID = "Gbuffer Position Texture";
	bufferManger.CreateImage(&gbuffer.ViewSpacePosition,swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.ViewSpacePosition.imageView = bufferManger.CreateImageView(&gbuffer.ViewSpacePosition, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	gbuffer.ViewSpacePosition.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	gbuffer.Normal.ImageID = "Gbuffer WorldSpaceNormal Texture";
	bufferManger.CreateImage(&gbuffer.Normal,swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc);
	gbuffer.Normal.imageView = bufferManger.CreateImageView(&gbuffer.Normal, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	gbuffer.Normal.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);
	
	gbuffer.PrevNormal.ImageID = "Gbuffer prev WorldSpaceNormal Texture";
	bufferManger.CreateImage(&gbuffer.PrevNormal, swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
	gbuffer.PrevNormal.imageView = bufferManger.CreateImageView(&gbuffer.PrevNormal, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	gbuffer.PrevNormal.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	gbuffer.ViewSpaceNormal.ImageID = "Gbuffer ViewSpaceNormal Texture";
	bufferManger.CreateImage(&gbuffer.ViewSpaceNormal,swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.ViewSpaceNormal.imageView = bufferManger.CreateImageView(&gbuffer.ViewSpaceNormal, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	gbuffer.ViewSpaceNormal.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	gbuffer.Materials.ImageID = "Gbuffer Materials Texture";
	bufferManger.CreateImage(&gbuffer.Materials ,swapchainextent, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.Materials.imageView = bufferManger.CreateImageView(&gbuffer.Materials, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
	gbuffer.Materials.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	gbuffer.Albedo.ImageID = "Gbuffer Albedo Texture";
	bufferManger.CreateImage(&gbuffer.Albedo,swapchainextent, vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.Albedo.imageView = bufferManger.CreateImageView(&gbuffer.Albedo, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
	gbuffer.Albedo.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	gbuffer.Emissive.ImageID = "Gbuffer Emissive Texture";
	bufferManger.CreateImage(&gbuffer.Emissive, swapchainextent, vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.Emissive.imageView = bufferManger.CreateImageView(&gbuffer.Emissive, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
	gbuffer.Emissive.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	gbuffer.MotionVector.ImageID = "MotionVectors Texture";
	bufferManger.CreateImage(&gbuffer.MotionVector, swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.MotionVector.imageView = bufferManger.CreateImageView(&gbuffer.MotionVector, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	gbuffer.MotionVector.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);


	fxaa_FullScreenQuad->CreateImage(swapchainextent);
	SSGI_FullScreenQuad->CreateGIImage();
	Combined_FullScreenQuad->CreateImage(swapchainextent);
	ssao_FullScreenQuad->CreateImage();
	RT_Reflection->CreateStorageImage();
	dynamicDiffuse_RTGI->CreateSampledGIImage(); 
	Restir_DI->CreateImage();
	lighting_RTX->CreateStorageImage();

	lighting_RTX->createDescriptorSetsBasedOnGBuffer(DescriptorPool, &gbuffer,&TLAS);
	ssao_FullScreenQuad->createDescriptorSetsBasedOnGBuffer(DescriptorPool, gbuffer);
	Combined_FullScreenQuad->createDescriptorSetsBasedOnGBuffer(DescriptorPool, lighting_RTX->ResultingStorageImage, SSGI_FullScreenQuad->BlurPong_UPSampleFullRes, ssao_FullScreenQuad->BluredSSAOImage, gbuffer.Materials,gbuffer.Albedo, dynamicDiffuse_RTGI->Probe_Sampled_GI_Image);
	fxaa_FullScreenQuad->createDescriptorSets(DescriptorPool, Combined_FullScreenQuad->FinalResultImage);
	SSGI_FullScreenQuad->createDescriptorSets(DescriptorPool,gbuffer, lighting_RTX->ResultingStorageImage,DepthTextureData);
	RT_Reflection->createRaytracedDescriptorSets(DescriptorPool, TLAS, gbuffer, lighting_RTX->UniformBuffers);
	dynamicDiffuse_RTGI->createDescriptorSets(DescriptorPool, gbuffer);
	Restir_DI->createDescriptorSetsBasedOnGBuffer(DescriptorPool,&TLAS);


	vk::CommandBuffer cmd =  bufferManger.CreateSingleUseCommandBuffer(commandPool);
	ImageTransitionData TransitionToGeneral{};
	TransitionToGeneral.oldlayout = vk::ImageLayout::eUndefined;
	TransitionToGeneral.newlayout = vk::ImageLayout::eGeneral;
	TransitionToGeneral.AspectFlag = vk::ImageAspectFlagBits::eColor;
	TransitionToGeneral.SourceAccessflag = vk::AccessFlagBits::eNone;
	TransitionToGeneral.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eShaderRead;
	TransitionToGeneral.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	TransitionToGeneral.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eFragmentShader;

	vulkanContext.ResetTemporalAccumilation();

	bufferManger.TransitionImage(cmd, &gbuffer.Position, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &gbuffer.ViewSpacePosition, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &gbuffer.Normal, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &gbuffer.ViewSpaceNormal, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &gbuffer.Albedo, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &gbuffer.Emissive, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &gbuffer.Materials, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &gbuffer.MotionVector, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &Combined_FullScreenQuad->FinalResultImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &SSGI_FullScreenQuad->SSGIPassImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &SSGI_FullScreenQuad->SSGIPassLastFrameImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &SSGI_FullScreenQuad->SSGIAccumilationImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &SSGI_FullScreenQuad->BlurPing_DownSampleHalfRes, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &SSGI_FullScreenQuad->BlurPong_DownSampleHalfRes, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &SSGI_FullScreenQuad->BlurPing_DownSampleQuaterRes, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &SSGI_FullScreenQuad->BlurPong_DownSampleQuaterRes, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &SSGI_FullScreenQuad->BlurPing_UPSampleHalfRes, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &SSGI_FullScreenQuad->BlurPong_UPSampleHalfRes, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &SSGI_FullScreenQuad->BlurPing_UPSampleFullRes, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &SSGI_FullScreenQuad->BlurPong_UPSampleFullRes, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &ssao_FullScreenQuad->SSAOImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &ssao_FullScreenQuad->BluredSSAOImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &fxaa_FullScreenQuad->FxaaImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &RT_Reflection->HorizontalBlurReflectionPassImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &RT_Reflection->FullBlurReflectionPassImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &dynamicDiffuse_RTGI->Probe_Sampled_GI_Image, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &Restir_DI->PrevResevoirImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &Restir_DI->ReSTIRDI_Results, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &lighting_RTX->ResultingStorageImage, TransitionToGeneral);

	bufferManger.SubmitAndDestoyCommandBuffer(commandPool, cmd,vulkanContext.graphicsQueue);

	
	FinalRenderTextureId = ImGui_ImplVulkan_AddTexture(fxaa_FullScreenQuad->FxaaImage.imageSampler,
		                                               fxaa_FullScreenQuad->FxaaImage.imageView,
		VK_IMAGE_LAYOUT_GENERAL);

	LightingAndReflectionsRenderTextureId = ImGui_ImplVulkan_AddTexture(lighting_RTX->ResultingStorageImage.imageSampler,
		                                                                lighting_RTX->ResultingStorageImage.imageView,
		VK_IMAGE_LAYOUT_GENERAL);


	SSGITextureId    = ImGui_ImplVulkan_AddTexture(SSGI_FullScreenQuad->SSGIAccumilationImage.imageSampler,
		                                                   SSGI_FullScreenQuad->SSGIAccumilationImage.imageView,
		                                                   VK_IMAGE_LAYOUT_GENERAL);
	

	ReSTIR_DITextureId = ImGui_ImplVulkan_AddTexture(Restir_DI->ReSTIRDI_Results.imageSampler,
		                                                       Restir_DI->ReSTIRDI_Results.imageView,
		                                                       VK_IMAGE_LAYOUT_GENERAL);

	DDGI_Radiance = ImGui_ImplVulkan_AddTexture(dynamicDiffuse_RTGI->RadianceImageAtlasImage.imageSampler,
		                                       dynamicDiffuse_RTGI->RadianceImageAtlasImage.imageView,
		                                    VK_IMAGE_LAYOUT_GENERAL);


   Sampled_GI_ID = ImGui_ImplVulkan_AddTexture(dynamicDiffuse_RTGI->Probe_Sampled_GI_Image.imageSampler,
	                                           dynamicDiffuse_RTGI->Probe_Sampled_GI_Image.imageView,
			                                   VK_IMAGE_LAYOUT_GENERAL);


	std::cout << "Swapchain size: "
		<< vulkanContext.swapchainExtent.width << " x "
		<< vulkanContext.swapchainExtent.height
		<< std::endl;
}

void App::CreateGraphicsPipeline()
{

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
	pipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanContext.swapchainformat;
	pipelineRenderingCreateInfo.depthAttachmentFormat = vulkanContext.FindCompatableDepthFormat();


	vk::PipelineInputAssemblyStateCreateInfo inputAssembleInfo{};
	inputAssembleInfo.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembleInfo.primitiveRestartEnable = vk::False;

	/////////////////////////////////////////////////////////////////////////////
	vk::Viewport viewport{};
	viewport.setX(0.0f);
	viewport.setY(0.0f);
	viewport.setHeight((float)vulkanContext.swapchainExtent.height);
	viewport.setWidth((float)vulkanContext.swapchainExtent.width);
	viewport.setMinDepth(0.0f);
	viewport.setMaxDepth(1.0f);

	vk::Offset2D scissorOffset = { 0,0 };

	vk::Rect2D scissor{};
	scissor.setOffset(scissorOffset);
	scissor.setExtent(vulkanContext.swapchainExtent);

	vk::PipelineViewportStateCreateInfo viewportState{};
	viewportState.setViewportCount(1);
	viewportState.setViewportCount(1);
	viewportState.setScissorCount(1);
	viewportState.setViewports(viewport);
	viewportState.setScissors(scissor);
	////////////////////////////////////////////////////////////////////////////////

	// Rasteriser information
	vk::PipelineRasterizationStateCreateInfo rasterizerinfo{};
	rasterizerinfo.depthClampEnable = vk::False;
	rasterizerinfo.rasterizerDiscardEnable = vk::False;
	rasterizerinfo.polygonMode = vk::PolygonMode::eFill;
	rasterizerinfo.lineWidth = 1.0f;
	rasterizerinfo.cullMode = vk::CullModeFlagBits::eNone;
	rasterizerinfo.frontFace = vk::FrontFace::eCounterClockwise;
	rasterizerinfo.depthBiasEnable = vk::False;
	rasterizerinfo.depthBiasConstantFactor = 0.0f;
	rasterizerinfo.depthBiasClamp = 0.0f;
	rasterizerinfo.depthBiasSlopeFactor = 0.0f;

	//Multi Sampling/
	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sampleShadingEnable = vk::False;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = vk::False;
	multisampling.alphaToOneEnable = vk::False;

	///////////// Color blending *COME BACK TO THIS////////////////////
	vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR |
		vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA;
	colorBlendAttachment.blendEnable = vk::True;

	colorBlendAttachment.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha);
	colorBlendAttachment.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
	colorBlendAttachment.setColorBlendOp(vk::BlendOp::eAdd);
	colorBlendAttachment.setSrcAlphaBlendFactor(vk::BlendFactor::eOne);
	colorBlendAttachment.setDstAlphaBlendFactor(vk::BlendFactor::eZero);
	colorBlendAttachment.setAlphaBlendOp(vk::BlendOp::eAdd);

	vk::PipelineColorBlendStateCreateInfo colorBlend{};
	colorBlend.setLogicOpEnable(vk::False);
	colorBlend.logicOp = vk::LogicOp::eCopy;
	colorBlend.setAttachmentCount(1);
	colorBlend.setPAttachments(&colorBlendAttachment);
	//////////////////////////////////////////////////////////////////////

	std::vector<vk::DynamicState> DynamicStates = {
	vk::DynamicState::eViewport,
	vk::DynamicState::eScissor,
	vk::DynamicState::ePolygonModeEXT
	};

	vk::PipelineDynamicStateCreateInfo DynamicState{};
	DynamicState.dynamicStateCount = static_cast<uint32_t>(DynamicStates.size());
	DynamicState.pDynamicStates = DynamicStates.data();


	{
		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanContext.swapchainformat;

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(glm::vec4));
		range.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(fxaa_FullScreenQuad->descriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;


		FullScreen_Quad_Pipeline_Data  Temp = pipelineManager.create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/FXAA.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);

		FXAAPassPipelineLayout = Temp.FQ_PipelineLayout;
		FXAAPassPipeline = Temp.FQ_Pipeline;
	}


	
	{

		std::array<vk::Format, 1> colorFormats = { vk::Format::eR8G8B8A8Unorm };

		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(ssao_FullScreenQuad->descriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		FullScreen_Quad_Pipeline_Data  Temp = pipelineManager.create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/SSAO_Shader.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);

		SSAOPipelineLayout = Temp.FQ_PipelineLayout;
		SSAOPipeline = Temp.FQ_Pipeline;
	}

	{

		std::array<vk::Format, 1> colorFormats = { vk::Format::eR8G8B8A8Unorm };

		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(glm::vec2));
		range.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(ssao_FullScreenQuad->SSAOBlurDescriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;


		FullScreen_Quad_Pipeline_Data  Temp = pipelineManager.create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/SSAOBlur_Shader.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);

		SSAOBlurPipelineLayout = Temp.FQ_PipelineLayout;
		SSAOBlurPipeline = Temp.FQ_Pipeline;

	}


	{
		std::array<vk::Format, 1> colorFormats = { vk::Format::eR16G16B16A16Sfloat };

		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(PostProcessSettings));
		range.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(Combined_FullScreenQuad->descriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = &range;
		pipelineLayoutInfo.pushConstantRangeCount = 1;

		FullScreen_Quad_Pipeline_Data  Temp = pipelineManager.create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/CombinedImage.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);

		CombinedImagePipelineLayout = Temp.FQ_PipelineLayout;
		CombinedImagePassPipeline = Temp.FQ_Pipeline;
	}


	{
		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/Light_Shader.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/Light_Shader.frag.spv");

		VkShaderModule VertShaderModule = pipelineManager.createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = pipelineManager.createShaderModule(FragShaderCode);

		vk::PipelineShaderStageCreateInfo VertShaderStageInfo{};
		VertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		VertShaderStageInfo.module = VertShaderModule;
		VertShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo FragmentShaderStageInfo{};
		FragmentShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		FragmentShaderStageInfo.module = FragShaderModule;
		FragmentShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo ,FragmentShaderStageInfo };

		auto BindDesctiptions      = VertexOnly::GetBindingDescription();
		auto attributeDescriptions = VertexOnly::GetAttributeDescription();

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.setVertexBindingDescriptionCount(1);
		vertexInputInfo.setVertexAttributeDescriptionCount(1);
		vertexInputInfo.setPVertexBindingDescriptions(&BindDesctiptions);
		vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());

		vk::PipelineDepthStencilStateCreateInfo depthStencilState{};
		                                        depthStencilState.depthTestEnable = VK_TRUE;
		                                        depthStencilState.depthWriteEnable = VK_TRUE;
		                                        depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
		                                        depthStencilState.minDepthBounds = 0.0f;
		                                        depthStencilState.maxDepthBounds = 1.0f;
		                                        depthStencilState.stencilTestEnable = VK_FALSE;
        
	    vk::PushConstantRange range = {};
	                          range.stageFlags = vk::ShaderStageFlagBits::eFragment;
	                          range.offset = 0;
	                          range.size = 12;

	    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
                                     pipelineLayoutInfo.setLayoutCount = 1;
                                     pipelineLayoutInfo.setSetLayouts(lights[0]->descriptorSetLayout);
                                     pipelineLayoutInfo.pushConstantRangeCount = 1;
                                     pipelineLayoutInfo.pPushConstantRanges = &range;
        
		LightpipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);
		
		vk::Format lightPassFormat = vk::Format::eR16G16B16A16Sfloat;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = &lightPassFormat;

		LightgraphicsPipeline = pipelineManager.createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			                                  viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, LightpipelineLayout);

		vulkanContext.LogicalDevice.destroyShaderModule(VertShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(FragShaderModule);

	}

	{
		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/DDGI_Probe.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/DDGI_Probe.frag.spv");

		VkShaderModule VertShaderModule = pipelineManager.createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = pipelineManager.createShaderModule(FragShaderCode);

		vk::PipelineShaderStageCreateInfo VertShaderStageInfo{};
		VertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		VertShaderStageInfo.module = VertShaderModule;
		VertShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo FragmentShaderStageInfo{};
		FragmentShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		FragmentShaderStageInfo.module = FragShaderModule;
		FragmentShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo ,FragmentShaderStageInfo };

		auto BindDesctiptions = ModelVertex::GetBindingDescription();
		auto attributeDescriptions = ModelVertex::GetAttributeDescription();

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.setVertexBindingDescriptionCount(1);
		vertexInputInfo.setVertexAttributeDescriptionCount(4);
		vertexInputInfo.setPVertexBindingDescriptions(&BindDesctiptions);
		vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());

		vk::PipelineDepthStencilStateCreateInfo depthStencilState{};
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.stencilTestEnable = VK_FALSE;

		vk::PushConstantRange range = {};
		range.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
		range.offset = 0;
		range.size = sizeof(CameraConstantBuffer);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(dynamicDiffuse_RTGI->ProbeDescriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		DDGIProbepipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		vk::Format lightPassFormat = vk::Format::eR16G16B16A16Sfloat;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = &lightPassFormat;

		DDGIProbePipeline = pipelineManager.createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, DDGIProbepipelineLayout);

		vulkanContext.LogicalDevice.destroyShaderModule(VertShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(FragShaderModule);

	}



	{
		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/SkyBox_Shader.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/SkyBox_Shader.frag.spv");

		VkShaderModule VertShaderModule = pipelineManager.createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = pipelineManager.createShaderModule(FragShaderCode);

		vk::PipelineShaderStageCreateInfo VertShaderStageInfo{};
		VertShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		VertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		VertShaderStageInfo.module = VertShaderModule;
		VertShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo FragmentShaderStageInfo{};
		FragmentShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		FragmentShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		FragmentShaderStageInfo.module = FragShaderModule;
		FragmentShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo ,FragmentShaderStageInfo };

		vk::PipelineDepthStencilStateCreateInfo depthStencilState;
		  										depthStencilState.depthTestEnable = vk::True;
												depthStencilState.depthWriteEnable = vk::False;
												depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
												depthStencilState.minDepthBounds = 0.0f;
												depthStencilState.maxDepthBounds = 1.0f;
												depthStencilState.stencilTestEnable = VK_FALSE;

         auto BindDesctiptions      = VertexOnly::GetBindingDescription();
         auto attributeDescriptions = VertexOnly::GetAttributeDescription();
         
         vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
         vertexInputInfo.setVertexBindingDescriptionCount(1);
         vertexInputInfo.setVertexAttributeDescriptionCount(1);
         vertexInputInfo.setPVertexBindingDescriptions(&BindDesctiptions);
         vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());


		 vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		 pipelineLayoutInfo.setLayoutCount = 1;
		 pipelineLayoutInfo.setSetLayouts(skyBox->descriptorSetLayout);
		 pipelineLayoutInfo.pushConstantRangeCount = 0;
		 pipelineLayoutInfo.pPushConstantRanges = nullptr;

		 vk::Format skyBoxFormat = vk::Format::eR16G16B16A16Sfloat;
		 pipelineRenderingCreateInfo.pColorAttachmentFormats = &skyBoxFormat;

		 SkyBoxpipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		 SkyBoxgraphicsPipeline = pipelineManager.createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			                                  viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, SkyBoxpipelineLayout);

		 vulkanContext.LogicalDevice.destroyShaderModule(VertShaderModule);
		 vulkanContext.LogicalDevice.destroyShaderModule(FragShaderModule);

	}


	{

		vk::PipelineRasterizationStateCreateInfo rasterizerinfo{};
		rasterizerinfo.depthClampEnable = vk::False;
		rasterizerinfo.rasterizerDiscardEnable = vk::False;
		rasterizerinfo.polygonMode = vk::PolygonMode::eFill;
		rasterizerinfo.lineWidth = 1.0f;
		rasterizerinfo.cullMode = vk::CullModeFlagBits::eNone;
		rasterizerinfo.frontFace = vk::FrontFace::eCounterClockwise;
		rasterizerinfo.depthBiasEnable = vk::False;
		rasterizerinfo.depthBiasConstantFactor = 0.0f;
		rasterizerinfo.depthBiasClamp = 0.0f;
		rasterizerinfo.depthBiasSlopeFactor = 0.0f;

		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/GeometryPass.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/GeometryPass.frag.spv");

		VkShaderModule VertShaderModule = pipelineManager.createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = pipelineManager.createShaderModule(FragShaderCode);

		vk::PipelineShaderStageCreateInfo VertShaderStageInfo{};
		VertShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		VertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		VertShaderStageInfo.module = VertShaderModule;
		VertShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo FragmentShaderStageInfo{};
		FragmentShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		FragmentShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		FragmentShaderStageInfo.module = FragShaderModule;
		FragmentShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo ,FragmentShaderStageInfo };

		vk::PipelineDepthStencilStateCreateInfo depthStencilState;
		depthStencilState.depthTestEnable = vk::True;
		depthStencilState.depthWriteEnable = vk::True;
		depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.stencilTestEnable = VK_FALSE;

		auto BindDesctiptions      = ModelVertex::GetBindingDescription();
		auto attributeDescriptions = ModelVertex::GetAttributeDescription();

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.setVertexBindingDescriptionCount(1);
		vertexInputInfo.setVertexAttributeDescriptionCount(4);
		vertexInputInfo.setPVertexBindingDescriptions(&BindDesctiptions);
		vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());

		std::array<vk::Format, 8> colorFormats = {
	                             vk::Format::eR16G16B16A16Sfloat, // Position
								 vk::Format::eR16G16B16A16Sfloat, // ViewSpacePosition
	                             vk::Format::eR16G16B16A16Sfloat, // Normal
					             vk::Format::eR16G16B16A16Sfloat, // // ViewSpaceNormal
	                             vk::Format::eR8G8B8A8Srgb,       // Albedo
								 vk::Format::eR8G8B8A8Srgb,       // Emmisive
								 vk::Format::eR8G8B8A8Unorm,      //Material
								 vk::Format::eR16G16B16A16Sfloat       //ReflectionMask
	                             };


		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = colorFormats.size();
		pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();
		pipelineRenderingCreateInfo.depthAttachmentFormat = vulkanContext.FindCompatableDepthFormat();


		vk::DescriptorSetLayout setLayouts[] = { Models[0]->descriptorSetLayout };

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(glm::mat4) * 2);
		range.setStageFlags(vk::ShaderStageFlagBits::eVertex);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = setLayouts;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		std::array<vk::PipelineColorBlendAttachmentState, 8> colorBlendAttachments = {
			// Position attachment blend state
			vk::PipelineColorBlendAttachmentState{}
				.setColorWriteMask(vk::ColorComponentFlagBits::eR |
								  vk::ColorComponentFlagBits::eG |
								  vk::ColorComponentFlagBits::eB |
								  vk::ColorComponentFlagBits::eA)
				.setBlendEnable(VK_FALSE),
			// Normal attachment blend state
			vk::PipelineColorBlendAttachmentState{}
				.setColorWriteMask(vk::ColorComponentFlagBits::eR |
								  vk::ColorComponentFlagBits::eG |
								  vk::ColorComponentFlagBits::eB |
								  vk::ColorComponentFlagBits::eA)
				.setBlendEnable(VK_FALSE),

			// Albedo attachment blend state
			vk::PipelineColorBlendAttachmentState{}
				.setColorWriteMask(vk::ColorComponentFlagBits::eR |
								  vk::ColorComponentFlagBits::eG |
								  vk::ColorComponentFlagBits::eB |
								  vk::ColorComponentFlagBits::eA)
				.setBlendEnable(VK_FALSE),
			// Albedo attachment blend state
			vk::PipelineColorBlendAttachmentState{}
				.setColorWriteMask(vk::ColorComponentFlagBits::eR |
								  vk::ColorComponentFlagBits::eG |
								  vk::ColorComponentFlagBits::eB |
								  vk::ColorComponentFlagBits::eA)
				.setBlendEnable(VK_FALSE),
			// Albedo attachment blend state
			vk::PipelineColorBlendAttachmentState{}
				.setColorWriteMask(vk::ColorComponentFlagBits::eR |
								  vk::ColorComponentFlagBits::eG |
								  vk::ColorComponentFlagBits::eB |
								  vk::ColorComponentFlagBits::eA)
				.setBlendEnable(VK_FALSE),
			// Albedo attachment blend state
		vk::PipelineColorBlendAttachmentState{}
			.setColorWriteMask(vk::ColorComponentFlagBits::eR |
							  vk::ColorComponentFlagBits::eG |
							  vk::ColorComponentFlagBits::eB |
							  vk::ColorComponentFlagBits::eA)
			.setBlendEnable(VK_FALSE),
			// Albedo attachment blend state
		vk::PipelineColorBlendAttachmentState{}
			.setColorWriteMask(vk::ColorComponentFlagBits::eR |
							  vk::ColorComponentFlagBits::eG |
							  vk::ColorComponentFlagBits::eB |
							  vk::ColorComponentFlagBits::eA)
			.setBlendEnable(VK_FALSE),

					vk::PipelineColorBlendAttachmentState{}
			.setColorWriteMask(vk::ColorComponentFlagBits::eR |
							  vk::ColorComponentFlagBits::eG |
							  vk::ColorComponentFlagBits::eB |
							  vk::ColorComponentFlagBits::eA)
			.setBlendEnable(VK_FALSE)
		};

		vk::PipelineColorBlendStateCreateInfo colorBlend{};
		colorBlend.setLogicOpEnable(VK_FALSE);
		colorBlend.setAttachmentCount(colorBlendAttachments.size());
		colorBlend.setPAttachments(colorBlendAttachments.data());

		geometryPassPipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		geometryPassPipeline = pipelineManager.createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, geometryPassPipelineLayout);

		vulkanContext.LogicalDevice.destroyShaderModule(VertShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(FragShaderModule);
	}

	///////////////////////////////////////////RAY TRACING PIPELINES////////////////////////////////////////////////////////////////
	{
	
		auto RayGen_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/ReSTIR_DI_Raygen.rgen.spv");
		auto RayClosestHit_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/ReSTIR_DI_ClosestHit.rchit.spv");
		auto RayGenMiss_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/ReSTIRDI_Miss.rmiss.spv");

		VkShaderModule RayGen_ShaderModule = pipelineManager.createShaderModule(RayGen_ShaderCode);
		VkShaderModule RayClosestHit_ShaderModule = pipelineManager.createShaderModule(RayClosestHit_ShaderCode);
		VkShaderModule RayMiss_ShaderModule = pipelineManager.createShaderModule(RayGenMiss_ShaderCode);


		vk::PipelineShaderStageCreateInfo RayGen_ShaderStageInfo{};
		RayGen_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayGen_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eRaygenKHR;
		RayGen_ShaderStageInfo.module = RayGen_ShaderModule;
		RayGen_ShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo RayClosestHit_ShaderStageInfo{};
		RayClosestHit_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayClosestHit_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eClosestHitKHR;
		RayClosestHit_ShaderStageInfo.module = RayClosestHit_ShaderModule;
		RayClosestHit_ShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo RayMiss_ShaderStageInfo{};
		RayMiss_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayMiss_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eMissKHR;
		RayMiss_ShaderStageInfo.module = RayMiss_ShaderModule;
		RayMiss_ShaderStageInfo.pName = "main";

		std::vector<vk::PipelineShaderStageCreateInfo> ShaderStages = { RayGen_ShaderStageInfo ,
																		RayClosestHit_ShaderStageInfo,
																		RayMiss_ShaderStageInfo };

		vk::RayTracingShaderGroupCreateInfoKHR RayGen_GroupInfo{};
		RayGen_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		RayGen_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		RayGen_GroupInfo.generalShader = 0;
		RayGen_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		RayGen_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		RayGen_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		vk::RayTracingShaderGroupCreateInfoKHR RayClosestHit_GroupInfo{};
		RayClosestHit_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		RayClosestHit_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
		RayClosestHit_GroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
		RayClosestHit_GroupInfo.closestHitShader = 1;
		RayClosestHit_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		RayClosestHit_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		vk::RayTracingShaderGroupCreateInfoKHR Miss_GroupInfo{};
		Miss_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		Miss_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		Miss_GroupInfo.generalShader = 2;
		Miss_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		Miss_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		Miss_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;


		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> ShaderGroups = {
			RayGen_GroupInfo,
			RayClosestHit_GroupInfo,
			Miss_GroupInfo,
			Miss_GroupInfo,
		};

		vk::PushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstant);

		vk::DescriptorSetLayout layouts[2] = { Restir_DI->RayTracingDescriptorSetLayout ,Restir_DI->DDGIATLASDescriptorSetLayout };

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 2;
		pipelineLayoutInfo.pSetLayouts = layouts;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		ReSTIR_RT_PipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		ReSTIR_RTPassPipeline = pipelineManager.createRayTracingGraphicsPipeline(ReSTIR_RT_PipelineLayout, ShaderStages, ShaderGroups);

		vulkanContext.LogicalDevice.destroyShaderModule(RayGen_ShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(RayClosestHit_ShaderModule); 
		vulkanContext.LogicalDevice.destroyShaderModule(RayMiss_ShaderModule);
	}

	{
		auto RayGen_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/Reflection_Raygen.rgen.spv");
		auto RayClosestHit_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/Reflection_ClosestHit.rchit.spv");
		auto RayGenMiss_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/Reflection_Miss.rmiss.spv");

		VkShaderModule RayGen_ShaderModule = pipelineManager.createShaderModule(RayGen_ShaderCode);
		VkShaderModule RayClosestHit_ShaderModule = pipelineManager.createShaderModule(RayClosestHit_ShaderCode);
		VkShaderModule RayMiss_ShaderModule = pipelineManager.createShaderModule(RayGenMiss_ShaderCode);


		vk::PipelineShaderStageCreateInfo RayGen_ShaderStageInfo{};
		RayGen_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayGen_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eRaygenKHR;
		RayGen_ShaderStageInfo.module = RayGen_ShaderModule;
		RayGen_ShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo RayClosestHit_ShaderStageInfo{};
		RayClosestHit_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayClosestHit_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eClosestHitKHR;
		RayClosestHit_ShaderStageInfo.module = RayClosestHit_ShaderModule;
		RayClosestHit_ShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo RayMiss_ShaderStageInfo{};
		RayMiss_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayMiss_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eMissKHR;
		RayMiss_ShaderStageInfo.module = RayMiss_ShaderModule;
		RayMiss_ShaderStageInfo.pName = "main";

		std::vector<vk::PipelineShaderStageCreateInfo> ShaderStages = { RayGen_ShaderStageInfo ,
																		RayClosestHit_ShaderStageInfo,
																		RayMiss_ShaderStageInfo };

		vk::RayTracingShaderGroupCreateInfoKHR RayGen_GroupInfo{};
		RayGen_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		RayGen_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		RayGen_GroupInfo.generalShader = 0;
		RayGen_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		RayGen_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		RayGen_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		vk::RayTracingShaderGroupCreateInfoKHR RayClosestHit_GroupInfo{};
		RayClosestHit_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		RayClosestHit_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
		RayClosestHit_GroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
		RayClosestHit_GroupInfo.closestHitShader = 1;
		RayClosestHit_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		RayClosestHit_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		vk::RayTracingShaderGroupCreateInfoKHR Miss_GroupInfo{};
		Miss_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		Miss_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		Miss_GroupInfo.generalShader = 2;
		Miss_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		Miss_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		Miss_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;


		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> ShaderGroups = {
			RayGen_GroupInfo,
			RayClosestHit_GroupInfo,
			Miss_GroupInfo,
			Miss_GroupInfo,
		};
		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(ReflectionsFlags));
		range.setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &RT_Reflection->RayTracingDescriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		RT_ReflectionPipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		RT_ReflectionPassPipeline = pipelineManager.createRayTracingGraphicsPipeline(RT_ReflectionPipelineLayout, ShaderStages, ShaderGroups);

		vulkanContext.LogicalDevice.destroyShaderModule(RayGen_ShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(RayClosestHit_ShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(RayMiss_ShaderModule);

	}

	{
		auto RayGen_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/Lighting_Raygen.rgen.spv");
		auto RayClosestHit_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/Lighting_ClosestHit.rchit.spv");
		auto RayGenMiss_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/Lighting_Miss.rmiss.spv");

		VkShaderModule RayGen_ShaderModule = pipelineManager.createShaderModule(RayGen_ShaderCode);
		VkShaderModule RayClosestHit_ShaderModule = pipelineManager.createShaderModule(RayClosestHit_ShaderCode);
		VkShaderModule RayMiss_ShaderModule = pipelineManager.createShaderModule(RayGenMiss_ShaderCode);


		vk::PipelineShaderStageCreateInfo RayGen_ShaderStageInfo{};
		RayGen_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayGen_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eRaygenKHR;
		RayGen_ShaderStageInfo.module = RayGen_ShaderModule;
		RayGen_ShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo RayClosestHit_ShaderStageInfo{};
		RayClosestHit_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayClosestHit_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eClosestHitKHR;
		RayClosestHit_ShaderStageInfo.module = RayClosestHit_ShaderModule;
		RayClosestHit_ShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo RayMiss_ShaderStageInfo{};
		RayMiss_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayMiss_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eMissKHR;
		RayMiss_ShaderStageInfo.module = RayMiss_ShaderModule;
		RayMiss_ShaderStageInfo.pName = "main";

		std::vector<vk::PipelineShaderStageCreateInfo> ShaderStages = { RayGen_ShaderStageInfo ,
																		RayClosestHit_ShaderStageInfo,
																		RayMiss_ShaderStageInfo };

		vk::RayTracingShaderGroupCreateInfoKHR RayGen_GroupInfo{};
		RayGen_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		RayGen_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		RayGen_GroupInfo.generalShader = 0;
		RayGen_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		RayGen_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		RayGen_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		vk::RayTracingShaderGroupCreateInfoKHR RayClosestHit_GroupInfo{};
		RayClosestHit_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		RayClosestHit_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
		RayClosestHit_GroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
		RayClosestHit_GroupInfo.closestHitShader = 1;
		RayClosestHit_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		RayClosestHit_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		vk::RayTracingShaderGroupCreateInfoKHR Miss_GroupInfo{};
		Miss_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		Miss_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		Miss_GroupInfo.generalShader = 2;
		Miss_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		Miss_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		Miss_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;


		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> ShaderGroups = {
			RayGen_GroupInfo,
			RayClosestHit_GroupInfo,
			Miss_GroupInfo,
			Miss_GroupInfo,
		};

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(int));
		range.setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &lighting_RTX->descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		DeferedLightingPassPipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		DeferedLightingPassPipeline = pipelineManager.createRayTracingGraphicsPipeline(DeferedLightingPassPipelineLayout, ShaderStages, ShaderGroups);

		vulkanContext.LogicalDevice.destroyShaderModule(RayGen_ShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(RayClosestHit_ShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(RayMiss_ShaderModule);

	}

	{
		auto RayGen_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/DDGI_Raygen.rgen.spv");
		auto RayClosestHit_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/DDGI_ClosestHit.rchit.spv");
		auto RayGenMiss_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/DDGI_Miss.rmiss.spv");
		auto ShadowMiss_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/DDGI_Shadow_Miss.rmiss.spv");

		VkShaderModule RayGen_ShaderModule = pipelineManager.createShaderModule(RayGen_ShaderCode);
		VkShaderModule RayClosestHit_ShaderModule = pipelineManager.createShaderModule(RayClosestHit_ShaderCode);
		VkShaderModule RayMiss_ShaderModule = pipelineManager.createShaderModule(RayGenMiss_ShaderCode);
		VkShaderModule ShadowMiss_ShaderModule = pipelineManager.createShaderModule(ShadowMiss_ShaderCode);

		vk::PipelineShaderStageCreateInfo RayGen_ShaderStageInfo{};
		RayGen_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayGen_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eRaygenKHR;
		RayGen_ShaderStageInfo.module = RayGen_ShaderModule;
		RayGen_ShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo RayClosestHit_ShaderStageInfo{};
		RayClosestHit_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayClosestHit_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eClosestHitKHR;
		RayClosestHit_ShaderStageInfo.module = RayClosestHit_ShaderModule;
		RayClosestHit_ShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo RayMiss_ShaderStageInfo{};
		RayMiss_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayMiss_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eMissKHR;
		RayMiss_ShaderStageInfo.module = RayMiss_ShaderModule;
		RayMiss_ShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo ShadowMiss_ShaderStageInfo{};
		ShadowMiss_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		ShadowMiss_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eMissKHR;
		ShadowMiss_ShaderStageInfo.module = ShadowMiss_ShaderModule;
		ShadowMiss_ShaderStageInfo.pName = "main";

		std::vector<vk::PipelineShaderStageCreateInfo> ShaderStages = {
			RayGen_ShaderStageInfo,       
			RayClosestHit_ShaderStageInfo,
			RayMiss_ShaderStageInfo,      
			ShadowMiss_ShaderStageInfo    
		};


		vk::RayTracingShaderGroupCreateInfoKHR RayGen_GroupInfo{};
		RayGen_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		RayGen_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		RayGen_GroupInfo.generalShader = 0; 
		RayGen_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		RayGen_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		RayGen_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		vk::RayTracingShaderGroupCreateInfoKHR RayClosestHit_GroupInfo{};
		RayClosestHit_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		RayClosestHit_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
		RayClosestHit_GroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
		RayClosestHit_GroupInfo.closestHitShader = 1; 
		RayClosestHit_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		RayClosestHit_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		vk::RayTracingShaderGroupCreateInfoKHR Miss_GroupInfo{};
		Miss_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		Miss_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		Miss_GroupInfo.generalShader = 2;
		Miss_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		Miss_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		Miss_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		vk::RayTracingShaderGroupCreateInfoKHR ShadowMiss_GroupInfo{};
		ShadowMiss_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
		ShadowMiss_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		ShadowMiss_GroupInfo.generalShader = 3; 
		ShadowMiss_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		ShadowMiss_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		ShadowMiss_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> ShaderGroups = {
			RayGen_GroupInfo,        
			RayClosestHit_GroupInfo, 
			Miss_GroupInfo,          
			ShadowMiss_GroupInfo,    
		};

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(RTpcInfo));
		range.setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &dynamicDiffuse_RTGI->RaytracingDescriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		RT_DDGIPipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		RT_DDGIPassPipeline = pipelineManager.createRayTracingGraphicsPipeline(RT_DDGIPipelineLayout, ShaderStages, ShaderGroups);

		vulkanContext.LogicalDevice.destroyShaderModule(RayGen_ShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(RayClosestHit_ShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(RayMiss_ShaderModule);
		vulkanContext.LogicalDevice.destroyShaderModule(ShadowMiss_ShaderModule);


	}
	{

		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanContext.swapchainformat;

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(int));
		range.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(SSGI_FullScreenQuad->TemporalAccumilationDescriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;


		FullScreen_Quad_Pipeline_Data  Temp = pipelineManager.create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/TemporalAccumulation.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);

		TA_SSGIPipelineLayout = Temp.FQ_PipelineLayout;
		TA_SSGIPipeline = Temp.FQ_Pipeline;
	}


	{

		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanContext.swapchainformat;

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(int));
		range.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(SSGI_FullScreenQuad->Blured_TemporalAccumilationDescriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;


		FullScreen_Quad_Pipeline_Data  Temp = pipelineManager.create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/SSGI_Blur_Shader.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);

		BluredSSGIPipelineLayout = Temp.FQ_PipelineLayout;
		BluredSSGIPipeline = Temp.FQ_Pipeline;
	}


	{
		std::array<vk::Format, 1> colorFormats = { vk::Format::eR16G16B16A16Sfloat };

		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(int));
		range.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(RT_Reflection->BlurDescriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;


		FullScreen_Quad_Pipeline_Data  Temp = pipelineManager.create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/RT-ReflectionI_Blur_Shader.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);

		BluredRTreflectionsPipelineLayout = Temp.FQ_PipelineLayout;
		BluredRTreflectionPipeline = Temp.FQ_Pipeline;
	}


	{
		auto ComputeShaderCode = readFile("../Shaders/Compiled_Shader_Files/Grid.comp.spv");

		VkShaderModule ComputeShaderModule = pipelineManager.createShaderModule(ComputeShaderCode);

		vk::PipelineShaderStageCreateInfo ComputeShaderStageInfo{};
		ComputeShaderStageInfo.sType  = vk::StructureType::ePipelineShaderStageCreateInfo;
		ComputeShaderStageInfo.stage  = vk::ShaderStageFlagBits::eCompute;
		ComputeShaderStageInfo.module = ComputeShaderModule;
		ComputeShaderStageInfo.pName  = "main";

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(GridData));
		range.setStageFlags(vk::ShaderStageFlagBits::eCompute);


		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &dynamicDiffuse_RTGI->GridDescriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		GridComputePipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		GridComputePassPipeline = pipelineManager.creatComputePipeline(GridComputePipelineLayout,ComputeShaderStageInfo);

		vulkanContext.LogicalDevice.destroyShaderModule(ComputeShaderModule);
	}

	{
		auto ComputeShaderCode = readFile("../Shaders/Compiled_Shader_Files/Irradiance_Visibility.comp.spv");

		VkShaderModule ComputeShaderModule = pipelineManager.createShaderModule(ComputeShaderCode);

		vk::PipelineShaderStageCreateInfo ComputeShaderStageInfo{};
		ComputeShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		ComputeShaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
		ComputeShaderStageInfo.module = ComputeShaderModule;
		ComputeShaderStageInfo.pName = "main";

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(GeneralAtlasInfo));
		range.setStageFlags(vk::ShaderStageFlagBits::eCompute);


		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &dynamicDiffuse_RTGI->ConstructProbeDataDescriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		IrradianceComputePipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		IrradianceComputePassPipeline = pipelineManager.creatComputePipeline(IrradianceComputePipelineLayout, ComputeShaderStageInfo);

		vulkanContext.LogicalDevice.destroyShaderModule(ComputeShaderModule);
	}

	{
		auto ComputeShaderCode = readFile("../Shaders/Compiled_Shader_Files/ProbeStatus.comp.spv");

		VkShaderModule ComputeShaderModule = pipelineManager.createShaderModule(ComputeShaderCode);

		vk::PipelineShaderStageCreateInfo ComputeShaderStageInfo{};
		ComputeShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		ComputeShaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
		ComputeShaderStageInfo.module = ComputeShaderModule;
		ComputeShaderStageInfo.pName = "main";

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(GeneralAtlasInfo));
		range.setStageFlags(vk::ShaderStageFlagBits::eCompute);


		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &dynamicDiffuse_RTGI->ProbeStatusDescriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		ProbeStatusPipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		ProbeStatusComputePassPipeline = pipelineManager.creatComputePipeline(ProbeStatusPipelineLayout, ComputeShaderStageInfo);

		vulkanContext.LogicalDevice.destroyShaderModule(ComputeShaderModule);
	}


	{
		auto ComputeShaderCode = readFile("../Shaders/Compiled_Shader_Files/Sample_GI_Probes.comp.spv");

		VkShaderModule ComputeShaderModule = pipelineManager.createShaderModule(ComputeShaderCode);

		vk::PipelineShaderStageCreateInfo ComputeShaderStageInfo{};
		ComputeShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		ComputeShaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
		ComputeShaderStageInfo.module = ComputeShaderModule;
		ComputeShaderStageInfo.pName = "main";

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(SampleGridInfo));
		range.setStageFlags(vk::ShaderStageFlagBits::eCompute);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &dynamicDiffuse_RTGI->DDGISamplingDescriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		SampleDDGIComputePipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		SampleDDGIComputePassPipeline = pipelineManager.creatComputePipeline(SampleDDGIComputePipelineLayout, ComputeShaderStageInfo);

		vulkanContext.LogicalDevice.destroyShaderModule(ComputeShaderModule);
	}


	{
		auto ComputeShaderCode = readFile("../Shaders/Compiled_Shader_Files/SSGI.comp.spv");

		VkShaderModule ComputeShaderModule = pipelineManager.createShaderModule(ComputeShaderCode);

		vk::PipelineShaderStageCreateInfo ComputeShaderStageInfo{};
		ComputeShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		ComputeShaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
		ComputeShaderStageInfo.module = ComputeShaderModule;
		ComputeShaderStageInfo.pName = "main";


		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &SSGI_FullScreenQuad->descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		SSGIPipelineLayout = vulkanContext.LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		SSGIPipeline = pipelineManager.creatComputePipeline(SSGIPipelineLayout, ComputeShaderStageInfo);

		vulkanContext.LogicalDevice.destroyShaderModule(ComputeShaderModule);
	}


}

uint32_t App::alignedSize(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

void App::createShaderBindingTable() {

	///Binding Table for Reflection
	{
		const size_t   handleSize = vulkanContext.RayTracingPipelineProperties.shaderGroupHandleSize;
		const size_t   handleSizeAligned = alignedSize(handleSize, vulkanContext.RayTracingPipelineProperties.shaderGroupHandleAlignment);
		const uint32_t groupcount = 3;
		const uint32_t sbtSize = groupcount * handleSizeAligned;

		// Get shader group handles
		std::vector<uint8_t> shaderHandleStorage(sbtSize);

		vulkanContext.vkGetRayTracingShaderGroupHandlesKHR(
			static_cast<VkDevice>(vulkanContext.LogicalDevice),
			static_cast<VkPipeline>(RT_ReflectionPassPipeline),
			0,  // First group
			groupcount,
			shaderHandleStorage.size(),
			shaderHandleStorage.data());

		Reflection_raygenShaderBindingTableBuffer.BufferID = "Reflection raygen Shader Binding Table Buffer";
		Reflection_missShaderBindingTableBuffer.BufferID   = "Reflection miss Shader Binding Table Buffer";
		Reflection_hitShaderBindingTableBuffer.BufferID    = "Reflection hit Shader Binding Table Buffer";

		bufferManger.CreateBuffer(&Reflection_raygenShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);
		bufferManger.CreateBuffer(&Reflection_missShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);
		bufferManger.CreateBuffer(&Reflection_hitShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);

		bufferManger.CopyDataToBuffer(shaderHandleStorage.data(), Reflection_raygenShaderBindingTableBuffer);
		bufferManger.CopyDataToBuffer(shaderHandleStorage.data() + handleSizeAligned, Reflection_hitShaderBindingTableBuffer);
		bufferManger.CopyDataToBuffer(shaderHandleStorage.data() + handleSizeAligned * 2, Reflection_missShaderBindingTableBuffer);
	}

	{
		const size_t   handleSize = vulkanContext.RayTracingPipelineProperties.shaderGroupHandleSize;
		const size_t   handleSizeAligned = alignedSize(handleSize, vulkanContext.RayTracingPipelineProperties.shaderGroupHandleAlignment);
		const uint32_t groupcount = 3;
		const uint32_t sbtSize = groupcount * handleSizeAligned;

		// Get shader group handles
		std::vector<uint8_t> shaderHandleStorage(sbtSize);

		vulkanContext.vkGetRayTracingShaderGroupHandlesKHR(
			static_cast<VkDevice>(vulkanContext.LogicalDevice),
			static_cast<VkPipeline>(DeferedLightingPassPipeline),
			0,  // First group
			groupcount,
			shaderHandleStorage.size(),
			shaderHandleStorage.data());

		Lighting_raygenShaderBindingTableBuffer.BufferID = "Lighting raygen Shader Binding Table Buffer";
		Lighting_missShaderBindingTableBuffer.BufferID   = "Lighting miss Shader Binding Table Buffer";
		Lighting_hitShaderBindingTableBuffer.BufferID    = "Lighting hit Shader Binding Table Buffer";

		bufferManger.CreateBuffer(&Lighting_raygenShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);
		bufferManger.CreateBuffer(&Lighting_missShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);
		bufferManger.CreateBuffer(&Lighting_hitShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);

		bufferManger.CopyDataToBuffer(shaderHandleStorage.data(), Lighting_raygenShaderBindingTableBuffer);
		bufferManger.CopyDataToBuffer(shaderHandleStorage.data() + handleSizeAligned, Lighting_hitShaderBindingTableBuffer);
		bufferManger.CopyDataToBuffer(shaderHandleStorage.data() + handleSizeAligned * 2, Lighting_missShaderBindingTableBuffer);
	}


	///Binding Table for DDGI
	{
		const size_t handleSize = vulkanContext.RayTracingPipelineProperties.shaderGroupHandleSize;
		const size_t handleSizeAligned = alignedSize(handleSize, vulkanContext.RayTracingPipelineProperties.shaderGroupHandleAlignment);

		const uint32_t groupcount = 4;
		const uint32_t sbtSize = groupcount * handleSizeAligned;

		std::vector<uint8_t> shaderHandleStorage(sbtSize);

		vulkanContext.vkGetRayTracingShaderGroupHandlesKHR(
			static_cast<VkDevice>(vulkanContext.LogicalDevice),
			static_cast<VkPipeline>(RT_DDGIPassPipeline),
			0,
			groupcount,
			shaderHandleStorage.size(),
			shaderHandleStorage.data());

		DDGI_raygenShaderBindingTableBuffer.BufferID = "DDGI raygen Shader Binding Table Buffer";
		DDGI_missShaderBindingTableBuffer.BufferID = "DDGI miss Shader Binding Table Buffer";
		DDGI_hitShaderBindingTableBuffer.BufferID = "DDGI hit Shader Binding Table Buffer";

		bufferManger.CreateBuffer(&DDGI_raygenShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);

		bufferManger.CreateBuffer(&DDGI_hitShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);

		bufferManger.CreateBuffer(&DDGI_missShaderBindingTableBuffer, handleSizeAligned * 2, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);

		bufferManger.CopyDataToBuffer(shaderHandleStorage.data(), DDGI_raygenShaderBindingTableBuffer);

		bufferManger.CopyDataToBuffer(shaderHandleStorage.data() + handleSizeAligned, DDGI_hitShaderBindingTableBuffer);

		std::vector<uint8_t> missHandlesCombined(handleSizeAligned * 2);
		memcpy(missHandlesCombined.data(), shaderHandleStorage.data() + (handleSizeAligned * 2), handleSizeAligned * 2);

		bufferManger.CopyDataToBuffer(missHandlesCombined.data(), DDGI_missShaderBindingTableBuffer);
	}


	{
		const size_t   handleSize = vulkanContext.RayTracingPipelineProperties.shaderGroupHandleSize;
		const size_t   handleSizeAligned = alignedSize(handleSize, vulkanContext.RayTracingPipelineProperties.shaderGroupHandleAlignment);
		const uint32_t groupcount = 3;
		const uint32_t sbtSize = groupcount * handleSizeAligned;

		// Get shader group handles
		std::vector<uint8_t> shaderHandleStorage(sbtSize);

		vulkanContext.vkGetRayTracingShaderGroupHandlesKHR(
			static_cast<VkDevice>(vulkanContext.LogicalDevice),
			static_cast<VkPipeline>(ReSTIR_RTPassPipeline),
			0,  // First group
			groupcount,
			shaderHandleStorage.size(),
			shaderHandleStorage.data());

		ReSTIR_DI_raygenShaderBindingTableBuffer.BufferID = "ReSTIR_DI raygen Shader Binding Table Buffer";
		ReSTIR_DI_missShaderBindingTableBuffer.BufferID   = "ReSTIR_DI miss Shader Binding Table Buffer";
		ReSTIR_DI_hitShaderBindingTableBuffer.BufferID    = "ReSTIR_DI hit Shader Binding Table Buffer";

		bufferManger.CreateBuffer(&ReSTIR_DI_raygenShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);
		bufferManger.CreateBuffer(&ReSTIR_DI_missShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);
		bufferManger.CreateBuffer(&ReSTIR_DI_hitShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext.graphicsQueue);

		bufferManger.CopyDataToBuffer(shaderHandleStorage.data(), ReSTIR_DI_raygenShaderBindingTableBuffer);
		bufferManger.CopyDataToBuffer(shaderHandleStorage.data() + handleSizeAligned, ReSTIR_DI_hitShaderBindingTableBuffer);
		bufferManger.CopyDataToBuffer(shaderHandleStorage.data() + handleSizeAligned * 2, ReSTIR_DI_missShaderBindingTableBuffer);
	}

}

void App::DestroyShaderBindingTable() {

	bufferManger.DestroyBuffer(Reflection_raygenShaderBindingTableBuffer);
	bufferManger.DestroyBuffer(Reflection_missShaderBindingTableBuffer);
	bufferManger.DestroyBuffer(Reflection_hitShaderBindingTableBuffer);


	bufferManger.DestroyBuffer(DDGI_raygenShaderBindingTableBuffer);
	bufferManger.DestroyBuffer(DDGI_missShaderBindingTableBuffer);
	bufferManger.DestroyBuffer(DDGI_hitShaderBindingTableBuffer);

	bufferManger.DestroyBuffer(ReSTIR_DI_raygenShaderBindingTableBuffer);
	bufferManger.DestroyBuffer(ReSTIR_DI_missShaderBindingTableBuffer);
	bufferManger.DestroyBuffer(ReSTIR_DI_hitShaderBindingTableBuffer);


	bufferManger.DestroyBuffer(Lighting_raygenShaderBindingTableBuffer);
	bufferManger.DestroyBuffer(Lighting_missShaderBindingTableBuffer);
	bufferManger.DestroyBuffer(Lighting_hitShaderBindingTableBuffer);
}




void App::createCommandPool()
{ 
	vk::CommandPoolCreateInfo poolInfo{};
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	poolInfo.queueFamilyIndex = vulkanContext.graphicsQueueFamilyIndex;

	commandPool = vulkanContext.LogicalDevice.createCommandPool(poolInfo);

	if (!commandPool)
	{
		throw std::runtime_error("failed to create command pool!");

	}

}



void App::createCommandBuffer()
{
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	vk::CommandBufferAllocateInfo allocateInfo{};
	allocateInfo.commandPool = commandPool;
	allocateInfo.level = vk::CommandBufferLevel::ePrimary;
	allocateInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	commandBuffers = vulkanContext.LogicalDevice.allocateCommandBuffers(allocateInfo);

	if (commandBuffers.empty())
	{
		throw std::runtime_error("failed to create command Buffer!");

	}


}
void App::createSyncObjects() {
	// Present complete semaphores - one per swapchain image
	presentCompleteSemaphores.resize(vulkanContext.swapchainImageData.size());

	// Render complete semaphores 
	renderCompleteSemaphores.resize(vulkanContext.swapchainImageData.size());

	// Fences - one per frame in flight
	waitFences.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < vulkanContext.swapchainImageData.size(); i++) {
		vk::SemaphoreCreateInfo semaphoreInfo{};
		vulkanContext.LogicalDevice.createSemaphore(&semaphoreInfo, nullptr, &presentCompleteSemaphores[i]);
		vulkanContext.LogicalDevice.createSemaphore(&semaphoreInfo, nullptr, &renderCompleteSemaphores[i]);
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::FenceCreateInfo fenceInfo{};
		fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
		vulkanContext.LogicalDevice.createFence(&fenceInfo, nullptr, &waitFences[i]);
	}
}

void App::Run()
{
	FramesPerSecondCounter fpsCounter(0.1f);

	while (!window.shouldClose())
	{
		for (auto& model : Models) {
			model->UpdateHistory();
		}

		// 2. INPUT & LOGIC
		glfwPollEvents();
		CalculateFps(fpsCounter);

		camera.OnFrameStart();
		camera.Update(deltaTime); 

		userinterface.DrawUi(this, skyBox.get());

		Draw();
	}

	vulkanContext.LogicalDevice.waitIdle();
}

void App::CalculateFps(FramesPerSecondCounter& fpsCounter)
{
		const double newTimeStamp = glfwGetTime();
		deltaTime = static_cast<float>(newTimeStamp - LasttimeStamp);
		LasttimeStamp = newTimeStamp;
		fpsCounter.tick(deltaTime);
}
void App::Draw()
{

	// ZoneScopedN("render"); // Tracy profiling marker (commented out)

	// --- CPU-GPU Synchronization ---
	// Wait for the fence associated with the current frame to ensure the command buffer 
	// from this frame index is no longer in use before we reuse it.
	// This prevents the CPU from getting too far ahead of the GPU (frames in flight control)
	vulkanContext.LogicalDevice.waitForFences(1, &waitFences[currentFrame], vk::True, UINT64_MAX);
	vulkanContext.LogicalDevice.resetFences(1, &waitFences[currentFrame]);

	// --- Swapchain Acquisition ---
	uint32_t imageIndex;
	try {
		// Request the next available swapchain image.
		// presentCompleteSemaphores[currentFrame] will be signaled when the image is ready.
		vulkanContext.LogicalDevice.acquireNextImageKHR(
			vulkanContext.swapChain,
			UINT64_MAX,
			presentCompleteSemaphores[currentFrame],
			nullptr,
			&imageIndex
		);
	}
	catch (const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		std::cerr << "Attempting to recreate swap chain..." << std::endl;
		recreateSwapChain();
		framebufferResized = false;
		return; 
	}

	// --- Frame Preparation ---
	updateUniformBuffer(currentFrame);  // Update uniform data for this frame
	recordCommandBuffer(commandBuffers[currentFrame], imageIndex);  // Record commands using the acquired image

	// --- GPU-GPU Synchronization ---
	// The graphics queue will wait at the color attachment stage until the image is available
	vk::Semaphore waitSemaphores[] = { presentCompleteSemaphores[currentFrame] };

	vk::PipelineStageFlags waitStages[] = {
		    vk::PipelineStageFlagBits::eAllCommands
	};

	// This semaphore will be signaled when rendering completes.
	// CRITICAL: Uses imageIndex because presentation engine needs per-image synchronization.
	// By the time we reuse this imageIndex, we know presentation is done with its semaphore.
	vk::Semaphore submitSemaphores[] = { renderCompleteSemaphores[imageIndex] };

	vk::SubmitInfo submitInfo{};
	submitInfo.sType                = vk::StructureType::eSubmitInfo;
	submitInfo.waitSemaphoreCount   = 1;
	submitInfo.pWaitSemaphores      = waitSemaphores;  // Wait for image acquisition
	submitInfo.pWaitDstStageMask    = waitStages;     // Wait at color attachment stage
	submitInfo.commandBufferCount   = 1;
	submitInfo.pCommandBuffers      = &commandBuffers[currentFrame];  // Frame-specific CB
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores    = submitSemaphores;  // Signal when rendering done

	// Submit to the graphics queue with the current frame's fence
	if (vulkanContext.graphicsQueue.submit(1, &submitInfo, waitFences[currentFrame]) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to submit draw commands");
	}

	// --- Presentation ---
	vk::SwapchainKHR swapchains[] = { vulkanContext.swapChain };

	vk::PresentInfoKHR presentInfo{};
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores    =  submitSemaphores;  // Wait for rendering completion
	presentInfo.swapchainCount     = 1;
	presentInfo.pSwapchains        = swapchains;
	presentInfo.pImageIndices      = &imageIndex;

	try {
		// Present the image - will wait on renderCompleteSemaphores[imageIndex]
		vk::Result result = vulkanContext.presentQueue.presentKHR(presentInfo);

	}
	catch (const vk::OutOfDateKHRError& e) {
		// Handle swapchain out-of-date or other errors
		std::cerr << "Exception: " << e.what() << std::endl;
		std::cerr << "Attempting to recreate swap chain..." << std::endl;
		recreateSwapChain();
		framebufferResized = false;
	}

	// Advance to the next frame index (wraps based on MAX_FRAMES_IN_FLIGHT)
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void App::updateUniformBuffer(uint32_t currentImage) {
	
	UpdateTLAS();

	for (auto& light : lights)
	{
		light->UpdateUniformBuffer(currentImage);
	}


	for (auto& model : Models)
	{
		model->UpdateUniformBuffer(currentImage);
	}

	skyBox->UpdateUniformBuffer(currentImage);

	lighting_RTX->UpdateUniformBuffer(currentImage, lights);
	ssao_FullScreenQuad->UpdataeUniformBufferData();
    SSGI_FullScreenQuad->UpdateUniformBuffer(currentImage, lights,deltaTime);
	RT_Reflection->UpdateUniformBuffer(currentImage, lights, Models);

	bool ddgiRecreated = dynamicDiffuse_RTGI->UpdateUniformBuffer(DescriptorPool, TLAS, lighting_RTX->UniformBuffers,gbuffer,false, lights.size());

	if (ddgiRecreated)
	{

		DDGIIrradianceAtlasID = ImGui_ImplVulkan_AddTexture(
			dynamicDiffuse_RTGI->IradianceImageAtlasImage.imageSampler,
			dynamicDiffuse_RTGI->IradianceImageAtlasImage.imageView,
			VK_IMAGE_LAYOUT_GENERAL
		);

		DDGIIVisibilityAtlasID = ImGui_ImplVulkan_AddTexture(
			dynamicDiffuse_RTGI->VisibilityImageAtlasImage.imageSampler,
			dynamicDiffuse_RTGI->VisibilityImageAtlasImage.imageView,
			VK_IMAGE_LAYOUT_GENERAL
		);


		Restir_DI->createDescriptorDDGIATLAS(DescriptorPool);
	}
}

void  App::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) {


	vk::CommandBufferBeginInfo begininfo{};
	begininfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	commandBuffer.begin(begininfo);

	vk::ClearValue clearColor{};
	clearColor.color = { 0.0f, 0.0f, 0.0f, 0.0f };

	VkOffset2D imageoffset = { 0, 0 };

	vk::Extent3D swapchainextent = vk::Extent3D(vulkanContext.swapchainExtent.width, vulkanContext.swapchainExtent.height, 1);

	vk::Viewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width =  vulkanContext.swapchainExtent.width;
	viewport.height = vulkanContext.swapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor{};
	scissor.offset = imageoffset;
	scissor.extent.width =  vulkanContext.swapchainExtent.width;
	scissor.extent.height = vulkanContext.swapchainExtent.height;


	vk::DeviceSize offsets[] = { 0 };

	
	vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer,Gbuffer_Label);

	 /////////////////// GBUFFER PASS ///////////////////////// 
	{
	

	    ImageTransitionData TransitiontoGeneraRT{};
	    TransitiontoGeneraRT.oldlayout = vk::ImageLayout::eUndefined;
	    TransitiontoGeneraRT.newlayout = vk::ImageLayout::eGeneral;
	    TransitiontoGeneraRT.AspectFlag = vk::ImageAspectFlagBits::eColor;
	    TransitiontoGeneraRT.SourceAccessflag = vk::AccessFlagBits::eNone;
	    TransitiontoGeneraRT.DestinationAccessflag = vk::AccessFlagBits::eShaderWrite;
	    TransitiontoGeneraRT.SourceOnThePipeline = vk::PipelineStageFlagBits::eNone;
	    TransitiontoGeneraRT.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;
	    
	    bufferManger.TransitionImage(commandBuffer, &gbuffer.Normal   , TransitiontoGeneraRT);
	    bufferManger.TransitionImage(commandBuffer, &gbuffer.PrevNormal, TransitiontoGeneraRT);
	    
	    vk::ImageSubresourceLayers SrcSubresourceLayers;
	    SrcSubresourceLayers.mipLevel = 0;
	    SrcSubresourceLayers.baseArrayLayer = 0;
	    SrcSubresourceLayers.layerCount = 1;
	    SrcSubresourceLayers.aspectMask = vk::ImageAspectFlagBits::eColor;
	    
	    vk::ImageSubresourceLayers DstSubresourceLayers;
	    DstSubresourceLayers.mipLevel = 0;
	    DstSubresourceLayers.baseArrayLayer = 0;
	    DstSubresourceLayers.layerCount = 1;
	    DstSubresourceLayers.aspectMask = vk::ImageAspectFlagBits::eColor;
	    
	    vk::Extent3D ImageSize = {
	    	vulkanContext.swapchainExtent.width ,
	    	vulkanContext.swapchainExtent.height,
	    	1
	    };
	    
	    bufferManger.CopyImageToAnotherImage(commandBuffer,
	    	gbuffer.Normal, vk::ImageLayout::eGeneral, SrcSubresourceLayers,
	    	gbuffer.PrevNormal, vk::ImageLayout::eGeneral, SrcSubresourceLayers,
	    	ImageSize, vulkanContext.graphicsQueue);



		ImageTransitionData TransitionToGeneral{};
		TransitionToGeneral.oldlayout = vk::ImageLayout::eUndefined;
		TransitionToGeneral.newlayout = vk::ImageLayout::eGeneral;
		TransitionToGeneral.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionToGeneral.SourceAccessflag = vk::AccessFlagBits::eNone;
		TransitionToGeneral.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eShaderRead;
		TransitionToGeneral.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
		TransitionToGeneral.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eFragmentShader;

		bufferManger.TransitionImage(commandBuffer, &SSGI_FullScreenQuad->SSGIPassLastFrameImage, TransitionToGeneral);
		bufferManger.TransitionImage(commandBuffer, &SSGI_FullScreenQuad->SSGIAccumilationImage, TransitionToGeneral);



		vk::RenderingAttachmentInfo PositioncolorAttachmentInfo{};
		PositioncolorAttachmentInfo.imageView = gbuffer.Position.imageView;
		PositioncolorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		PositioncolorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		PositioncolorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		PositioncolorAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfo ViewSpacePositioncolorAttachmentInfo{};
		ViewSpacePositioncolorAttachmentInfo.imageView = gbuffer.ViewSpacePosition.imageView;
		ViewSpacePositioncolorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		ViewSpacePositioncolorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		ViewSpacePositioncolorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		ViewSpacePositioncolorAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfo NormalcolorAttachmentInfo{};
		NormalcolorAttachmentInfo.imageView = gbuffer.Normal.imageView;
		NormalcolorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		NormalcolorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		NormalcolorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		NormalcolorAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfo ViewSpaceNormalcolorAttachmentInfo{};
		ViewSpaceNormalcolorAttachmentInfo.imageView = gbuffer.ViewSpaceNormal.imageView;
		ViewSpaceNormalcolorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		ViewSpaceNormalcolorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		ViewSpaceNormalcolorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		ViewSpaceNormalcolorAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfo AlbedocolorAttachmentInfo{};
		AlbedocolorAttachmentInfo.imageView = gbuffer.Albedo.imageView;
		AlbedocolorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		AlbedocolorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		AlbedocolorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		AlbedocolorAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfo EmmisivAttachmentInfo{};
		EmmisivAttachmentInfo.imageView = gbuffer.Emissive.imageView;
		EmmisivAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		EmmisivAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		EmmisivAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		EmmisivAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfo MaterialscolorAttachmentInfo{};
		MaterialscolorAttachmentInfo.imageView = gbuffer.Materials.imageView;
		MaterialscolorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		MaterialscolorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		MaterialscolorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		MaterialscolorAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfo MotionVectorcolorAttachmentInfo{};
		MotionVectorcolorAttachmentInfo.imageView = gbuffer.MotionVector.imageView;
		MotionVectorcolorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		MotionVectorcolorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		MotionVectorcolorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		MotionVectorcolorAttachmentInfo.clearValue = clearColor;

		std::array<vk::RenderingAttachmentInfo, 8> ColorAttachments{ PositioncolorAttachmentInfo,ViewSpacePositioncolorAttachmentInfo,
			                                                         NormalcolorAttachmentInfo, ViewSpaceNormalcolorAttachmentInfo,
			                                                         AlbedocolorAttachmentInfo,EmmisivAttachmentInfo,
			                                                         MaterialscolorAttachmentInfo,MotionVectorcolorAttachmentInfo };

		vk::RenderingAttachmentInfo depthStencilAttachment;
		depthStencilAttachment.imageView = DepthTextureData.imageView;
		depthStencilAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		depthStencilAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		depthStencilAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		depthStencilAttachment.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

		vk::RenderingInfo renderingInfo{};
		renderingInfo.renderArea.offset = imageoffset;
		renderingInfo.renderArea.extent.height = vulkanContext.swapchainExtent.height;
		renderingInfo.renderArea.extent.width = vulkanContext.swapchainExtent.width;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = ColorAttachments.size();
		renderingInfo.pColorAttachments = ColorAttachments.data();
		renderingInfo.pDepthAttachment = &depthStencilAttachment;


		commandBuffer.beginRendering(renderingInfo);
		commandBuffer.setViewport(0, 1, &viewport);
		commandBuffer.setScissor(0, 1, &scissor);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, geometryPassPipeline);

		if (bWireFrame)
		{
			vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_LINE);
		}
		else
		{
			vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_FILL);
		}

		for (auto& model : Models)
		{
			model->Draw(commandBuffer, geometryPassPipelineLayout, currentFrame);
		}

		commandBuffer.endRendering();

	}

	vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);

	/////////////////// GBUFFER PASS END ///////////////////////// 

 	vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_FILL);


	vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, SSAO_Label);
	{
		ImageTransitionData GBufferDepthToSample{};
		GBufferDepthToSample.oldlayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		GBufferDepthToSample.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		GBufferDepthToSample.AspectFlag = vk::ImageAspectFlagBits::eDepth;
		GBufferDepthToSample.SourceAccessflag = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		GBufferDepthToSample.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
		GBufferDepthToSample.SourceOnThePipeline = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
		GBufferDepthToSample.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

		bufferManger.TransitionImage(commandBuffer, &DepthTextureData, GBufferDepthToSample);

		vk::RenderingAttachmentInfo SSAOColorAttachment{};
		SSAOColorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		SSAOColorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		SSAOColorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		SSAOColorAttachment.imageView = ssao_FullScreenQuad->SSAOImage.imageView;
		SSAOColorAttachment.clearValue = clearColor;

		vk::RenderingInfo renderingInfo{};
		renderingInfo.renderArea.offset = imageoffset;
		renderingInfo.renderArea.extent.height = ssao_FullScreenQuad->SSAOImageSize.height;
		renderingInfo.renderArea.extent.width  = ssao_FullScreenQuad->SSAOImageSize.width;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &SSAOColorAttachment;

		vk::Viewport SSAOviewport{};
		SSAOviewport.x = 0.0f;
		SSAOviewport.y = 0.0f;
		SSAOviewport.width  = ssao_FullScreenQuad->SSAOImageSize.width;
		SSAOviewport.height = ssao_FullScreenQuad->SSAOImageSize.height;
		SSAOviewport.minDepth = 0.0f;
		SSAOviewport.maxDepth = 1.0f;

		vk::Rect2D SSAOscissor{};
		SSAOscissor.offset = imageoffset;
		SSAOscissor.extent.width  = ssao_FullScreenQuad->SSAOImageSize.width;
		SSAOscissor.extent.height = ssao_FullScreenQuad->SSAOImageSize.height;


		commandBuffer.beginRendering(renderingInfo);
		commandBuffer.setViewport(0, 1, &SSAOviewport);
		commandBuffer.setScissor(0, 1, &SSAOscissor);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, SSAOPipeline);

		ssao_FullScreenQuad->Draw(commandBuffer, SSAOPipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}

	{
		vk::RenderingAttachmentInfo SSAOBluredColorAttachment{};
		SSAOBluredColorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		SSAOBluredColorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		SSAOBluredColorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		SSAOBluredColorAttachment.imageView = ssao_FullScreenQuad->BluredSSAOImage.imageView;
		SSAOBluredColorAttachment.clearValue = clearColor;

		vk::RenderingInfo renderingInfo{};
		renderingInfo.renderArea.offset = imageoffset;
		renderingInfo.renderArea.extent.height = ssao_FullScreenQuad->BluredSSAOImageSize.height;
		renderingInfo.renderArea.extent.width = ssao_FullScreenQuad->BluredSSAOImageSize.width;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &SSAOBluredColorAttachment;

		vk::Viewport SSAOviewport{};
		SSAOviewport.x = 0.0f;
		SSAOviewport.y = 0.0f;
		SSAOviewport.width = ssao_FullScreenQuad->BluredSSAOImageSize.width;
		SSAOviewport.height = ssao_FullScreenQuad->BluredSSAOImageSize.height;
		SSAOviewport.minDepth = 0.0f;
		SSAOviewport.maxDepth = 1.0f;

		vk::Rect2D SSAOscissor{};
		SSAOscissor.offset = imageoffset;
		SSAOscissor.extent.width = ssao_FullScreenQuad->BluredSSAOImageSize.width;
		SSAOscissor.extent.height = ssao_FullScreenQuad->BluredSSAOImageSize.height;


		commandBuffer.beginRendering(renderingInfo);
		commandBuffer.setViewport(0, 1, &SSAOviewport);
		commandBuffer.setScissor(0, 1, &SSAOscissor);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, SSAOBlurPipeline);

		ssao_FullScreenQuad->DrawSSAOBlurHorizontal(commandBuffer, SSAOBlurPipelineLayout, currentFrame);
		commandBuffer.endRendering();

		{
			vk::RenderingAttachmentInfo SSAOBluredColorAttachment{};
			SSAOBluredColorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
			SSAOBluredColorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
			SSAOBluredColorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
			SSAOBluredColorAttachment.imageView = ssao_FullScreenQuad->BluredSSAOImage.imageView;
			SSAOBluredColorAttachment.clearValue = clearColor;

			vk::RenderingInfo renderingInfo{};
			renderingInfo.renderArea.offset = imageoffset;
			renderingInfo.renderArea.extent.height = ssao_FullScreenQuad->BluredSSAOImageSize.height;
			renderingInfo.renderArea.extent.width = ssao_FullScreenQuad->BluredSSAOImageSize.width;
			renderingInfo.layerCount = 1;
			renderingInfo.colorAttachmentCount = 1;
			renderingInfo.pColorAttachments = &SSAOBluredColorAttachment;

			commandBuffer.beginRendering(renderingInfo);
			commandBuffer.setViewport(0, 1, &SSAOviewport);
			commandBuffer.setScissor(0, 1, &SSAOscissor);
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, SSAOBlurPipeline);

			ssao_FullScreenQuad->DrawSSAOBlurVertical(commandBuffer, SSAOBlurPipelineLayout, currentFrame);

			commandBuffer.endRendering();
		}
	}

	vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);


	{
		vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, DDGI_Grid_Generation_Label);

		ImageTransitionData TransitiontoGeneralRT{};
		TransitiontoGeneralRT.oldlayout = vk::ImageLayout::eUndefined;
		TransitiontoGeneralRT.newlayout = vk::ImageLayout::eGeneral;
		TransitiontoGeneralRT.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitiontoGeneralRT.SourceAccessflag = vk::AccessFlagBits::eNone;
		TransitiontoGeneralRT.DestinationAccessflag = vk::AccessFlagBits::eShaderWrite;
		TransitiontoGeneralRT.SourceOnThePipeline = vk::PipelineStageFlagBits::eNone;
		TransitiontoGeneralRT.DestinationOnThePipeline = vk::PipelineStageFlagBits::eRayTracingShaderKHR;

		bufferManger.TransitionImage(commandBuffer, &dynamicDiffuse_RTGI->RadianceImageAtlasImage, TransitiontoGeneralRT);
		bufferManger.TransitionImage(commandBuffer, &dynamicDiffuse_RTGI->IradianceImageAtlasImage, TransitiontoGeneralRT);
		bufferManger.TransitionImage(commandBuffer, &dynamicDiffuse_RTGI->VisibilityImageAtlasImage, TransitiontoGeneralRT);
		bufferManger.TransitionImage(commandBuffer, &dynamicDiffuse_RTGI->Prev_IradianceImageAtlasImage, TransitiontoGeneralRT);
		bufferManger.TransitionImage(commandBuffer, &dynamicDiffuse_RTGI->Prev_VisibilityImageAtlasImage, TransitiontoGeneralRT);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, GridComputePassPipeline);
		
		dynamicDiffuse_RTGI->DispatchGridCompute(commandBuffer, GridComputePipelineLayout, currentFrame);

		vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);

		vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, DDGI_Directions_Generation_Label);
		dynamicDiffuse_RTGI->DispatchDirectionsCompute(commandBuffer, GridComputePipelineLayout, currentFrame,deltaTime);
		vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);


		vk::BufferMemoryBarrier barrier{};
		barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		barrier.buffer = dynamicDiffuse_RTGI->ProbeDataStorageBuffers[0].buffer;
		barrier.offset = 0;
		barrier.size = VK_WHOLE_SIZE;

		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eRayTracingShaderKHR,
			{},
			0, nullptr,
			1, &barrier,
			0, nullptr
		);

		vk::BufferMemoryBarrier barrier2{};
		barrier2.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
		barrier2.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		barrier2.buffer = dynamicDiffuse_RTGI->ProbeFibonacciDirectionsStorageBuffers[0].buffer;
		barrier2.offset = 0;
		barrier2.size = VK_WHOLE_SIZE;

		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eRayTracingShaderKHR,
			{},
			0, nullptr,
			1, &barrier2,
			0, nullptr
		);


	}

	{
		vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, DDGI_Trace_Ray_Label);
		{
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, RT_DDGIPassPipeline);

			dynamicDiffuse_RTGI->Draw(
				DDGI_raygenShaderBindingTableBuffer,
				DDGI_hitShaderBindingTableBuffer,
				DDGI_missShaderBindingTableBuffer,
				commandBuffer,
				RT_DDGIPipelineLayout,
				currentFrame);


			vk::ImageMemoryBarrier rtToComputeBarrier{};
			rtToComputeBarrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
			rtToComputeBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			rtToComputeBarrier.oldLayout = vk::ImageLayout::eGeneral; 
			rtToComputeBarrier.newLayout = vk::ImageLayout::eGeneral; 
			rtToComputeBarrier.image = dynamicDiffuse_RTGI->RadianceImageAtlasImage.image;
			rtToComputeBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			rtToComputeBarrier.subresourceRange.baseMipLevel = 0;
			rtToComputeBarrier.subresourceRange.levelCount = 1;
			rtToComputeBarrier.subresourceRange.baseArrayLayer = 0;
			rtToComputeBarrier.subresourceRange.layerCount = 1;
			rtToComputeBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			rtToComputeBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			commandBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eRayTracingShaderKHR, 
				vk::PipelineStageFlagBits::eComputeShader,    
				{},
				0, nullptr,
				0, nullptr,
				1, & rtToComputeBarrier
			);
		}
		vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);



		{


			vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, DDGI_Calculate_Irradiance_Label);

			{
				commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, IrradianceComputePassPipeline);

				dynamicDiffuse_RTGI->DispatchCalcProbeDataCompute(commandBuffer, IrradianceComputePipelineLayout, currentFrame);

				vk::ImageMemoryBarrier imagebarrier;
				imagebarrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
				imagebarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
				imagebarrier.oldLayout = vk::ImageLayout::eGeneral;
				imagebarrier.newLayout = vk::ImageLayout::eGeneral;
				imagebarrier.image = dynamicDiffuse_RTGI->VisibilityImageAtlasImage.image;
				imagebarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
				imagebarrier.subresourceRange.baseMipLevel = 0;
				imagebarrier.subresourceRange.levelCount = 1;
				imagebarrier.subresourceRange.baseArrayLayer = 0;
				imagebarrier.subresourceRange.layerCount = 1;

				imagebarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				imagebarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				commandBuffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eComputeShader,
					vk::PipelineStageFlagBits::eRayTracingShaderKHR,
					{},
					0, nullptr,
					0, nullptr,
					1, &imagebarrier
				);
			}
			vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);

			vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, DDGI_Update_Probe_Status_Label);

			{
				commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, ProbeStatusComputePassPipeline);

				dynamicDiffuse_RTGI->DispatchProbeStatus(commandBuffer, ProbeStatusPipelineLayout, currentFrame);

				vk::BufferMemoryBarrier barrier{};
				barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
				barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
				barrier.buffer = dynamicDiffuse_RTGI->ProbeDataStorageBuffers[0].buffer;
				barrier.offset = 0;
				barrier.size = VK_WHOLE_SIZE;

				commandBuffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eComputeShader,
					vk::PipelineStageFlagBits::eRayTracingShaderKHR,
					{},
					0, nullptr,
					1, & barrier,
					0, nullptr
				);
			}
			vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);


			{

				ImageTransitionData TransitiontoGeneraCompute{};
				TransitiontoGeneraCompute.oldlayout = vk::ImageLayout::eUndefined;
				TransitiontoGeneraCompute.newlayout = vk::ImageLayout::eGeneral;
				TransitiontoGeneraCompute.AspectFlag = vk::ImageAspectFlagBits::eColor;
				TransitiontoGeneraCompute.SourceAccessflag = vk::AccessFlagBits::eNone;
				TransitiontoGeneraCompute.DestinationAccessflag = vk::AccessFlagBits::eShaderWrite;
				TransitiontoGeneraCompute.SourceOnThePipeline = vk::PipelineStageFlagBits::eNone;
				TransitiontoGeneraCompute.DestinationOnThePipeline = vk::PipelineStageFlagBits::eComputeShader;

				bufferManger.TransitionImage(commandBuffer, &dynamicDiffuse_RTGI->IradianceImageAtlasImage, TransitiontoGeneraCompute);
				bufferManger.TransitionImage(commandBuffer, &dynamicDiffuse_RTGI->VisibilityImageAtlasImage, TransitiontoGeneraCompute);
				bufferManger.TransitionImage(commandBuffer, &dynamicDiffuse_RTGI->Prev_IradianceImageAtlasImage, TransitiontoGeneraCompute);
				bufferManger.TransitionImage(commandBuffer, &dynamicDiffuse_RTGI->Prev_VisibilityImageAtlasImage, TransitiontoGeneraCompute);

				vk::ImageSubresourceLayers SrcSubresourceLayers;
				SrcSubresourceLayers.mipLevel = 0;
				SrcSubresourceLayers.baseArrayLayer = 0;
				SrcSubresourceLayers.layerCount = 1;
				SrcSubresourceLayers.aspectMask = vk::ImageAspectFlagBits::eColor;

				vk::ImageSubresourceLayers DstSubresourceLayers;
				DstSubresourceLayers.mipLevel = 0;
				DstSubresourceLayers.baseArrayLayer = 0;
				DstSubresourceLayers.layerCount = 1;
				DstSubresourceLayers.aspectMask = vk::ImageAspectFlagBits::eColor;

				vk::Extent3D ImageSize = {
					dynamicDiffuse_RTGI->IradianceImageExtent.width ,
					dynamicDiffuse_RTGI->IradianceImageExtent.height,
					1
				};

				bufferManger.CopyImageToAnotherImage(commandBuffer,
					dynamicDiffuse_RTGI->IradianceImageAtlasImage, vk::ImageLayout::eGeneral, SrcSubresourceLayers,
					dynamicDiffuse_RTGI->Prev_IradianceImageAtlasImage, vk::ImageLayout::eGeneral, SrcSubresourceLayers,
					ImageSize, vulkanContext.graphicsQueue);

				bufferManger.CopyImageToAnotherImage(commandBuffer,
					dynamicDiffuse_RTGI->VisibilityImageAtlasImage, vk::ImageLayout::eGeneral, SrcSubresourceLayers,
					dynamicDiffuse_RTGI->Prev_VisibilityImageAtlasImage, vk::ImageLayout::eGeneral, SrcSubresourceLayers,
					ImageSize, vulkanContext.graphicsQueue);
			}


		}


		vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, DDGI_Sample_From_PorbeLabel);

		{
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, SampleDDGIComputePassPipeline);

			dynamicDiffuse_RTGI->DispatchSampleGIFromProbeDataCompute(commandBuffer, SampleDDGIComputePipelineLayout, currentFrame);

			vk::ImageMemoryBarrier imagebarrier;
			imagebarrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
			imagebarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			imagebarrier.oldLayout = vk::ImageLayout::eUndefined;
			imagebarrier.newLayout = vk::ImageLayout::eGeneral;
			imagebarrier.image = dynamicDiffuse_RTGI->Probe_Sampled_GI_Image.image;
			imagebarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			imagebarrier.subresourceRange.baseMipLevel = 0;
			imagebarrier.subresourceRange.levelCount = 1;
			imagebarrier.subresourceRange.baseArrayLayer = 0;
			imagebarrier.subresourceRange.layerCount = 1;

			imagebarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imagebarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			commandBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eComputeShader,
				vk::PipelineStageFlagBits::eFragmentShader,
				{},
				0, nullptr,
				0, nullptr,
				1, &imagebarrier
			);
		}
		vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);


	}

	vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, ReSTIR_Label);
	{
		if (DefferedDecider == 2) { /// if we are not looking at ReSTIR stop tracing

			ImageTransitionData TransitiontoGeneraRT{};
			TransitiontoGeneraRT.oldlayout = vk::ImageLayout::eUndefined;
			TransitiontoGeneraRT.newlayout = vk::ImageLayout::eGeneral;
			TransitiontoGeneraRT.AspectFlag = vk::ImageAspectFlagBits::eColor;
			TransitiontoGeneraRT.SourceAccessflag = vk::AccessFlagBits::eNone;
			TransitiontoGeneraRT.DestinationAccessflag = vk::AccessFlagBits::eShaderWrite;
			TransitiontoGeneraRT.SourceOnThePipeline = vk::PipelineStageFlagBits::eNone;
			TransitiontoGeneraRT.DestinationOnThePipeline = vk::PipelineStageFlagBits::eRayTracingShaderKHR;

			bufferManger.TransitionImage(commandBuffer, &Restir_DI->ResevoirImage, TransitiontoGeneraRT);
			bufferManger.TransitionImage(commandBuffer, &Restir_DI->PrevResevoirImage, TransitiontoGeneraRT);

			vk::ImageSubresourceLayers SrcSubresourceLayers;
			SrcSubresourceLayers.mipLevel = 0;
			SrcSubresourceLayers.baseArrayLayer = 0;
			SrcSubresourceLayers.layerCount = 1;
			SrcSubresourceLayers.aspectMask = vk::ImageAspectFlagBits::eColor;

			vk::ImageSubresourceLayers DstSubresourceLayers;
			DstSubresourceLayers.mipLevel = 0;
			DstSubresourceLayers.baseArrayLayer = 0;
			DstSubresourceLayers.layerCount = 1;
			DstSubresourceLayers.aspectMask = vk::ImageAspectFlagBits::eColor;

			vk::Extent3D ImageSize = {
				vulkanContext.swapchainExtent.width ,
				vulkanContext.swapchainExtent.height,
				1
			};

			bufferManger.CopyImageToAnotherImage(commandBuffer,
				Restir_DI->ResevoirImage, vk::ImageLayout::eGeneral, SrcSubresourceLayers,
				Restir_DI->PrevResevoirImage, vk::ImageLayout::eGeneral, SrcSubresourceLayers,
				ImageSize, vulkanContext.graphicsQueue);

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, ReSTIR_RTPassPipeline);

			Restir_DI->Draw(
				ReSTIR_DI_raygenShaderBindingTableBuffer,
				ReSTIR_DI_hitShaderBindingTableBuffer,
				ReSTIR_DI_missShaderBindingTableBuffer,
				commandBuffer,
				ReSTIR_RT_PipelineLayout,
				currentFrame);
		}
	}
	vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);

	vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, RTReflections_Label);

	{
		ImageTransitionData TransitiontoGeneralRT{};
		TransitiontoGeneralRT.oldlayout = vk::ImageLayout::eUndefined;
		TransitiontoGeneralRT.newlayout = vk::ImageLayout::eGeneral;
		TransitiontoGeneralRT.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitiontoGeneralRT.SourceAccessflag = vk::AccessFlagBits::eNone;
		TransitiontoGeneralRT.DestinationAccessflag = vk::AccessFlagBits::eShaderWrite;
		TransitiontoGeneralRT.SourceOnThePipeline = vk::PipelineStageFlagBits::eNone;
		TransitiontoGeneralRT.DestinationOnThePipeline = vk::PipelineStageFlagBits::eRayTracingShaderKHR;
		TransitiontoGeneralRT.LevelCount = RT_Reflection->ReflectionPassImage.miplevels;

		bufferManger.TransitionImage(commandBuffer, &RT_Reflection->ReflectionPassImage, TransitiontoGeneralRT);



		commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, RT_ReflectionPassPipeline);

		RT_Reflection->Draw(
			Reflection_raygenShaderBindingTableBuffer,
			Reflection_hitShaderBindingTableBuffer,
			Reflection_missShaderBindingTableBuffer,
			commandBuffer,
			RT_ReflectionPipelineLayout,
			currentFrame);

		ImageTransitionData TransitiontoDST{};
		TransitiontoDST.oldlayout = vk::ImageLayout::eUndefined;
		TransitiontoDST.newlayout = vk::ImageLayout::eTransferDstOptimal;
		TransitiontoDST.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitiontoDST.SourceAccessflag = vk::AccessFlagBits::eShaderWrite;
		TransitiontoDST.DestinationAccessflag = vk::AccessFlagBits::eTransferWrite;
		TransitiontoDST.SourceOnThePipeline = vk::PipelineStageFlagBits::eRayTracingShaderKHR;
		TransitiontoDST.DestinationOnThePipeline = vk::PipelineStageFlagBits::eTransfer;
		TransitiontoDST.LevelCount = RT_Reflection->ReflectionPassImage.miplevels;

		bufferManger.TransitionImage(commandBuffer, &RT_Reflection->ReflectionPassImage, TransitiontoDST);

		bufferManger.GenerateMipMaps(&RT_Reflection->ReflectionPassImage, &commandBuffer, RT_Reflection->swapchainextent.width,
			                          RT_Reflection->swapchainextent.height, vulkanContext.graphicsQueue,1);

		ImageTransitionData TransitiontoGeneral{};
		TransitiontoGeneral.oldlayout = vk::ImageLayout::eUndefined;
		TransitiontoGeneral.newlayout = vk::ImageLayout::eGeneral;
		TransitiontoGeneral.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitiontoGeneral.SourceAccessflag = vk::AccessFlagBits::eNone;
		TransitiontoGeneral.DestinationAccessflag = vk::AccessFlagBits::eShaderWrite;
		TransitiontoGeneral.SourceOnThePipeline = vk::PipelineStageFlagBits::eNone;
		TransitiontoGeneral.DestinationOnThePipeline = vk::PipelineStageFlagBits::eRayTracingShaderKHR;
		TransitiontoGeneral.LevelCount = RT_Reflection->ReflectionPassImage.miplevels;

		bufferManger.TransitionImage(commandBuffer, &RT_Reflection->ReflectionPassImage, TransitiontoGeneral);


		{
			vk::RenderingAttachmentInfo BlurPassColorAttachmentInfo{};
			BlurPassColorAttachmentInfo.imageView = RT_Reflection->HorizontalBlurReflectionPassImage.imageView;
			BlurPassColorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
			BlurPassColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
			BlurPassColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
			BlurPassColorAttachmentInfo.clearValue = clearColor;

			vk::RenderingInfo renderingInfo{};
			renderingInfo.renderArea.offset = imageoffset;
			renderingInfo.renderArea.extent.height = vulkanContext.swapchainExtent.height;
			renderingInfo.renderArea.extent.width = vulkanContext.swapchainExtent.width;
			renderingInfo.layerCount = 1;
			renderingInfo.colorAttachmentCount = 1;
			renderingInfo.pColorAttachments = &BlurPassColorAttachmentInfo;

			commandBuffer.setViewport(0, 1, &viewport);
			commandBuffer.setScissor(0, 1, &scissor);
			commandBuffer.beginRendering(renderingInfo);

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, BluredRTreflectionPipeline);
			RT_Reflection->DrawHorizontalBlurPass(commandBuffer, BluredRTreflectionsPipelineLayout, currentFrame);

			commandBuffer.endRendering();
		}


		{
			vk::RenderingAttachmentInfo BlurPassColorAttachmentInfo{};
			BlurPassColorAttachmentInfo.imageView = RT_Reflection->FullBlurReflectionPassImage.imageView;
			BlurPassColorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
			BlurPassColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
			BlurPassColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
			BlurPassColorAttachmentInfo.clearValue = clearColor;

			vk::RenderingInfo renderingInfo{};
			renderingInfo.renderArea.offset = imageoffset;
			renderingInfo.renderArea.extent.height = vulkanContext.swapchainExtent.height;
			renderingInfo.renderArea.extent.width = vulkanContext.swapchainExtent.width;
			renderingInfo.layerCount = 1;
			renderingInfo.colorAttachmentCount = 1;
			renderingInfo.pColorAttachments = &BlurPassColorAttachmentInfo;

			commandBuffer.setViewport(0, 1, &viewport);
			commandBuffer.setScissor(0, 1, &scissor);
			commandBuffer.beginRendering(renderingInfo);

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, BluredRTreflectionPipeline);
			RT_Reflection->DrawVerticalBlurPass(commandBuffer, BluredRTreflectionsPipelineLayout, currentFrame);

			commandBuffer.endRendering();
		}

	}
	vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
	
	vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, DirectLighting_Label);
    /////////////////// LIGHTING PASS ///////////////////////// 
	{
		if (DefferedDecider == 3 || DefferedDecider == 0) {

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, DeferedLightingPassPipeline);

			lighting_RTX->Draw(
				Lighting_raygenShaderBindingTableBuffer,
				Lighting_hitShaderBindingTableBuffer,
				Lighting_missShaderBindingTableBuffer,
				commandBuffer,
				DeferedLightingPassPipelineLayout,
				currentFrame);
		}
	}
	 /////////////////// LIGHTING PASS END ///////////////////////// 
	vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);


	if (DefferedDecider != 2) {
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, SSGIPipeline);
		SSGI_FullScreenQuad->ComputeSSGI(commandBuffer, SSGIPipelineLayout, currentFrame);

	vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, SSGI_Label);
	{

		vk::ImageMemoryBarrier barrier{};
		barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		barrier.oldLayout = vk::ImageLayout::eGeneral;
		barrier.newLayout = vk::ImageLayout::eGeneral;
		barrier.image = SSGI_FullScreenQuad->SSGIPassImage.image;
		barrier.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eFragmentShader,
			{}, 0, nullptr, 0, nullptr, 1, & barrier
		);

		ImageTransitionData TransitionDeptTODepthOptimal{};
		TransitionDeptTODepthOptimal.oldlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionDeptTODepthOptimal.newlayout = vk::ImageLayout::eDepthAttachmentOptimal;
		TransitionDeptTODepthOptimal.AspectFlag = vk::ImageAspectFlagBits::eDepth;
		TransitionDeptTODepthOptimal.SourceAccessflag = vk::AccessFlagBits::eShaderRead;
		TransitionDeptTODepthOptimal.DestinationAccessflag = vk::AccessFlagBits::eDepthStencilAttachmentWrite |vk::AccessFlagBits::eDepthStencilAttachmentRead;
		TransitionDeptTODepthOptimal.SourceOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;
		TransitionDeptTODepthOptimal.DestinationOnThePipeline = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
		bufferManger.TransitionImage(commandBuffer, &DepthTextureData, TransitionDeptTODepthOptimal);

	}

	{
		ImageTransitionData TransitionTOSrc{};
		TransitionTOSrc.oldlayout = vk::ImageLayout::eGeneral;
		TransitionTOSrc.newlayout = vk::ImageLayout::eTransferSrcOptimal;
		TransitionTOSrc.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionTOSrc.SourceAccessflag = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eShaderRead;
		TransitionTOSrc.DestinationAccessflag = vk::AccessFlagBits::eTransferRead;
		TransitionTOSrc.SourceOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eFragmentShader;
		TransitionTOSrc.DestinationOnThePipeline = vk::PipelineStageFlagBits::eTransfer;

		bufferManger.TransitionImage(commandBuffer, &SSGI_FullScreenQuad->SSGIAccumilationImage, TransitionTOSrc);

		ImageTransitionData TransitionTODst{};
		TransitionTODst.oldlayout = vk::ImageLayout::eGeneral;
		TransitionTODst.newlayout = vk::ImageLayout::eTransferDstOptimal;
		TransitionTODst.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionTODst.SourceAccessflag = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eShaderRead;
		TransitionTODst.DestinationAccessflag = vk::AccessFlagBits::eTransferWrite;
		TransitionTODst.SourceOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eFragmentShader;
		TransitionTODst.DestinationOnThePipeline = vk::PipelineStageFlagBits::eTransfer;


		bufferManger.TransitionImage(commandBuffer, &SSGI_FullScreenQuad->SSGIPassLastFrameImage, TransitionTODst);


		vk::ImageSubresourceLayers SrcSubresourceLayers;
		SrcSubresourceLayers.mipLevel = 0;
		SrcSubresourceLayers.baseArrayLayer = 0;
	    SrcSubresourceLayers.layerCount = 1;
		SrcSubresourceLayers.aspectMask = vk::ImageAspectFlagBits::eColor;

		vk::ImageSubresourceLayers DstSubresourceLayers;
		DstSubresourceLayers.mipLevel = 0;
		DstSubresourceLayers.baseArrayLayer = 0;
		DstSubresourceLayers.layerCount = 1;
		DstSubresourceLayers.aspectMask = vk::ImageAspectFlagBits::eColor;

		vk::Extent3D swapchainExtenthalf = {
			SSGI_FullScreenQuad->SSGI_ImageFullResolution.width ,
			SSGI_FullScreenQuad->SSGI_ImageFullResolution.height,
			1
		};

		bufferManger.CopyImageToAnotherImage(commandBuffer,
			                                  SSGI_FullScreenQuad->SSGIAccumilationImage,  vk::ImageLayout::eTransferSrcOptimal, SrcSubresourceLayers,
			                                  SSGI_FullScreenQuad->SSGIPassLastFrameImage, vk::ImageLayout::eTransferDstOptimal, DstSubresourceLayers,
			                                  swapchainExtenthalf, vulkanContext.graphicsQueue);






		ImageTransitionData TransitionSrcBack{};
		TransitionSrcBack.oldlayout = vk::ImageLayout::eTransferSrcOptimal;
		TransitionSrcBack.newlayout = vk::ImageLayout::eGeneral;
		TransitionSrcBack.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionSrcBack.SourceAccessflag = vk::AccessFlagBits::eTransferRead; 
		TransitionSrcBack.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eShaderRead;
		TransitionSrcBack.SourceOnThePipeline = vk::PipelineStageFlagBits::eTransfer;
		TransitionSrcBack.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eFragmentShader;

		bufferManger.TransitionImage(commandBuffer, &SSGI_FullScreenQuad->SSGIAccumilationImage, TransitionSrcBack);


		ImageTransitionData TransitionDstToSample{};
		TransitionDstToSample.oldlayout = vk::ImageLayout::eTransferDstOptimal;
		TransitionDstToSample.newlayout = vk::ImageLayout::eGeneral;
		TransitionDstToSample.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionDstToSample.SourceAccessflag = vk::AccessFlagBits::eTransferWrite;
		TransitionDstToSample.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eShaderRead;
		TransitionDstToSample.SourceOnThePipeline = vk::PipelineStageFlagBits::eTransfer;
		TransitionDstToSample.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eFragmentShader;

		bufferManger.TransitionImage(commandBuffer, &SSGI_FullScreenQuad->SSGIPassLastFrameImage, TransitionDstToSample);

	}

	{
		vk::RenderingAttachmentInfo TA_ImageAttachInfo;
		TA_ImageAttachInfo.clearValue = clearColor;
		TA_ImageAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		TA_ImageAttachInfo.imageView = SSGI_FullScreenQuad->SSGIAccumilationImage.imageView;
		TA_ImageAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
		TA_ImageAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingInfo SSGIImageInfo{};
		SSGIImageInfo.layerCount = 1;
		SSGIImageInfo.colorAttachmentCount = 1;
		SSGIImageInfo.pColorAttachments = &TA_ImageAttachInfo;
		SSGIImageInfo.renderArea.extent.width  = SSGI_FullScreenQuad->SSGI_ImageFullResolution.width ;
		SSGIImageInfo.renderArea.extent.height = SSGI_FullScreenQuad->SSGI_ImageFullResolution.height ;

		vk::Viewport viewport50{};
		viewport50.x = 0.0f;
		viewport50.y = 0.0f;
		viewport50.width  = SSGI_FullScreenQuad->SSGI_ImageFullResolution.width ;
		viewport50.height = SSGI_FullScreenQuad->SSGI_ImageFullResolution.height ;
		viewport50.minDepth = 0.0f;
		viewport50.maxDepth = 1.0f;

		vk::Rect2D scissor50{};
		scissor50.offset = imageoffset;
		scissor50.extent.width  = SSGI_FullScreenQuad->SSGI_ImageFullResolution.width ;
		scissor50.extent.height = SSGI_FullScreenQuad->SSGI_ImageFullResolution.height ;

		commandBuffer.setViewport(0, 1, &viewport50);
		commandBuffer.setScissor(0, 1, &scissor50);
		commandBuffer.beginRendering(SSGIImageInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, TA_SSGIPipeline);
		SSGI_FullScreenQuad->DrawTA(commandBuffer, TA_SSGIPipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}

	{
		vk::RenderingAttachmentInfo Blured_TA_ImageAttachInfo;
		Blured_TA_ImageAttachInfo.clearValue = clearColor;
		Blured_TA_ImageAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		Blured_TA_ImageAttachInfo.imageView = SSGI_FullScreenQuad->BlurPing_DownSampleHalfRes.imageView;
		Blured_TA_ImageAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
		Blured_TA_ImageAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingInfo BluredSSGIImageInfo{};
		BluredSSGIImageInfo.layerCount = 1;
		BluredSSGIImageInfo.colorAttachmentCount = 1;
		BluredSSGIImageInfo.pColorAttachments = &Blured_TA_ImageAttachInfo;
		BluredSSGIImageInfo.renderArea.extent.width = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.width;
		BluredSSGIImageInfo.renderArea.extent.height = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.height;

		vk::Viewport viewport50{};
		viewport50.x = 0.0f;
		viewport50.y = 0.0f;
		viewport50.width = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.width;
		viewport50.height = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.height;
		viewport50.minDepth = 0.0f;
		viewport50.maxDepth = 1.0f;

		vk::Rect2D scissor50{};
		scissor50.offset = imageoffset;
		scissor50.extent.width = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.width;
		scissor50.extent.height = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.height;

		commandBuffer.setViewport(0, 1, &viewport50);
		commandBuffer.setScissor(0, 1, &scissor50);
		commandBuffer.beginRendering(BluredSSGIImageInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, BluredSSGIPipeline);


		SSGI_FullScreenQuad->DrawDownSampleHalfResFirstPass(commandBuffer, BluredSSGIPipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}

	{
		vk::RenderingAttachmentInfo Blured_TA_ImageAttachInfo;
		Blured_TA_ImageAttachInfo.clearValue = clearColor;
		Blured_TA_ImageAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		Blured_TA_ImageAttachInfo.imageView = SSGI_FullScreenQuad->BlurPong_DownSampleHalfRes.imageView;
		Blured_TA_ImageAttachInfo.loadOp = vk::AttachmentLoadOp::eDontCare;
		Blured_TA_ImageAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingInfo BluredSSGIImageInfo{};
		BluredSSGIImageInfo.layerCount = 1;
		BluredSSGIImageInfo.colorAttachmentCount = 1;
		BluredSSGIImageInfo.pColorAttachments = &Blured_TA_ImageAttachInfo;
		BluredSSGIImageInfo.renderArea.extent.width = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.width;
		BluredSSGIImageInfo.renderArea.extent.height = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.height;

		vk::Viewport viewport50{};
		viewport50.x = 0.0f;
		viewport50.y = 0.0f;
		viewport50.width = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.width;
		viewport50.height = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.height;
		viewport50.minDepth = 0.0f;
		viewport50.maxDepth = 1.0f;

		vk::Rect2D scissor50{};
		scissor50.offset = imageoffset;
		scissor50.extent.width = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.width;
		scissor50.extent.height = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.height;

		commandBuffer.setViewport(0, 1, &viewport50);
		commandBuffer.setScissor(0, 1, &scissor50);
		commandBuffer.beginRendering(BluredSSGIImageInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, BluredSSGIPipeline);

		SSGI_FullScreenQuad->DrawDownSampleHalfResSecondPass(commandBuffer, BluredSSGIPipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}


	{
		vk::RenderingAttachmentInfo Blured_TA_ImageAttachInfo;
		Blured_TA_ImageAttachInfo.clearValue = clearColor;
		Blured_TA_ImageAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		Blured_TA_ImageAttachInfo.imageView = SSGI_FullScreenQuad->BlurPing_DownSampleQuaterRes.imageView;
		Blured_TA_ImageAttachInfo.loadOp = vk::AttachmentLoadOp::eDontCare;
		Blured_TA_ImageAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingInfo BluredSSGIImageInfo{};
		BluredSSGIImageInfo.layerCount = 1;
		BluredSSGIImageInfo.colorAttachmentCount = 1;
		BluredSSGIImageInfo.pColorAttachments = &Blured_TA_ImageAttachInfo;
		BluredSSGIImageInfo.renderArea.extent.width = SSGI_FullScreenQuad->SSGI_ImageQuaterResolution.width;
		BluredSSGIImageInfo.renderArea.extent.height = SSGI_FullScreenQuad->SSGI_ImageQuaterResolution.height;

		vk::Viewport viewport50{};
		viewport50.x = 0.0f;
		viewport50.y = 0.0f;
		viewport50.width = SSGI_FullScreenQuad->SSGI_ImageQuaterResolution.width;
		viewport50.height = SSGI_FullScreenQuad->SSGI_ImageQuaterResolution.height;
		viewport50.minDepth = 0.0f;
		viewport50.maxDepth = 1.0f;

		vk::Rect2D scissor50{};
		scissor50.offset = imageoffset;
		scissor50.extent.width = SSGI_FullScreenQuad->SSGI_ImageQuaterResolution.width;
		scissor50.extent.height = SSGI_FullScreenQuad->SSGI_ImageQuaterResolution.height;

		commandBuffer.setViewport(0, 1, &viewport50);
		commandBuffer.setScissor(0, 1, &scissor50);
		commandBuffer.beginRendering(BluredSSGIImageInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, BluredSSGIPipeline);


		SSGI_FullScreenQuad->DrawDownSampleQuaterfResFirstPass(commandBuffer, BluredSSGIPipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}


	{
		vk::RenderingAttachmentInfo Blured_TA_ImageAttachInfo;
		Blured_TA_ImageAttachInfo.clearValue = clearColor;
		Blured_TA_ImageAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		Blured_TA_ImageAttachInfo.imageView = SSGI_FullScreenQuad->BlurPong_DownSampleQuaterRes.imageView;
		Blured_TA_ImageAttachInfo.loadOp = vk::AttachmentLoadOp::eDontCare;
		Blured_TA_ImageAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingInfo BluredSSGIImageInfo{};
		BluredSSGIImageInfo.layerCount = 1;
		BluredSSGIImageInfo.colorAttachmentCount = 1;
		BluredSSGIImageInfo.pColorAttachments = &Blured_TA_ImageAttachInfo;
		BluredSSGIImageInfo.renderArea.extent.width = SSGI_FullScreenQuad->SSGI_ImageQuaterResolution.width;
		BluredSSGIImageInfo.renderArea.extent.height = SSGI_FullScreenQuad->SSGI_ImageQuaterResolution.height;

		vk::Viewport viewport50{};
		viewport50.x = 0.0f;
		viewport50.y = 0.0f;
		viewport50.width = SSGI_FullScreenQuad->SSGI_ImageQuaterResolution.width;
		viewport50.height = SSGI_FullScreenQuad->SSGI_ImageQuaterResolution.height;
		viewport50.minDepth = 0.0f;
		viewport50.maxDepth = 1.0f;

		vk::Rect2D scissor50{};
		scissor50.offset = imageoffset;
		scissor50.extent.width = SSGI_FullScreenQuad->SSGI_ImageQuaterResolution.width;
		scissor50.extent.height = SSGI_FullScreenQuad->SSGI_ImageQuaterResolution.height;

		commandBuffer.setViewport(0, 1, &viewport50);
		commandBuffer.setScissor(0, 1, &scissor50);
		commandBuffer.beginRendering(BluredSSGIImageInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, BluredSSGIPipeline);


		SSGI_FullScreenQuad->DrawDownSampleQuaterfResSecondPass(commandBuffer, BluredSSGIPipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}


	{
		vk::RenderingAttachmentInfo Blured_TA_ImageAttachInfo;
		Blured_TA_ImageAttachInfo.clearValue = clearColor;
		Blured_TA_ImageAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		Blured_TA_ImageAttachInfo.imageView = SSGI_FullScreenQuad->BlurPing_UPSampleHalfRes.imageView;
		Blured_TA_ImageAttachInfo.loadOp = vk::AttachmentLoadOp::eDontCare;
		Blured_TA_ImageAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingInfo BluredSSGIImageInfo{};
		BluredSSGIImageInfo.layerCount = 1;
		BluredSSGIImageInfo.colorAttachmentCount = 1;
		BluredSSGIImageInfo.pColorAttachments = &Blured_TA_ImageAttachInfo;
		BluredSSGIImageInfo.renderArea.extent.width = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.width;
		BluredSSGIImageInfo.renderArea.extent.height = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.height;

		vk::Viewport viewport50{};
		viewport50.x = 0.0f;
		viewport50.y = 0.0f;
		viewport50.width = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.width;
		viewport50.height = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.height;
		viewport50.minDepth = 0.0f;
		viewport50.maxDepth = 1.0f;

		vk::Rect2D scissor50{};
		scissor50.offset = imageoffset;
		scissor50.extent.width = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.width;
		scissor50.extent.height = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.height;

		commandBuffer.setViewport(0, 1, &viewport50);
		commandBuffer.setScissor(0, 1, &scissor50);
		commandBuffer.beginRendering(BluredSSGIImageInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, BluredSSGIPipeline);


		SSGI_FullScreenQuad->DrawUPSampleHalfResFirstPass(commandBuffer, BluredSSGIPipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}


	{
		vk::RenderingAttachmentInfo Blured_TA_ImageAttachInfo;
		Blured_TA_ImageAttachInfo.clearValue = clearColor;
		Blured_TA_ImageAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		Blured_TA_ImageAttachInfo.imageView = SSGI_FullScreenQuad->BlurPong_UPSampleHalfRes.imageView;
		Blured_TA_ImageAttachInfo.loadOp = vk::AttachmentLoadOp::eDontCare;
		Blured_TA_ImageAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingInfo BluredSSGIImageInfo{};
		BluredSSGIImageInfo.layerCount = 1;
		BluredSSGIImageInfo.colorAttachmentCount = 1;
		BluredSSGIImageInfo.pColorAttachments = &Blured_TA_ImageAttachInfo;
		BluredSSGIImageInfo.renderArea.extent.width = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.width;
		BluredSSGIImageInfo.renderArea.extent.height = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.height;

		vk::Viewport viewport50{};
		viewport50.x = 0.0f;
		viewport50.y = 0.0f;
		viewport50.width = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.width;
		viewport50.height = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.height;
		viewport50.minDepth = 0.0f;
		viewport50.maxDepth = 1.0f;

		vk::Rect2D scissor50{};
		scissor50.offset = imageoffset;
		scissor50.extent.width = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.width;
		scissor50.extent.height = SSGI_FullScreenQuad->SSGI_ImageHalfResolution.height;

		commandBuffer.setViewport(0, 1, &viewport50);
		commandBuffer.setScissor(0, 1, &scissor50);
		commandBuffer.beginRendering(BluredSSGIImageInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, BluredSSGIPipeline);


		SSGI_FullScreenQuad->DrawUPSampleHalfResSecondPass(commandBuffer, BluredSSGIPipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}

	{
		vk::RenderingAttachmentInfo Blured_TA_ImageAttachInfo;
		Blured_TA_ImageAttachInfo.clearValue = clearColor;
		Blured_TA_ImageAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		Blured_TA_ImageAttachInfo.imageView = SSGI_FullScreenQuad->BlurPing_UPSampleFullRes.imageView;
		Blured_TA_ImageAttachInfo.loadOp = vk::AttachmentLoadOp::eDontCare;
		Blured_TA_ImageAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingInfo BluredSSGIImageInfo{};
		BluredSSGIImageInfo.layerCount = 1;
		BluredSSGIImageInfo.colorAttachmentCount = 1;
		BluredSSGIImageInfo.pColorAttachments = &Blured_TA_ImageAttachInfo;
		BluredSSGIImageInfo.renderArea.extent.width = SSGI_FullScreenQuad->SSGI_ImageFullResolution.width;
		BluredSSGIImageInfo.renderArea.extent.height = SSGI_FullScreenQuad->SSGI_ImageFullResolution.height;

		vk::Viewport viewport50{};
		viewport50.x = 0.0f;
		viewport50.y = 0.0f;
		viewport50.width = SSGI_FullScreenQuad->SSGI_ImageFullResolution.width;
		viewport50.height = SSGI_FullScreenQuad->SSGI_ImageFullResolution.height;
		viewport50.minDepth = 0.0f;
		viewport50.maxDepth = 1.0f;

		vk::Rect2D scissor50{};
		scissor50.offset = imageoffset;
		scissor50.extent.width = SSGI_FullScreenQuad->SSGI_ImageFullResolution.width;
		scissor50.extent.height = SSGI_FullScreenQuad->SSGI_ImageFullResolution.height;

		commandBuffer.setViewport(0, 1, &viewport50);
		commandBuffer.setScissor(0, 1, &scissor50);
		commandBuffer.beginRendering(BluredSSGIImageInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, BluredSSGIPipeline);

		SSGI_FullScreenQuad->DrawUPSampleFullResFirstPass(commandBuffer, BluredSSGIPipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}


	{
		vk::RenderingAttachmentInfo Blured_TA_ImageAttachInfo;
		Blured_TA_ImageAttachInfo.clearValue = clearColor;
		Blured_TA_ImageAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		Blured_TA_ImageAttachInfo.imageView = SSGI_FullScreenQuad->BlurPong_UPSampleFullRes.imageView;
		Blured_TA_ImageAttachInfo.loadOp = vk::AttachmentLoadOp::eDontCare;
		Blured_TA_ImageAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingInfo BluredSSGIImageInfo{};
		BluredSSGIImageInfo.layerCount = 1;
		BluredSSGIImageInfo.colorAttachmentCount = 1;
		BluredSSGIImageInfo.pColorAttachments = &Blured_TA_ImageAttachInfo;
		BluredSSGIImageInfo.renderArea.extent.width = SSGI_FullScreenQuad->SSGI_ImageFullResolution.width;
		BluredSSGIImageInfo.renderArea.extent.height = SSGI_FullScreenQuad->SSGI_ImageFullResolution.height;

		vk::Viewport viewport50{};
		viewport50.x = 0.0f;
		viewport50.y = 0.0f;
		viewport50.width = SSGI_FullScreenQuad->SSGI_ImageFullResolution.width;
		viewport50.height = SSGI_FullScreenQuad->SSGI_ImageFullResolution.height;
		viewport50.minDepth = 0.0f;
		viewport50.maxDepth = 1.0f;

		vk::Rect2D scissor50{};
		scissor50.offset = imageoffset;
		scissor50.extent.width = SSGI_FullScreenQuad->SSGI_ImageFullResolution.width;
		scissor50.extent.height = SSGI_FullScreenQuad->SSGI_ImageFullResolution.height;

		commandBuffer.setViewport(0, 1, &viewport50);
		commandBuffer.setScissor(0, 1, &scissor50);
		commandBuffer.beginRendering(BluredSSGIImageInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, BluredSSGIPipeline);

		SSGI_FullScreenQuad->DrawUPSampleFullResSecondPass(commandBuffer, BluredSSGIPipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}
}
	vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);

	{
		vk::RenderingAttachmentInfo SkyBoxRenderAttachInfo;
		SkyBoxRenderAttachInfo.clearValue = clearColor;
		SkyBoxRenderAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		SkyBoxRenderAttachInfo.imageView = lighting_RTX->ResultingStorageImage.imageView;
		SkyBoxRenderAttachInfo.loadOp = vk::AttachmentLoadOp::eLoad;
		SkyBoxRenderAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;
		
		vk::RenderingAttachmentInfo DepthAttachInfo;
		DepthAttachInfo.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
		DepthAttachInfo.imageView = DepthTextureData.imageView;
		DepthAttachInfo.loadOp = vk::AttachmentLoadOp::eLoad;
		DepthAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;
		DepthAttachInfo.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
		
		vk::RenderingInfo SkyBoxRenderInfo{};
		SkyBoxRenderInfo.layerCount = 1;
		SkyBoxRenderInfo.colorAttachmentCount = 1;
		SkyBoxRenderInfo.pColorAttachments = &SkyBoxRenderAttachInfo;
		SkyBoxRenderInfo.pDepthAttachment = &DepthAttachInfo;
		SkyBoxRenderInfo.renderArea.extent.width = vulkanContext.swapchainExtent.width;
		SkyBoxRenderInfo.renderArea.extent.height = vulkanContext.swapchainExtent.height;
		
		
		commandBuffer.setViewport(0, 1, &viewport);
		commandBuffer.setScissor(0, 1, &scissor);
		commandBuffer.beginRendering(SkyBoxRenderInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, SkyBoxgraphicsPipeline);
		skyBox->Draw(commandBuffer, SkyBoxpipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}


	{
		vk::RenderingAttachmentInfo CombinedImageAttachInfo;
		CombinedImageAttachInfo.clearValue = clearColor;
		CombinedImageAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		CombinedImageAttachInfo.imageView = Combined_FullScreenQuad->FinalResultImage.imageView;
		CombinedImageAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
		CombinedImageAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingInfo CombinedImageInfo{};
		CombinedImageInfo.layerCount = 1;
		CombinedImageInfo.colorAttachmentCount = 1;
		CombinedImageInfo.pColorAttachments = &CombinedImageAttachInfo;
		CombinedImageInfo.renderArea.extent.width = vulkanContext.swapchainExtent.width;
		CombinedImageInfo.renderArea.extent.height = vulkanContext.swapchainExtent.height;

		commandBuffer.setViewport(0, 1, &viewport);
		commandBuffer.setScissor(0, 1, &scissor);
		commandBuffer.beginRendering(CombinedImageInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, CombinedImagePassPipeline);
		Combined_FullScreenQuad->Draw(commandBuffer, CombinedImagePipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}


	{

		vk::RenderingAttachmentInfo LightPassColorAttachmentInfo{};
		LightPassColorAttachmentInfo.imageView = Combined_FullScreenQuad->FinalResultImage.imageView;;
		LightPassColorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		LightPassColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eLoad;
		LightPassColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		LightPassColorAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfo depthStencilAttachment;
		depthStencilAttachment.imageView = DepthTextureData.imageView;
		depthStencilAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		depthStencilAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
		depthStencilAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		depthStencilAttachment.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

		vk::RenderingInfo renderingInfo{};
		renderingInfo.renderArea.offset = imageoffset;
		renderingInfo.renderArea.extent.height = vulkanContext.swapchainExtent.height;
		renderingInfo.renderArea.extent.width = vulkanContext.swapchainExtent.width;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &LightPassColorAttachmentInfo;
		renderingInfo.pDepthAttachment = &depthStencilAttachment;

		if (bWireFrame)
		{
			vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_LINE);
		}
		else
		{
			vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_FILL);
		}

		commandBuffer.setViewport(0, 1, &viewport);
		commandBuffer.setScissor(0, 1, &scissor);
		commandBuffer.beginRendering(renderingInfo);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, LightgraphicsPipeline);

		for (auto& light : lights)
		{
			light->Draw(commandBuffer, LightpipelineLayout, currentFrame);
		}

		commandBuffer.endRendering();
	}


	{

		vk::RenderingAttachmentInfo ProbeDrawColorAttachmentInfo{};
		ProbeDrawColorAttachmentInfo.imageView = Combined_FullScreenQuad->FinalResultImage.imageView;;
		ProbeDrawColorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		ProbeDrawColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eLoad;
		ProbeDrawColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		ProbeDrawColorAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfo depthStencilAttachment;
		depthStencilAttachment.imageView = DepthTextureData.imageView;
		depthStencilAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		depthStencilAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
		depthStencilAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		depthStencilAttachment.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

		vk::RenderingInfo renderingInfo{};
		renderingInfo.renderArea.offset = imageoffset;
		renderingInfo.renderArea.extent.height = vulkanContext.swapchainExtent.height;
		renderingInfo.renderArea.extent.width = vulkanContext.swapchainExtent.width;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &ProbeDrawColorAttachmentInfo;
		renderingInfo.pDepthAttachment = &depthStencilAttachment;

		if (bWireFrame)
		{
			vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_LINE);
		}
		else
		{
			vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_FILL);
		}

		commandBuffer.setViewport(0, 1, &viewport);
		commandBuffer.setScissor(0, 1, &scissor);
		commandBuffer.beginRendering(renderingInfo);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, DDGIProbePipeline);

		dynamicDiffuse_RTGI->Draw(commandBuffer, DDGIProbepipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}

	/////////////////// FORWARD PASS END ///////////////////////// 
	vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_FILL);

	vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, FXAA_Label);
	{
		vk::RenderingAttachmentInfo LightPassColorAttachmentInfo{};
		LightPassColorAttachmentInfo.imageView = fxaa_FullScreenQuad->FxaaImage.imageView;
		LightPassColorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		LightPassColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		LightPassColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		LightPassColorAttachmentInfo.clearValue = clearColor;

		vk::RenderingInfo renderingInfo{};
		renderingInfo.renderArea.offset = imageoffset;
		renderingInfo.renderArea.extent.height = vulkanContext.swapchainExtent.height;
		renderingInfo.renderArea.extent.width = vulkanContext.swapchainExtent.width;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &LightPassColorAttachmentInfo;

		commandBuffer.setViewport(0, 1, &viewport);
		commandBuffer.setScissor(0, 1, &scissor);

		commandBuffer.beginRendering(renderingInfo);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, FXAAPassPipeline);
		fxaa_FullScreenQuad->Draw(commandBuffer, FXAAPassPipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}
	vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);


	userinterface.RenderUi(commandBuffer, imageIndex);

}

void App::destroy_DepthImage()
{
	bufferManger.DestroyImage(DepthTextureData);
}

void App::destroy_GbufferImages()
{
	bufferManger.DestroyImage(gbuffer.Position);
	bufferManger.DestroyImage(gbuffer.ViewSpacePosition);
	bufferManger.DestroyImage(gbuffer.Normal);
	bufferManger.DestroyImage(gbuffer.ViewSpaceNormal);
	bufferManger.DestroyImage(gbuffer.Materials);
	bufferManger.DestroyImage(gbuffer.Albedo);
	bufferManger.DestroyImage(gbuffer.Emissive);
	bufferManger.DestroyImage(gbuffer.MotionVector);
	bufferManger.DestroyImage(gbuffer.PrevNormal);

	ssao_FullScreenQuad->DestroyImage();
	fxaa_FullScreenQuad->DestroyImage();
	SSGI_FullScreenQuad->DestroyImage();
	Combined_FullScreenQuad->DestroyImage();
	RT_Reflection->DestroyStorageImage();
	dynamicDiffuse_RTGI->DestroySampledGIImage();
	Restir_DI->DestroyImage();
	lighting_RTX->DestroyStorageImage();


}


void App::recreateSwapChain() {
	
	int width = 0, height = 0;
	glfwGetFramebufferSize(window.GetWindow(), &width, &height);

	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window.GetWindow(), &width, &height);
		glfwWaitEvents();
	}

	vulkanContext.LogicalDevice.waitIdle();

	vulkanContext.destroy_swapchain();
	destroy_DepthImage();
	destroy_GbufferImages();

	vulkanContext.LogicalDevice.waitIdle();

	vulkanContext.create_swapchain();

	camera.SetSwapchainHeight(vulkanContext.swapchainExtent.height);
	camera.SetSwapchainWidth(vulkanContext.swapchainExtent.width);
	createDepthTextureImage();
	createGBuffer();

}

void App::recreatePipeline()
{
	vulkanContext.LogicalDevice.waitIdle();
	destroyPipeline();

	CreateGraphicsPipeline();
}


void App::DestroySyncObjects()
{
	for (auto& presentSemaphores : presentCompleteSemaphores)
	{
		vulkanContext.LogicalDevice.destroySemaphore(presentSemaphores);
	}

	for (auto& renderSemaphores : renderCompleteSemaphores)
	{
		vulkanContext.LogicalDevice.destroySemaphore(renderSemaphores);
	}

	for (auto& Fences : waitFences)
	{
		vulkanContext.LogicalDevice.destroyFence(Fences);
	}
}

void App::DestroyTLAS()
{
	if (TLAS_Buffer.buffer) {

		bufferManger.DestroyBuffer(TLAS_Buffer);
		bufferManger.DestroyBuffer(TLAS_SCRATCH_Buffer);
		bufferManger.DestroyBuffer(TLAS_InstanceData);
		vulkanContext.vkDestroyAccelerationStructureKHR(vulkanContext.LogicalDevice, TLAS, nullptr);
	}
}

void App::DestroyBuffers()
{

	destroy_DepthImage();
	destroy_GbufferImages();

	for (auto& model : SponzaSceneModels)
	{
		model.reset();
	}

	for (auto& model : CornelSceneModels)
	{
		model.reset();
	}

	for (auto& model : AltCornelSceneModels)
	{
		model.reset();
	}

	for (auto& light : lights)
	{
		light.reset();
	}

	skyBox.reset();

	lighting_RTX.reset();
	ssao_FullScreenQuad.reset();
	fxaa_FullScreenQuad.reset();
	SSGI_FullScreenQuad.reset();
	Combined_FullScreenQuad.reset();
	Restir_DI.reset();
	RT_Reflection.reset();
	dynamicDiffuse_RTGI.reset();
	DestroyTLAS();
	bufferManger.DestroySharedBuffers();
	DestroyShaderBindingTable();
	//bufferManger.reset();
}

void App::destroyPipeline()
{
	vulkanContext.LogicalDevice.destroyPipeline(DeferedLightingPassPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(FXAAPassPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(LightgraphicsPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(SkyBoxgraphicsPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(geometryPassPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(SSAOPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(SSAOBlurPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(SSRPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(RT_ShadowsPassPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(RT_ReflectionPassPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(SSGIPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(BluredSSGIPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(TA_SSGIPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(CombinedImagePassPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(BluredRTreflectionPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(DDGIProbePipeline);
	vulkanContext.LogicalDevice.destroyPipeline(GridComputePassPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(RT_DDGIPassPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(IrradianceComputePassPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(SampleDDGIComputePassPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(ProbeStatusComputePassPipeline);
	vulkanContext.LogicalDevice.destroyPipeline(ReSTIR_RTPassPipeline);


	vulkanContext.LogicalDevice.destroyPipelineLayout(DeferedLightingPassPipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(FXAAPassPipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(LightpipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(SkyBoxpipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(geometryPassPipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(SSAOPipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(SSAOBlurPipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(SSRPipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(RT_ShadowsPipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(RT_ReflectionPipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(SSGIPipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(TA_SSGIPipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(BluredSSGIPipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(CombinedImagePipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(BluredRTreflectionsPipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(DDGIProbepipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(GridComputePipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(RT_DDGIPipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(IrradianceComputePipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(SampleDDGIComputePipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(ProbeStatusPipelineLayout);
	vulkanContext.LogicalDevice.destroyPipelineLayout(ReSTIR_RT_PipelineLayout);

}


 App::~App()
{

	//userinterface.reset();
	DestroyBuffers();

	if (!commandBuffers.empty()) {
		vulkanContext.LogicalDevice.freeCommandBuffers(commandPool, commandBuffers);
		commandBuffers.clear();
	}
	vulkanContext.LogicalDevice.destroyDescriptorPool(DescriptorPool);
	vulkanContext.LogicalDevice.destroyCommandPool(commandPool);

	destroyPipeline();

	DestroySyncObjects();
	vkb::destroy_debug_utils_messenger(vulkanContext.VulkanInstance, vulkanContext.Debug_Messenger);
	//vulkanContext.reset();


	//window.reset();
#ifndef NDEBUG
	_CrtDumpMemoryLeaks();  
#endif

}

void App::SwapchainResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}

