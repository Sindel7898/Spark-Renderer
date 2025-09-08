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
#include "RT_Reflections.h"
#include "CombinedResult_FullScreenQuad.h"
#include "SSGI.h"
#include "NRD.h"
#include "NRI.h"
#include "Extensions/NRIHelper.h"
#include "Extensions/NRIDeviceCreation.h"
#include "NRDIntegration.hpp"
#include <crtdbg.h>
#include "SkyBox.h"
#include "Model.h"
#include "UserInterface.h"
#include "Pipeline_Manager.h"
#include "FXAA_FullScreenQuad.h"
#include "SSR_FullScreenQuad.h"
#include "RT_Shadows.h"

#define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)

 App::App()
{
	window        = std::shared_ptr<Window>(new Window(1920, 1080, "Spark Renderer"), WindowDeleter);
	vulkanContext = std::shared_ptr<VulkanContext>(new VulkanContext(*window), VulkanContextDeleter);
	bufferManger  = std::shared_ptr<BufferManager>(new BufferManager (vulkanContext.get()), BufferManagerDeleter);
  	camera        = std::shared_ptr<Camera>(new Camera (vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, window->GetWindow()));
	userinterface = std::shared_ptr<UserInterface>(new UserInterface(vulkanContext.get(), window.get(), bufferManger.get()),UserInterfaceDeleter);
	glfwSetWindowUserPointer(window->GetWindow(), this);

	createSyncObjects();	
	//////////////////////////
	createCommandPool();




	skyBox = std::shared_ptr<SkyBox>(new SkyBox(vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), SkyBoxDeleter);


	//auto model  = std::shared_ptr<Model>(new Model("../Textures/Helmet/Helmet.gltf"   , vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), ModelDeleter);
	//auto model2 = std::shared_ptr<Model>(new Model("../Textures/Horse/Horse.gltf", vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), ModelDeleter);
	//auto model3 = std::shared_ptr<Model>(new Model("../Textures/Bunny/scene.gltf", vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), ModelDeleter);
	//auto model4 = std::shared_ptr<Model>(new Model("../Textures/Wall/Cube.gltf"            , vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), ModelDeleter);
	//auto model5 = std::shared_ptr<Model>(new Model("../Textures/Wall2/Cube.gltf", vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), ModelDeleter);
	//auto model6 = std::shared_ptr<Model>(new Model("../Textures/Wall3/Cube.gltf", vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), ModelDeleter);
	//auto model7 = std::shared_ptr<Model>(new Model("../Textures/Wall4/Cube.gltf", vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), ModelDeleter);
	//auto model8 = std::shared_ptr<Model>(new Model("../Textures/Dragon/scene.gltf", vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), ModelDeleter);
	auto model9 = std::shared_ptr<Model>(new Model("../Textures/Bistro/Untitled.gltf", vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), ModelDeleter);
	//auto model10 = std::shared_ptr<Model>(new Model("../Textures/PBR_Sponza/Sponza.gltf", vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), ModelDeleter);
	
	//model.get()->Instances[0]->SetPostion(glm::vec3(1.702, -9.761, 5.964));
	//model.get()->Instances[0]->SetRotation(glm::vec3(0.000, 0.000, 0.00));
	//model.get()->Instances[0]->SetScale(glm::vec3(1.500, 1.500, 1.500));
	//
	//model2.get()->Instances[0]->SetPostion(glm::vec3(0.024, -11.111, -4.403));
	//model2.get()->Instances[0]->SetScale(glm::vec3(50.000, 50.000, 50.000));
	//model2.get()->Instances[0]->SetRotation(glm::vec3(0.000, 0.000, 0.00));
	//
	//model3.get()->Instances[0]->SetPostion(glm::vec3(-13.581, -11.309, -0.131));
	//model3.get()->Instances[0]->SetRotation(glm::vec3(0.000, 0.000, 0.00));
	//model3.get()->Instances[0]->SetScale(glm::vec3(0.040, 0.040, 0.040));
	//
	//model4.get()->Instances[0]->SetPostion(glm::vec3(-21.740, -3.316, -1.843));
	//model4.get()->Instances[0]->SetRotation(glm::vec3(90.000, 90.000, -180.0));
	//model4.get()->Instances[0]->SetScale(glm::vec3(0.500, 0.500, 1.000));
	//
	//model5.get()->Instances[0]->SetPostion(glm::vec3(24.404, -3.275, -1.251));
	//model5.get()->Instances[0]->SetRotation(glm::vec3(93.814, 90.000, -180.000));
	//model5.get()->Instances[0]->SetScale(glm::vec3(0.500, 0.500, 1.000));
	//
	//model6.get()->Instances[0]->SetPostion(glm::vec3(3.159, -11.066, -1.801));
	//model6.get()->Instances[0]->SetRotation(glm::vec3(0.000, -0.000, 0.00));
	//model6.get()->Instances[0]->SetScale(glm::vec3(1.158, 0.054, 1.270));
	//
	//model7.get()->Instances[0]->SetPostion(glm::vec3(2.904, -5.455, -11.447));
	//model7.get()->Instances[0]->SetRotation(glm::vec3(90.000, 0.003, 0.000));
	//model7.get()->Instances[0]->SetScale(glm::vec3(1.200, 0.050, 1.270));
	//
	//model8.get()->Instances[0]->SetPostion(glm::vec3(14.125, -10.750, 1.885));
	//model8.get()->Instances[0]->SetScale(glm::vec3(0.070, 0.070, 0.070));
	//model8.get()->Instances[0]->SetRotation(glm::vec3(0.000, 22.913, 0.000));
	
	model9.get()->Instances[0]->CubeMapReflectiveSwitch(false);

	////
	////
    //Models.push_back(std::move(model));
    //Models.push_back(std::move(model2));
    //Models.push_back(std::move(model3));
    //Models.push_back(std::move(model4));
    //Models.push_back(std::move(model5));
    //Models.push_back(std::move(model6));
    //Models.push_back(std::move(model7));
    //Models.push_back(std::move(model8));
	Models.push_back(std::move(model9));
	////Models.push_back(std::move(model10));
	////
	UserInterfaceItems.push_back(Models[0].get());
	//UserInterfaceItems.push_back(Models[1].get());
	//UserInterfaceItems.push_back(Models[2].get());
	//UserInterfaceItems.push_back(Models[3].get());
	//UserInterfaceItems.push_back(Models[4].get());
	//UserInterfaceItems.push_back(Models[5].get());
	//UserInterfaceItems.push_back(Models[6].get());
	//UserInterfaceItems.push_back(Models[7].get());
	//UserInterfaceItems.push_back(Models[8].get());


	Raytracing_Shadows      =  std::shared_ptr<RT_Shadows>(new RT_Shadows(vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), RT_ShadowsDeleter);
	//Raytracing_Reflections   = std::shared_ptr<RT_Reflections>(new RT_Reflections(vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), RT_ReflectionsDeleter);

	lighting_FullScreenQuad = std::shared_ptr<Lighting_FullScreenQuad>(new Lighting_FullScreenQuad(bufferManger.get(), vulkanContext.get(), camera.get(), commandPool, skyBox.get(), Raytracing_Shadows.get()), Lighting_FullScreenQuadDeleter);
	ssao_FullScreenQuad     = std::shared_ptr<SSA0_FullScreenQuad>(new SSA0_FullScreenQuad(bufferManger.get(), vulkanContext.get(), camera.get(), commandPool), SSA0_FullScreenQuadDeleter);
	fxaa_FullScreenQuad     = std::shared_ptr<FXAA_FullScreenQuad>(new FXAA_FullScreenQuad(bufferManger.get(), vulkanContext.get(), camera.get(), commandPool), FXAA_FullScreenQuadDeleter);
	ssr_FullScreenQuad      = std::shared_ptr<SSR_FullScreenQuad>(new SSR_FullScreenQuad(bufferManger.get(), vulkanContext.get(), camera.get(), commandPool), SSR_FullScreenQuadDeleter);
	Combined_FullScreenQuad = std::shared_ptr<CombinedResult_FullScreenQuad>(new CombinedResult_FullScreenQuad(bufferManger.get(), vulkanContext.get(), camera.get(), commandPool), CombinedResult_FullScreenQuadDeleter);
	SSGI_FullScreenQuad     = std::shared_ptr<SSGI>(new SSGI(bufferManger.get(), vulkanContext.get(), camera.get(), commandPool), SSGIDeleter);
	pipelineManager         = std::shared_ptr<PipelineManager>(new PipelineManager(vulkanContext.get()));

	lights.reserve(4);

	for (int i = 0; i < 4; i++) {
		std::shared_ptr<Light> light = std::shared_ptr<Light>(new Light(vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), LightDeleter);
		lights.push_back(std::move(light));
	}

	lights[0]->SetPosition(glm::vec3(-4.351, 5.012, -4.192));
	lights[0]->lightType = 1;
	lights[0]->lightIntensity = 3.000;
	lights[0]->CastShadowsSwitch(true);
	lights[0]->ambientStrength = 0.1;
	lights[0]->SetScale(glm::vec3(0.200, 0.200, 0.200));
	
	lights[1]->SetPosition(glm::vec3(23.352, -15.081, 24.001));
	lights[1]->SetScale(glm::vec3(0.200, 0.200, 0.200));
	lights[1]->CastShadowsSwitch(true);
	lights[1]->ambientStrength = 0.100;
	lights[1]->lightIntensity = 5.000;
	lights[1]->lightType = 0;
	
	
	lights[2]->SetPosition(glm::vec3(-1.830, 4.918, 10.112));
	lights[2]->lightType = 1;
	lights[2]->lightIntensity = 3.000;
	lights[2]->CastShadowsSwitch(true);
	lights[2]->ambientStrength = 0.1;
	lights[2]->SetScale(glm::vec3(0.200, 0.200, 0.200));
	
	
	lights[3]->SetPosition(glm::vec3(3.723, 5.277, -21.320));
	lights[3]->lightType = 1;
	lights[3]->lightIntensity = 3.000;
	lights[3]->CastShadowsSwitch(true);
	lights[3]->ambientStrength = 0.1;
	lights[3]->SetScale(glm::vec3(0.200, 0.200, 0.200));
	
	
	lights[0]->color = glm::vec3(0.431, 0.337, 0.318);
	lights[1]->color = glm::vec3(0.980, 0.357, 0.000);
	lights[2]->color = glm::vec3(0.431, 0.337, 0.318);
	lights[3]->color = glm::vec3(0.431, 0.337, 0.318);


    //lights[0]->SetPosition(glm::vec3(2.146, 3.298, 2.924));
	//lights[0]->lightType = 1;
	//lights[0]->lightIntensity = 5.000;
	//lights[0]->CastShadowsSwitch(true);
	//lights[0]->ambientStrength = 0;
	//lights[0]->SetScale(glm::vec3(0.200, 0.200, 0.200));
	//
	//lights[1]->SetPosition(glm::vec3(-13.788, -1.738, 7.216));
	//lights[1]->SetScale(glm::vec3(0.200, 0.200, 0.200));
	//lights[1]->CastShadowsSwitch(true);
	//lights[1]->ambientStrength = 0;
	//lights[1]->lightIntensity = 5.000;
	//lights[1]->lightType = 1;
	//
	//
	//lights[2]->SetPosition(glm::vec3(21.394, -2.795, 4.368));
	//lights[2]->lightType = 1;
	//lights[2]->lightIntensity = 5.000;
	//lights[2]->CastShadowsSwitch(true);
	//lights[2]->ambientStrength = 0;
	//lights[2]->SetScale(glm::vec3(0.200, 0.200, 0.200));
	//
	//
	//lights[3]->SetPosition(glm::vec3(1.498, -2.099, 23.667));
	//lights[3]->lightType = 1;
	//lights[3]->lightIntensity = 10.000;
	//lights[3]->CastShadowsSwitch(true);
	//lights[3]->ambientStrength = 0;
	//lights[3]->SetScale(glm::vec3(0.200, 0.200, 0.200));
	//
	//
	//lights[0]->color = glm::vec3(0, 0, 1);
	//lights[1]->color = glm::vec3(0.827, 0.0, 1.0);
	//lights[2]->color = glm::vec3(0.0, 0.859, 0.980);
	//lights[3]->color = glm::vec3(0.996, 0.827, 0.0);


	for (auto& l : lights) {
		UserInterfaceItems.push_back(l.get());
	}


	createDescriptorPool();

	createCommandBuffer();
	CreateGraphicsPipeline();
	createShaderBindingTable();
	createDepthTextureImage();

	createTLAS();

	createGBuffer();

	recreateSwapChain();
}

 void App::createTLAS()
 {
	 // Create instance Buffer
	 TLAS_InstanceData.BufferID = "Scene TLAS InstanceData Buffer";
	 size_t totalSize = sizeof(vk::AccelerationStructureInstanceKHR) * Models.size();

	 bufferManger->CreateBuffer(&TLAS_InstanceData, totalSize,
		 vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
		 vk::BufferUsageFlagBits::eShaderDeviceAddress, commandPool, vulkanContext->graphicsQueue);

	 UpdateTLASInstanceBuffer();

	 ////////////////////////////////////GEOMETRY INFO /////////////////////////////////////////////////////////////////////
	 //get instance buffer adddress
	 vk::BufferDeviceAddressInfo InstanceInfo{};
	 InstanceInfo.buffer = TLAS_InstanceData.buffer;

	 vk::DeviceOrHostAddressConstKHR instanceDataDeviceAddresstance{};
	 instanceDataDeviceAddresstance.deviceAddress = vulkanContext->LogicalDevice.getBufferAddress(InstanceInfo); // pass instance buffer address
	 

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



	 uint32_t primitive_count = static_cast<uint32_t>(Models.size());

	 VkAccelerationStructureBuildGeometryInfoKHR TEMP_ACCELERATION_INFO = accelerationStructureBuildGeometryInfo;
	 VkAccelerationStructureBuildSizesInfoKHR TEMP_ACCELERATION_STRUCTURE_BUILD_SIZE{};
	 TEMP_ACCELERATION_STRUCTURE_BUILD_SIZE.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	 TEMP_ACCELERATION_STRUCTURE_BUILD_SIZE.pNext = nullptr;

	 vulkanContext->vkGetAccelerationStructureBuildSizesKHR(vulkanContext->LogicalDevice, 
		                                                    VkAccelerationStructureBuildTypeKHR::VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, 
		                                                     &TEMP_ACCELERATION_INFO, &primitive_count, &TEMP_ACCELERATION_STRUCTURE_BUILD_SIZE);

	 vk::AccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = TEMP_ACCELERATION_STRUCTURE_BUILD_SIZE;
	 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	                                               ///////CREATE TLAS BUFFER////////
	 TLAS_Buffer.BufferID = "Scene TLAS Buffer";

	 bufferManger->CreateDeviceBuffer(&TLAS_Buffer,
		                              accelerationStructureBuildSizesInfo.accelerationStructureSize,
		                              vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
		                              vk::BufferUsageFlagBits::eShaderDeviceAddress,
		                              commandPool,
		                              vulkanContext->graphicsQueue);

	 // Acceleration structure
     vk::AccelerationStructureCreateInfoKHR accelerationStructureCreate_info{};
	 accelerationStructureCreate_info.buffer = TLAS_Buffer.buffer;
	 accelerationStructureCreate_info.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
	 accelerationStructureCreate_info.type = vk::AccelerationStructureTypeKHR::eTopLevel;

	 VkAccelerationStructureCreateInfoKHR TEMP_ACCELERATION_STRUCTURE_CREATE_INFO = accelerationStructureCreate_info;
	 VkAccelerationStructureKHR TEMP_TLAS;
	 vulkanContext->vkCreateAccelerationStructureKHR(vulkanContext->LogicalDevice, &TEMP_ACCELERATION_STRUCTURE_CREATE_INFO, nullptr, &TEMP_TLAS);
	 TLAS = TEMP_TLAS;
	 
	 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
											///////CREATE TLAS SCRATCH BUFFER////////

	 TLAS_SCRATCH_Buffer.BufferID = "TLAS_ScratchBuffer Buffer";
	 bufferManger->CreateDeviceBuffer(&TLAS_SCRATCH_Buffer,
		                               accelerationStructureBuildSizesInfo.buildScratchSize,
		                               vk::BufferUsageFlagBits::eStorageBuffer |
		                               vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
		                               vk::BufferUsageFlagBits::eShaderDeviceAddress,
		                               commandPool,
		                               vulkanContext->graphicsQueue);

	 vk::BufferDeviceAddressInfo TLAS_ScratchBufferAdress;
	 TLAS_ScratchBufferAdress.buffer = TLAS_SCRATCH_Buffer.buffer;

	 accelerationStructureBuildGeometryInfo.dstAccelerationStructure = TLAS;
	 accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = vulkanContext->LogicalDevice.getBufferAddress(TLAS_ScratchBufferAdress);
	 accelerationStructureBuildGeometryInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
	 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

												///////BUILD TLAS ON THE GPU ////////

	 vk::CommandBuffer cmd = bufferManger->CreateSingleUseCommandBuffer(commandPool);

	 vk::AccelerationStructureBuildRangeInfoKHR BuildRangeInfo;
	 BuildRangeInfo.firstVertex     = 0;
	 BuildRangeInfo.primitiveCount  = primitive_count;
	 BuildRangeInfo.primitiveOffset = 0;
	 BuildRangeInfo.transformOffset = 0;

	 VkAccelerationStructureBuildRangeInfoKHR tempRange = BuildRangeInfo;
	 std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &tempRange };

	 VkAccelerationStructureBuildGeometryInfoKHR tempGeometryInfo = accelerationStructureBuildGeometryInfo;

	 vulkanContext->vkCmdBuildAccelerationStructuresKHR(cmd, 1,
		 &tempGeometryInfo,
		 accelerationBuildStructureRangeInfos.data());

	 bufferManger->SubmitAndDestoyCommandBuffer(commandPool, cmd, vulkanContext->graphicsQueue);
	 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 }

 void App::UpdateTLAS()
 {
	 // 1. Update instance data on the GPU
	 UpdateTLASInstanceBuffer();

	 // 2. Reuse instance buffer device address
	 vk::BufferDeviceAddressInfo instanceInfo{};
	 instanceInfo.buffer = TLAS_InstanceData.buffer;

	 vk::DeviceOrHostAddressConstKHR instanceDeviceAddress{};
	 instanceDeviceAddress.deviceAddress = vulkanContext->LogicalDevice.getBufferAddress(instanceInfo);

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
	 buildInfo.scratchData.deviceAddress = vulkanContext->LogicalDevice.getBufferAddress(scratchAddrInfo);

	 // 6. Build range info
	 vk::AccelerationStructureBuildRangeInfoKHR buildRange{};
	 buildRange.primitiveCount = static_cast<uint32_t>(Models.size());
	 buildRange.primitiveOffset = 0;
	 buildRange.firstVertex = 0;
	 buildRange.transformOffset = 0;

	 VkAccelerationStructureBuildRangeInfoKHR tempRange = buildRange;
	 std::vector<VkAccelerationStructureBuildRangeInfoKHR*> rangeInfos = { &tempRange };

	 // 7. Record and submit command buffer
	 vk::CommandBuffer cmd = bufferManger->CreateSingleUseCommandBuffer(commandPool);
	 VkAccelerationStructureBuildGeometryInfoKHR tempBuildInfo = buildInfo;

	 vulkanContext->vkCmdBuildAccelerationStructuresKHR(cmd, 1, &tempBuildInfo, rangeInfos.data());
	 bufferManger->SubmitAndDestoyCommandBuffer(commandPool, cmd, vulkanContext->graphicsQueue);
 }


 void App::UpdateTLASInstanceBuffer()
 {

	 std::vector< vk::AccelerationStructureInstanceKHR> Instances; // array of instances

	 // pupulate instance data into the array 
	 for (int i = 0; i < Models.size(); i++)
	 {
		 vk::AccelerationStructureDeviceAddressInfoKHR blasinfo{};
		 blasinfo.accelerationStructure = Models[i]->BLAS;

		 VkAccelerationStructureDeviceAddressInfoKHR Temp = blasinfo;

		 glm::mat ModelMatrix = Models[i]->Instances[0]->GetModelMatrix();

		 VkTransformMatrixKHR transformMatrix = {
				ModelMatrix[0][0], ModelMatrix[1][0], ModelMatrix[2][0], ModelMatrix[3][0], // Row 0
				ModelMatrix[0][1], ModelMatrix[1][1], ModelMatrix[2][1], ModelMatrix[3][1], // Row 1
				ModelMatrix[0][2], ModelMatrix[1][2], ModelMatrix[2][2], ModelMatrix[3][2], // Row 2
		 };

		 vk::AccelerationStructureInstanceKHR instance{};
		 instance.transform = transformMatrix;
		 instance.instanceCustomIndex = i;
		 instance.mask = 0xFF;
		 instance.instanceShaderBindingTableRecordOffset = 0;
		 instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		 instance.accelerationStructureReference = vulkanContext->vkGetAccelerationStructureDeviceAddressKHR(vulkanContext->LogicalDevice, &Temp);

		 Instances.push_back(instance);
	 }
	 // send all instance data into the buffer
	 bufferManger->CopyDataToBuffer(Instances.data(), TLAS_InstanceData);
 }


void App::createDescriptorPool()
{
	vk::DescriptorPoolSize Uniformpoolsize;
	Uniformpoolsize.type = vk::DescriptorType::eUniformBuffer;
	Uniformpoolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) *  100;

	vk::DescriptorPoolSize Samplerpoolsize;
	Samplerpoolsize.type = vk::DescriptorType::eCombinedImageSampler;
	Samplerpoolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 100;

	vk::DescriptorPoolSize AccelerationStructurepoolsize;
	AccelerationStructurepoolsize.type = vk::DescriptorType::eAccelerationStructureKHR;
	AccelerationStructurepoolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

	vk::DescriptorPoolSize StorageImagepoolsize;
	StorageImagepoolsize.type = vk::DescriptorType::eStorageImage;
	StorageImagepoolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 10;

	std::array<	vk::DescriptorPoolSize, 4> poolSizes{ Uniformpoolsize ,Samplerpoolsize,
		                                              AccelerationStructurepoolsize,StorageImagepoolsize };

	vk::DescriptorPoolCreateInfo poolInfo{};
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 1000;

	DescriptorPool = vulkanContext->LogicalDevice.createDescriptorPool(poolInfo, nullptr);

	for(auto& model : Models)
	{
		model->createDescriptorSets(DescriptorPool);
	}

	skyBox->createDescriptorSets(DescriptorPool);

	for (auto& light : lights)
	{
		light->createDescriptorSets(DescriptorPool);
	}
}


void App::createDepthTextureImage()
{
	vk::Extent3D swapchainextent = vk::Extent3D(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, 1);


	DepthTextureData.ImageID = "Depth Texture";
	bufferManger->CreateImage(&DepthTextureData,swapchainextent, vulkanContext->FindCompatableDepthFormat(), vk::ImageUsageFlagBits::eDepthStencilAttachment |vk::ImageUsageFlagBits::eSampled);
	DepthTextureData.imageView = bufferManger->CreateImageView(&DepthTextureData, vulkanContext->FindCompatableDepthFormat(), vk::ImageAspectFlagBits::eDepth);
	DepthTextureData.imageSampler = bufferManger->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	vk::CommandBuffer commandBuffer = bufferManger->CreateSingleUseCommandBuffer(commandPool);

	ImageTransitionData DataToTransitionInfo;
	DataToTransitionInfo.oldlayout = vk::ImageLayout::eUndefined;
	DataToTransitionInfo.newlayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	DataToTransitionInfo.AspectFlag = vk::ImageAspectFlagBits::eDepth;
	//////////////////////////////////////////////////////////////////////////////
	DataToTransitionInfo.SourceAccessflag = vk::AccessFlagBits::eNone;
	DataToTransitionInfo.DestinationAccessflag = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	///////////////////////////////////////////////////////////////////////////////
	DataToTransitionInfo.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	DataToTransitionInfo.DestinationOnThePipeline = vk::PipelineStageFlagBits::eEarlyFragmentTests;

	bufferManger->TransitionImage(commandBuffer, &DepthTextureData, DataToTransitionInfo);

	bufferManger->SubmitAndDestoyCommandBuffer(commandPool, commandBuffer, vulkanContext->graphicsQueue);

}


void App::createGBuffer()
{
	vulkanContext->ResetTemporalAccumilation();

	vk::Extent3D swapchainextent = vk::Extent3D(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, 1);

	gbuffer.Position.ImageID = "Gbuffer Position Texture";
	bufferManger->CreateImage(&gbuffer.Position,swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.Position.imageView = bufferManger->CreateImageView(&gbuffer.Position, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	gbuffer.Position.imageSampler = bufferManger->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	gbuffer.ViewSpacePosition.ImageID = "Gbuffer Position Texture";
	bufferManger->CreateImage(&gbuffer.ViewSpacePosition,swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.ViewSpacePosition.imageView = bufferManger->CreateImageView(&gbuffer.ViewSpacePosition, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	gbuffer.ViewSpacePosition.imageSampler = bufferManger->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	gbuffer.Normal.ImageID = "Gbuffer WorldSpaceNormal Texture";
	bufferManger->CreateImage(&gbuffer.Normal,swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.Normal.imageView = bufferManger->CreateImageView(&gbuffer.Normal, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	gbuffer.Normal.imageSampler = bufferManger->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	gbuffer.ViewSpaceNormal.ImageID = "Gbuffer ViewSpaceNormal Texture";
	bufferManger->CreateImage(&gbuffer.ViewSpaceNormal,swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.ViewSpaceNormal.imageView = bufferManger->CreateImageView(&gbuffer.ViewSpaceNormal, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	gbuffer.ViewSpaceNormal.imageSampler = bufferManger->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	gbuffer.Materials.ImageID = "Gbuffer Materials Texture";
	bufferManger->CreateImage(&gbuffer.Materials ,swapchainextent, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.Materials.imageView = bufferManger->CreateImageView(&gbuffer.Materials, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
	gbuffer.Materials.imageSampler = bufferManger->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	gbuffer.Albedo.ImageID = "Gbuffer Albedo Texture";
	bufferManger->CreateImage(&gbuffer.Albedo,swapchainextent, vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.Albedo.imageView = bufferManger->CreateImageView(&gbuffer.Albedo, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
	gbuffer.Albedo.imageSampler = bufferManger->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	LightingPassImageData.ImageID = "Gbuffer LightingPass Texture";
	bufferManger->CreateImage(&LightingPassImageData,swapchainextent, vulkanContext->swapchainformat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	LightingPassImageData.imageView = bufferManger->CreateImageView(&LightingPassImageData, vulkanContext->swapchainformat, vk::ImageAspectFlagBits::eColor);
	LightingPassImageData.imageSampler = bufferManger->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	ReflectionMaskImageData.ImageID = "ReflectionMask Texture";
	bufferManger->CreateImage(&ReflectionMaskImageData, swapchainextent, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	ReflectionMaskImageData.imageView = bufferManger->CreateImageView(&ReflectionMaskImageData, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
	ReflectionMaskImageData.imageSampler = bufferManger->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	fxaa_FullScreenQuad->CreateImage(swapchainextent);
	SSGI_FullScreenQuad->CreateGIImage();
	ssr_FullScreenQuad->CreateImage(swapchainextent);
	Raytracing_Shadows->CreateStorageImage();
	//Raytracing_Reflections->CreateStorageImage();
	Combined_FullScreenQuad->CreateImage(swapchainextent);
	ssao_FullScreenQuad->CreateImage();

	lighting_FullScreenQuad->createDescriptorSetsBasedOnGBuffer(DescriptorPool, &gbuffer,&ReflectionMaskImageData);
	ssao_FullScreenQuad->createDescriptorSetsBasedOnGBuffer(DescriptorPool, gbuffer);
	ssr_FullScreenQuad->createDescriptorSets(DescriptorPool, LightingPassImageData, gbuffer.ViewSpaceNormal,gbuffer.ViewSpacePosition, DepthTextureData, ReflectionMaskImageData,gbuffer.Materials);
	Combined_FullScreenQuad->createDescriptorSetsBasedOnGBuffer(DescriptorPool, LightingPassImageData, SSGI_FullScreenQuad->HalfRes_BluredSSGIAccumilationImage, ssao_FullScreenQuad->BluredSSAOImage, gbuffer.Materials,gbuffer.Albedo);
	fxaa_FullScreenQuad->createDescriptorSets(DescriptorPool, Combined_FullScreenQuad->FinalResultImage);
	Raytracing_Shadows->createRaytracedDescriptorSets(DescriptorPool, TLAS, gbuffer);
	//Raytracing_Reflections->createRaytracedDescriptorSets(DescriptorPool, TLAS, gbuffer);
	SSGI_FullScreenQuad->createDescriptorSets(DescriptorPool,gbuffer, LightingPassImageData,DepthTextureData);

	vk::CommandBuffer cmd =  bufferManger->CreateSingleUseCommandBuffer(commandPool);
	ImageTransitionData TransitionToGeneral{};
	TransitionToGeneral.oldlayout = vk::ImageLayout::eUndefined;
	TransitionToGeneral.newlayout = vk::ImageLayout::eGeneral;
	TransitionToGeneral.AspectFlag = vk::ImageAspectFlagBits::eColor;
	TransitionToGeneral.SourceAccessflag = vk::AccessFlagBits::eNone;
	TransitionToGeneral.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eShaderRead;
	TransitionToGeneral.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	TransitionToGeneral.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eFragmentShader;

	bufferManger->TransitionImage(cmd, &gbuffer.Position, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &gbuffer.ViewSpacePosition, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &gbuffer.Normal, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &gbuffer.ViewSpaceNormal, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &gbuffer.Albedo, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &gbuffer.Materials, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &LightingPassImageData, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &ssr_FullScreenQuad->SSRImage, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &ReflectionMaskImageData, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &Combined_FullScreenQuad->FinalResultImage, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &SSGI_FullScreenQuad->HalfRes_SSGIPassImage, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &SSGI_FullScreenQuad->HalfRes_SSGIPassLastFrameImage, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &SSGI_FullScreenQuad->HalfRes_SSGIAccumilationImage, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &SSGI_FullScreenQuad->HalfRes_HorizontalBluredSSGIAccumilationImage, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &SSGI_FullScreenQuad->HalfRes_BluredSSGIAccumilationImage, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &ssao_FullScreenQuad->SSAOImage, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &ssao_FullScreenQuad->BluredSSAOImage, TransitionToGeneral);
	//bufferManger->TransitionImage(cmd, &Raytracing_Reflections->RT_ReflectionPassImage, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &fxaa_FullScreenQuad->FxaaImage, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &Combined_FullScreenQuad->FinalResultImage, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &SSGI_FullScreenQuad->HalfRes_SSGIPassImage, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &SSGI_FullScreenQuad->HalfRes_HorizontalBluredSSGIAccumilationImage, TransitionToGeneral);
	bufferManger->TransitionImage(cmd, &SSGI_FullScreenQuad->HalfRes_BluredSSGIAccumilationImage, TransitionToGeneral);

	bufferManger->SubmitAndDestoyCommandBuffer(commandPool, cmd,vulkanContext->graphicsQueue);


	FinalRenderTextureId = ImGui_ImplVulkan_AddTexture(fxaa_FullScreenQuad->FxaaImage.imageSampler,
		                                               fxaa_FullScreenQuad->FxaaImage.imageView,
		VK_IMAGE_LAYOUT_GENERAL);

	LightingAndReflectionsRenderTextureId = ImGui_ImplVulkan_AddTexture(LightingPassImageData.imageSampler,
		                                                                LightingPassImageData.imageView,
		VK_IMAGE_LAYOUT_GENERAL);

	Shadow_TextureId       = ImGui_ImplVulkan_AddTexture (Raytracing_Shadows->ShadowPassImages[1].imageSampler,
		                                                  Raytracing_Shadows->ShadowPassImages[1].imageView,
		VK_IMAGE_LAYOUT_GENERAL);

	//Reflection_TextureId = ImGui_ImplVulkan_AddTexture(Raytracing_Reflections->RT_ReflectionPassImage.imageSampler,
		//                                               Raytracing_Reflections->RT_ReflectionPassImage.imageView,
		//VK_IMAGE_LAYOUT_GENERAL);

	
	SSAOTextureId           = ImGui_ImplVulkan_AddTexture(ssao_FullScreenQuad->BluredSSAOImage.imageSampler,
		                                                  ssao_FullScreenQuad->BluredSSAOImage.imageView,
		VK_IMAGE_LAYOUT_GENERAL);


	PositionRenderTextureId = ImGui_ImplVulkan_AddTexture(gbuffer.Position.imageSampler,
		                                                  gbuffer.Position.imageView,
		VK_IMAGE_LAYOUT_GENERAL);

	NormalTextureId         = ImGui_ImplVulkan_AddTexture(gbuffer.Normal.imageSampler,
		                                                  gbuffer.Normal.imageView,
		VK_IMAGE_LAYOUT_GENERAL);


	AlbedoTextureId         = ImGui_ImplVulkan_AddTexture(gbuffer.Albedo.imageSampler,
		                                                  gbuffer.Albedo.imageView,
		VK_IMAGE_LAYOUT_GENERAL);

	SSGITextureId            = ImGui_ImplVulkan_AddTexture(SSGI_FullScreenQuad->HalfRes_BluredSSGIAccumilationImage.imageSampler,
		                                                   SSGI_FullScreenQuad->HalfRes_BluredSSGIAccumilationImage.imageView,
		VK_IMAGE_LAYOUT_GENERAL);

	vulkanContext->ResetTemporalAccumilation();

}

void App::CreateGraphicsPipeline()
{

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
	pipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanContext->swapchainformat;
	pipelineRenderingCreateInfo.depthAttachmentFormat = vulkanContext->FindCompatableDepthFormat();


	vk::PipelineInputAssemblyStateCreateInfo inputAssembleInfo{};
	inputAssembleInfo.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembleInfo.primitiveRestartEnable = vk::False;

	/////////////////////////////////////////////////////////////////////////////
	vk::Viewport viewport{};
	viewport.setX(0.0f);
	viewport.setY(0.0f);
	viewport.setHeight((float)vulkanContext->swapchainExtent.height);
	viewport.setWidth((float)vulkanContext->swapchainExtent.width);
	viewport.setMinDepth(0.0f);
	viewport.setMaxDepth(1.0f);

	vk::Offset2D scissorOffset = { 0,0 };

	vk::Rect2D scissor{};
	scissor.setOffset(scissorOffset);
	scissor.setExtent(vulkanContext->swapchainExtent);

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
	   pipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanContext->swapchainformat;

	   vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};

	   pipelineLayoutInfo.setLayoutCount = 1;
	   pipelineLayoutInfo.setSetLayouts(lighting_FullScreenQuad->descriptorSetLayout);
	   pipelineLayoutInfo.pushConstantRangeCount = 0;
	   pipelineLayoutInfo.pPushConstantRanges = nullptr;

	   FullScreen_Quad_Pipeline_Data  Lighting = pipelineManager->create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/DefferedLightingPass.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);

	   DeferedLightingPassPipelineLayout = Lighting.FQ_PipelineLayout;
	   DeferedLightingPassPipeline = Lighting.FQ_Pipeline;

	}

	{
		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanContext->swapchainformat;

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(glm::vec4));
		range.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(fxaa_FullScreenQuad->descriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;


		FullScreen_Quad_Pipeline_Data  Temp = pipelineManager->create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/FXAA.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);

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

		FullScreen_Quad_Pipeline_Data  Temp = pipelineManager->create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/SSAO_Shader.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);

		SSAOPipelineLayout = Temp.FQ_PipelineLayout;
		SSAOPipeline = Temp.FQ_Pipeline;
	}

	{

		vk::PipelineDepthStencilStateCreateInfo depthStencilState;
		depthStencilState.depthTestEnable = vk::False;
		depthStencilState.depthWriteEnable = vk::False;
		depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.stencilTestEnable = vk::False;

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


		FullScreen_Quad_Pipeline_Data  Temp = pipelineManager->create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/SSAOBlur_Shader.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);

		SSAOBlurPipelineLayout = Temp.FQ_PipelineLayout;
		SSAOBlurPipeline = Temp.FQ_Pipeline;

	}


	{
		std::array<vk::Format, 1> colorFormats = { vulkanContext->swapchainformat };

		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(Combined_FullScreenQuad->descriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;


		FullScreen_Quad_Pipeline_Data  Temp = pipelineManager->create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/CombinedImage.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);

		CombinedImagePipelineLayout = Temp.FQ_PipelineLayout;
		CombinedImagePassPipeline = Temp.FQ_Pipeline;
	}

	{
		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = & vulkanContext->swapchainformat;

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(glm::mat4));
		range.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(ssr_FullScreenQuad->descriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		FullScreen_Quad_Pipeline_Data  Temp = pipelineManager->create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/SSR.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);

		SSRPipelineLayout = Temp.FQ_PipelineLayout;
		SSRPipeline = Temp.FQ_Pipeline;
	}

	{
		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/Light_Shader.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/Light_Shader.frag.spv");

		VkShaderModule VertShaderModule = pipelineManager->createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = pipelineManager->createShaderModule(FragShaderCode);

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
        
		LightpipelineLayout = vulkanContext->LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);
		
		LightgraphicsPipeline = pipelineManager->createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			                                  viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, LightpipelineLayout);

		vulkanContext->LogicalDevice.destroyShaderModule(VertShaderModule);
		vulkanContext->LogicalDevice.destroyShaderModule(FragShaderModule);

	}

	{
		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/SkyBox_Shader.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/SkyBox_Shader.frag.spv");

		VkShaderModule VertShaderModule = pipelineManager->createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = pipelineManager->createShaderModule(FragShaderCode);

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


		 SkyBoxpipelineLayout = vulkanContext->LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		 SkyBoxgraphicsPipeline = pipelineManager->createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			                                  viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, SkyBoxpipelineLayout);

		 vulkanContext->LogicalDevice.destroyShaderModule(VertShaderModule);
		 vulkanContext->LogicalDevice.destroyShaderModule(FragShaderModule);

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

		VkShaderModule VertShaderModule = pipelineManager->createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = pipelineManager->createShaderModule(FragShaderCode);

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

		std::array<vk::Format, 7> colorFormats = {
	                             vk::Format::eR16G16B16A16Sfloat, // Position
								 vk::Format::eR16G16B16A16Sfloat, // ViewSpacePosition
	                             vk::Format::eR16G16B16A16Sfloat, // Normal
					             vk::Format::eR16G16B16A16Sfloat, // // ViewSpaceNormal
	                             vk::Format::eR8G8B8A8Srgb,       // Albedo
								 vk::Format::eR8G8B8A8Unorm,      //Material
								 vk::Format::eR8G8B8A8Unorm       //ReflectionMask
	                             };


		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = colorFormats.size();
		pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();
		pipelineRenderingCreateInfo.depthAttachmentFormat = vulkanContext->FindCompatableDepthFormat();


		vk::DescriptorSetLayout setLayouts[] = { Models[0]->descriptorSetLayout };

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(glm::mat4));
		range.setStageFlags(vk::ShaderStageFlagBits::eVertex);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = setLayouts;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		std::array<vk::PipelineColorBlendAttachmentState, 7> colorBlendAttachments = {
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
			.setBlendEnable(VK_FALSE)
		};

		vk::PipelineColorBlendStateCreateInfo colorBlend{};
		colorBlend.setLogicOpEnable(VK_FALSE);
		colorBlend.setAttachmentCount(colorBlendAttachments.size());
		colorBlend.setPAttachments(colorBlendAttachments.data());

		geometryPassPipelineLayout = vulkanContext->LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		geometryPassPipeline = pipelineManager->createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, geometryPassPipelineLayout);

		vulkanContext->LogicalDevice.destroyShaderModule(VertShaderModule);
		vulkanContext->LogicalDevice.destroyShaderModule(FragShaderModule);
	}

	///////////////////////////////////////////RAY TRACING PIPELINES////////////////////////////////////////////////////////////////

	{
		auto RayGen_ShaderCode        = readFile("../Shaders/Compiled_Shader_Files/raygen.rgen.spv");
		auto RayGenMiss_ShaderCode    = readFile("../Shaders/Compiled_Shader_Files/RayGenMiss.rmiss.spv");

		VkShaderModule RayGen_ShaderModule        = pipelineManager->createShaderModule(RayGen_ShaderCode);
		VkShaderModule RayMiss_ShaderModule       = pipelineManager->createShaderModule(RayGenMiss_ShaderCode);


		vk::PipelineShaderStageCreateInfo RayGen_ShaderStageInfo{};
		RayGen_ShaderStageInfo.sType      = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayGen_ShaderStageInfo.stage      = vk::ShaderStageFlagBits::eRaygenKHR;
		RayGen_ShaderStageInfo.module     = RayGen_ShaderModule;
		RayGen_ShaderStageInfo.pName      = "main";

		vk::PipelineShaderStageCreateInfo RayMiss_ShaderStageInfo{};
		RayMiss_ShaderStageInfo.sType     = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayMiss_ShaderStageInfo.stage     = vk::ShaderStageFlagBits::eMissKHR;
		RayMiss_ShaderStageInfo.module    = RayMiss_ShaderModule;
		RayMiss_ShaderStageInfo.pName     = "main";

	   std::vector<vk::PipelineShaderStageCreateInfo> ShaderStages = { RayGen_ShaderStageInfo ,
																	  RayMiss_ShaderStageInfo};

	   vk::RayTracingShaderGroupCreateInfoKHR RayGen_GroupInfo{};
	   RayGen_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
	   RayGen_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
	   RayGen_GroupInfo.generalShader = 0;
	   RayGen_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
	   RayGen_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
	   RayGen_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

	   vk::RayTracingShaderGroupCreateInfoKHR Miss_GroupInfo{};
	   Miss_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
	   Miss_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
	   Miss_GroupInfo.generalShader = 1;
	   Miss_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
	   Miss_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
	   Miss_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

	   vk::RayTracingShaderGroupCreateInfoKHR Hit_GroupInfo{};
	   Hit_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
	   Hit_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
	   Hit_GroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
	   Hit_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
	   Hit_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
	   Hit_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;



	   std::vector<vk::RayTracingShaderGroupCreateInfoKHR> ShaderGroups = {
		   RayGen_GroupInfo,
		   Miss_GroupInfo,
		   Hit_GroupInfo
	   };

	   vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	   pipelineLayoutInfo.setLayoutCount = 1;
	   pipelineLayoutInfo.pSetLayouts = &Raytracing_Shadows->RayTracingDescriptorSetLayout;
	   pipelineLayoutInfo.pushConstantRangeCount = 0;
	   pipelineLayoutInfo.pPushConstantRanges = nullptr;

	   RT_ShadowsPipelineLayout = vulkanContext->LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);
	   
	   RT_ShadowsPassPipeline = pipelineManager->createRayTracingGraphicsPipeline(RT_ShadowsPipelineLayout, ShaderStages, ShaderGroups);
	   
	   vulkanContext->LogicalDevice.destroyShaderModule(RayGen_ShaderModule);
	   vulkanContext->LogicalDevice.destroyShaderModule(RayMiss_ShaderModule);

	}


	//{
	//	auto RayGen_ShaderCode     = readFile("../Shaders/Compiled_Shader_Files/Reflection_Raygen.rgen.spv");
	//	auto Miss_ShaderCode       = readFile("../Shaders/Compiled_Shader_Files/Reflection_Miss.rmiss.spv"); 
	//	auto ClosestHit_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/Reflection_ClosestHit.rchit.spv");
	//
	//	VkShaderModule RayGen_ShaderModule     = pipelineManager->createShaderModule(RayGen_ShaderCode);
	//	VkShaderModule Miss_ShaderModule = pipelineManager->createShaderModule(Miss_ShaderCode);
	//	VkShaderModule ClosestHit_ShaderModule = pipelineManager->createShaderModule(ClosestHit_ShaderCode);
	//
	//
	//	vk::PipelineShaderStageCreateInfo RayGen_ShaderStageInfo{};
	//	RayGen_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
	//	RayGen_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eRaygenKHR;
	//	RayGen_ShaderStageInfo.module = RayGen_ShaderModule;
	//	RayGen_ShaderStageInfo.pName = "main";
	//
	//	vk::PipelineShaderStageCreateInfo Miss_ShaderStageInfo{};
	//	Miss_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
	//	Miss_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eMissKHR;
	//	Miss_ShaderStageInfo.module = Miss_ShaderModule;
	//	Miss_ShaderStageInfo.pName = "main";
	//
	//	vk::PipelineShaderStageCreateInfo ClosestHit_ShaderStageInfo{};
	//	ClosestHit_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
	//	ClosestHit_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eClosestHitKHR;
	//	ClosestHit_ShaderStageInfo.module = ClosestHit_ShaderModule;
	//	ClosestHit_ShaderStageInfo.pName = "main";
	//
	//
	//
	//
	//	std::vector<vk::PipelineShaderStageCreateInfo> ShaderStages = { RayGen_ShaderStageInfo ,
	//																	Miss_ShaderStageInfo,
	//																	ClosestHit_ShaderStageInfo };
	//
	//	vk::RayTracingShaderGroupCreateInfoKHR RayGen_GroupInfo{};
	//	RayGen_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
	//	RayGen_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
	//	RayGen_GroupInfo.generalShader = 0;
	//	RayGen_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
	//	RayGen_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
	//	RayGen_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
	//
	//	vk::RayTracingShaderGroupCreateInfoKHR Miss_GroupInfo{}; 
	//	Miss_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
	//	Miss_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
	//	Miss_GroupInfo.generalShader = 1;
	//	Miss_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
	//	Miss_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
	//	Miss_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
	//
	//	vk::RayTracingShaderGroupCreateInfoKHR Hit_GroupInfo{};
	//	Hit_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
	//	Hit_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
	//	Hit_GroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
	//	Hit_GroupInfo.closestHitShader = 2;
	//	Hit_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
	//	Hit_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
	//
	//
	//	std::vector<vk::RayTracingShaderGroupCreateInfoKHR> ShaderGroups = {
	//		RayGen_GroupInfo,
	//		Miss_GroupInfo,
	//		Hit_GroupInfo
	//	};
	//
	//	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	//	pipelineLayoutInfo.setLayoutCount = 1;
	//	pipelineLayoutInfo.pSetLayouts = &Raytracing_Reflections->RayTracingDescriptorSetLayout;
	//	pipelineLayoutInfo.pushConstantRangeCount = 0;
	//	pipelineLayoutInfo.pPushConstantRanges = nullptr;
	//
	//	RT_ReflectionPipelineLayout = vulkanContext->LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);
	//
	//	RT_ReflectionPipeline = pipelineManager->createRayTracingGraphicsPipeline(RT_ReflectionPipelineLayout, ShaderStages, ShaderGroups);
	//
	//	vulkanContext->LogicalDevice.destroyShaderModule(RayGen_ShaderModule);
	//	vulkanContext->LogicalDevice.destroyShaderModule(Miss_ShaderModule);
	//	vulkanContext->LogicalDevice.destroyShaderModule(ClosestHit_ShaderModule);
	//}


	{	
		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanContext->swapchainformat;

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(int));
		range.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(SSGI_FullScreenQuad->descriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;


		FullScreen_Quad_Pipeline_Data  Temp = pipelineManager->create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/SSGI.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);

		SSGIPipelineLayout = Temp.FQ_PipelineLayout;
		SSGIPipeline = Temp.FQ_Pipeline;

	}


	{

		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanContext->swapchainformat;

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(int));
		range.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(SSGI_FullScreenQuad->TemporalAccumilationDescriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;


		FullScreen_Quad_Pipeline_Data  Temp = pipelineManager->create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/TemporalAccumulation.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);

		TA_SSGIPipelineLayout = Temp.FQ_PipelineLayout;
		TA_SSGIPipeline = Temp.FQ_Pipeline;
	}


	{
		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanContext->swapchainformat;

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(glm::vec2));
		range.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(SSGI_FullScreenQuad->Blured_TemporalAccumilationDescriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;


		FullScreen_Quad_Pipeline_Data  Temp = pipelineManager->create_FQ_Pipeline("../Shaders/Compiled_Shader_Files/SSGI_Blur_Shader.frag.spv", pipelineRenderingCreateInfo, pipelineLayoutInfo);

		BluredSSGIPipelineLayout = Temp.FQ_PipelineLayout;
		BluredSSGIPipeline = Temp.FQ_Pipeline;
	}

}

uint32_t App::alignedSize(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

void App::createShaderBindingTable() {

	{
		const size_t   handleSize = vulkanContext->RayTracingPipelineProperties.shaderGroupHandleSize;
		const size_t   handleSizeAligned = alignedSize(handleSize, vulkanContext->RayTracingPipelineProperties.shaderGroupHandleAlignment);
		const uint32_t groupCount = 3;
		const uint32_t sbtSize = groupCount * handleSizeAligned;

		// Get shader group handles
		std::vector<uint8_t> shaderHandleStorage(sbtSize);

		vulkanContext->vkGetRayTracingShaderGroupHandlesKHR(
			static_cast<VkDevice>(vulkanContext->LogicalDevice),
			static_cast<VkPipeline>(RT_ShadowsPassPipeline),
			0,  // First group
			groupCount,
			shaderHandleStorage.size(),
			shaderHandleStorage.data());

		raygenShaderBindingTableBuffer.BufferID = "raygen Shader Binding Table Buffer";
		missShaderBindingTableBuffer.BufferID = "miss Shader Binding Table Buffer";
		hitShaderBindingTableBuffer.BufferID = "hit Shader Binding Table Buffer";

		bufferManger->CreateBuffer(&raygenShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext->graphicsQueue);
		bufferManger->CreateBuffer(&missShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext->graphicsQueue);
		bufferManger->CreateBuffer(&hitShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext->graphicsQueue);

		bufferManger->CopyDataToBuffer(shaderHandleStorage.data(), raygenShaderBindingTableBuffer);
		bufferManger->CopyDataToBuffer(shaderHandleStorage.data() + handleSizeAligned, missShaderBindingTableBuffer);
		bufferManger->CopyDataToBuffer(shaderHandleStorage.data() + handleSizeAligned * 2, hitShaderBindingTableBuffer);
	}



	//{
	//	const size_t   handleSize = vulkanContext->RayTracingPipelineProperties.shaderGroupHandleSize;
	//	const size_t   handleSizeAligned = alignedSize(handleSize, vulkanContext->RayTracingPipelineProperties.shaderGroupHandleAlignment);
	//	const uint32_t groupCount = 3;
	//	const uint32_t sbtSize = groupCount * handleSizeAligned;
	//
	//	// Get shader group handles
	//	std::vector<uint8_t> shaderHandleStorage(sbtSize);
	//
	//	vulkanContext->vkGetRayTracingShaderGroupHandlesKHR(
	//		static_cast<VkDevice>(vulkanContext->LogicalDevice),
	//		static_cast<VkPipeline>(RT_ReflectionPipeline),
	//		0,  // First group
	//		groupCount,
	//		shaderHandleStorage.size(),
	//		shaderHandleStorage.data());
	//
	//	Reflection_raygenShaderBindingTableBuffer.BufferID = "raygen Shader Binding Table Buffer";
	//	Reflection_missShaderBindingTableBuffer.BufferID = "miss Shader Binding Table Buffer";
	//	Reflection_hitShaderBindingTableBuffer.BufferID = "hit Shader Binding Table Buffer";
	//
	//	bufferManger->CreateBuffer(&Reflection_raygenShaderBindingTableBuffer, handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext->graphicsQueue);
	//	bufferManger->CreateBuffer(&Reflection_missShaderBindingTableBuffer,   handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext->graphicsQueue);
	//	bufferManger->CreateBuffer(&Reflection_hitShaderBindingTableBuffer,    handleSizeAligned, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext->graphicsQueue);
	//
	//	bufferManger->CopyDataToBuffer(shaderHandleStorage.data(), Reflection_raygenShaderBindingTableBuffer);
	//	bufferManger->CopyDataToBuffer(shaderHandleStorage.data() + handleSizeAligned, Reflection_missShaderBindingTableBuffer);
	//	bufferManger->CopyDataToBuffer(shaderHandleStorage.data() + handleSizeAligned * 2, Reflection_hitShaderBindingTableBuffer);
	//}


}

void App::DestroyShaderBindingTable() {

	bufferManger->DestroyBuffer(raygenShaderBindingTableBuffer);
	bufferManger->DestroyBuffer(missShaderBindingTableBuffer);
	bufferManger->DestroyBuffer(hitShaderBindingTableBuffer);

	//bufferManger->DestroyBuffer(Reflection_raygenShaderBindingTableBuffer);
	//bufferManger->DestroyBuffer(Reflection_missShaderBindingTableBuffer);
	//bufferManger->DestroyBuffer(Reflection_hitShaderBindingTableBuffer);

}




void App::createCommandPool()
{ 
	vk::CommandPoolCreateInfo poolInfo{};
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	poolInfo.queueFamilyIndex = vulkanContext->graphicsQueueFamilyIndex;

	commandPool = vulkanContext->LogicalDevice.createCommandPool(poolInfo);

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

	commandBuffers = vulkanContext->LogicalDevice.allocateCommandBuffers(allocateInfo);

	if (commandBuffers.empty())
	{
		throw std::runtime_error("failed to create command Buffer!");

	}


}
void App::createSyncObjects() {
	// Present complete semaphores - one per swapchain image
	presentCompleteSemaphores.resize(vulkanContext->swapchainImageData.size());

	// Render complete semaphores 
	renderCompleteSemaphores.resize(vulkanContext->swapchainImageData.size());

	// Fences - one per frame in flight
	waitFences.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < vulkanContext->swapchainImageData.size(); i++) {
		vk::SemaphoreCreateInfo semaphoreInfo{};
		vulkanContext->LogicalDevice.createSemaphore(&semaphoreInfo, nullptr, &presentCompleteSemaphores[i]);
		vulkanContext->LogicalDevice.createSemaphore(&semaphoreInfo, nullptr, &renderCompleteSemaphores[i]);
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::FenceCreateInfo fenceInfo{};
		fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
		vulkanContext->LogicalDevice.createFence(&fenceInfo, nullptr, &waitFences[i]);
	}
}

void App::Run()
{
	FramesPerSecondCounter fpsCounter(0.1f);

	while (!window->shouldClose())
	{
		//FrameMarkNamed("main"); 
		glfwPollEvents();
		CalculateFps(fpsCounter);
		camera->Update(deltaTime);

		userinterface->DrawUi(this,skyBox.get());
		Draw();		

	}

	vulkanContext->LogicalDevice.waitIdle();
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
	vulkanContext->LogicalDevice.waitForFences(1, &waitFences[currentFrame], vk::True, UINT64_MAX);
	vulkanContext->LogicalDevice.resetFences(1, &waitFences[currentFrame]);

	// --- Swapchain Acquisition ---
	uint32_t imageIndex;
	try {
		// Request the next available swapchain image.
		// presentCompleteSemaphores[currentFrame] will be signaled when the image is ready.
		vulkanContext->LogicalDevice.acquireNextImageKHR(
			vulkanContext->swapChain,
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
		vk::PipelineStageFlagBits::eColorAttachmentOutput  // Where we'll wait
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
	if (vulkanContext->graphicsQueue.submit(1, &submitInfo, waitFences[currentFrame]) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to submit draw commands");
	}

	// --- Presentation ---
	vk::SwapchainKHR swapChains[] = { vulkanContext->swapChain };

	vk::PresentInfoKHR presentInfo{};
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores    =  submitSemaphores;  // Wait for rendering completion
	presentInfo.swapchainCount     = 1;
	presentInfo.pSwapchains        = swapChains;
	presentInfo.pImageIndices      = &imageIndex;

	try {
		// Present the image - will wait on renderCompleteSemaphores[imageIndex]
		vk::Result result = vulkanContext->presentQueue.presentKHR(presentInfo);

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

	lighting_FullScreenQuad->UpdateUniformBuffer(currentImage, lights);
	ssao_FullScreenQuad->UpdataeUniformBufferData();
	Raytracing_Shadows->UpdateUniformBuffer(currentImage, lights);
    SSGI_FullScreenQuad->UpdateUniformBuffer(currentImage, lights,deltaTime);

}

void  App::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) {


	vk::CommandBufferBeginInfo begininfo{};
	begininfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	commandBuffer.begin(begininfo);

	vk::ClearValue clearColor{};
	clearColor.color = { 0.0f, 0.0f, 0.0f, 0.0f };

	VkOffset2D imageoffset = { 0, 0 };

	vk::Extent3D swapchainextent = vk::Extent3D(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, 1);

	vk::Viewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width =  vulkanContext->swapchainExtent.width;
	viewport.height = vulkanContext->swapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor{};
	scissor.offset = imageoffset;
	scissor.extent.width =  vulkanContext->swapchainExtent.width;
	scissor.extent.height = vulkanContext->swapchainExtent.height;


	vk::DeviceSize offsets[] = { 0 };

	 /////////////////// GBUFFER PASS ///////////////////////// 
	{
		ImageTransitionData TransitionToGeneral{};
		TransitionToGeneral.oldlayout = vk::ImageLayout::eUndefined;
		TransitionToGeneral.newlayout = vk::ImageLayout::eGeneral;
		TransitionToGeneral.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionToGeneral.SourceAccessflag = vk::AccessFlagBits::eNone;
		TransitionToGeneral.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eShaderRead;
		TransitionToGeneral.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
		TransitionToGeneral.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eFragmentShader;

		bufferManger->TransitionImage(commandBuffer, &SSGI_FullScreenQuad->HalfRes_SSGIPassLastFrameImage, TransitionToGeneral);
		bufferManger->TransitionImage(commandBuffer, &SSGI_FullScreenQuad->HalfRes_SSGIAccumilationImage, TransitionToGeneral);

	


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

		vk::RenderingAttachmentInfo MaterialscolorAttachmentInfo{};
		MaterialscolorAttachmentInfo.imageView = gbuffer.Materials.imageView;
		MaterialscolorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		MaterialscolorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		MaterialscolorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		MaterialscolorAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfo ReflectionMaskcolorAttachmentInfo{};
		ReflectionMaskcolorAttachmentInfo.imageView = ReflectionMaskImageData.imageView;
		ReflectionMaskcolorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		ReflectionMaskcolorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		ReflectionMaskcolorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		ReflectionMaskcolorAttachmentInfo.clearValue = clearColor;

		std::array<vk::RenderingAttachmentInfo, 7> ColorAttachments{ PositioncolorAttachmentInfo,ViewSpacePositioncolorAttachmentInfo,
			                                                         NormalcolorAttachmentInfo, ViewSpaceNormalcolorAttachmentInfo,
			                                                         AlbedocolorAttachmentInfo,MaterialscolorAttachmentInfo,ReflectionMaskcolorAttachmentInfo };

		vk::RenderingAttachmentInfo depthStencilAttachment;
		depthStencilAttachment.imageView = DepthTextureData.imageView;
		depthStencilAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		depthStencilAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		depthStencilAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		depthStencilAttachment.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

		vk::RenderingInfo renderingInfo{};
		renderingInfo.renderArea.offset = imageoffset;
		renderingInfo.renderArea.extent.height = vulkanContext->swapchainExtent.height;
		renderingInfo.renderArea.extent.width = vulkanContext->swapchainExtent.width;
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
			vulkanContext->vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_LINE);
		}
		else
		{
			vulkanContext->vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_FILL);
		}

		for (auto& model : Models)
		{
			model->Draw(commandBuffer, geometryPassPipelineLayout, currentFrame);
		}

		commandBuffer.endRendering();

	}
	/////////////////// GBUFFER PASS END ///////////////////////// 

	vulkanContext->vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_FILL);

	{
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
		ssao_FullScreenQuad->DrawSSAOBlurVertical(commandBuffer, SSAOBlurPipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}

	{
		ImageTransitionData TransitiontoGeneralRT{};
		TransitiontoGeneralRT.oldlayout = vk::ImageLayout::eUndefined;
		TransitiontoGeneralRT.newlayout = vk::ImageLayout::eGeneral;
		TransitiontoGeneralRT.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitiontoGeneralRT.SourceAccessflag = vk::AccessFlagBits::eNone;
		TransitiontoGeneralRT.DestinationAccessflag = vk::AccessFlagBits::eShaderWrite;
		TransitiontoGeneralRT.SourceOnThePipeline = vk::PipelineStageFlagBits::eNone;
		TransitiontoGeneralRT.DestinationOnThePipeline = vk::PipelineStageFlagBits::eRayTracingShaderKHR;

		for (int i = 0; i < Raytracing_Shadows->ShadowPassImages.size(); i++)
		{
			bufferManger->TransitionImage(commandBuffer, &Raytracing_Shadows->ShadowPassImages[i], TransitiontoGeneralRT);

		}


		commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, RT_ShadowsPassPipeline);
		
		Raytracing_Shadows->Draw(
			raygenShaderBindingTableBuffer,
			hitShaderBindingTableBuffer,
			missShaderBindingTableBuffer,
			commandBuffer,
			RT_ShadowsPipelineLayout,
			currentFrame);
	}


	//{
	//
	//	commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, RT_ReflectionPipeline);
	//
	//	Raytracing_Reflections->Draw(
	//		Reflection_raygenShaderBindingTableBuffer,
	//		Reflection_hitShaderBindingTableBuffer,
	//		Reflection_missShaderBindingTableBuffer,
	//		commandBuffer,
	//		RT_ReflectionPipelineLayout,
	//		currentFrame);
	//}


    /////////////////// LIGHTING PASS ///////////////////////// 
	{
		vk::RenderingAttachmentInfo LightPassColorAttachmentInfo{};
		LightPassColorAttachmentInfo.imageView = LightingPassImageData.imageView;
		LightPassColorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		LightPassColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		LightPassColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		LightPassColorAttachmentInfo.clearValue = clearColor;

		vk::RenderingInfo renderingInfo{};
		renderingInfo.renderArea.offset = imageoffset;
		renderingInfo.renderArea.extent.height = vulkanContext->swapchainExtent.height; 
		renderingInfo.renderArea.extent.width =  vulkanContext->swapchainExtent.width;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &LightPassColorAttachmentInfo;

		commandBuffer.setViewport(0, 1, &viewport);
		commandBuffer.setScissor(0, 1, &scissor);
		commandBuffer.beginRendering(renderingInfo);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, DeferedLightingPassPipeline);
		lighting_FullScreenQuad->Draw(commandBuffer, DeferedLightingPassPipelineLayout, currentFrame);

		commandBuffer.endRendering();
	}
	 /////////////////// LIGHTING PASS END ///////////////////////// 

	{	
		ImageTransitionData TransitionDepthtTOShaderOptimal{};
		TransitionDepthtTOShaderOptimal.oldlayout = vk::ImageLayout::eDepthAttachmentOptimal;
		TransitionDepthtTOShaderOptimal.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionDepthtTOShaderOptimal.AspectFlag = vk::ImageAspectFlagBits::eDepth;
		TransitionDepthtTOShaderOptimal.SourceAccessflag = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;
		TransitionDepthtTOShaderOptimal.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
		TransitionDepthtTOShaderOptimal.SourceOnThePipeline = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		TransitionDepthtTOShaderOptimal.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;
		bufferManger->TransitionImage(commandBuffer, &DepthTextureData, TransitionDepthtTOShaderOptimal);

	
		vk::RenderingAttachmentInfo SSRRenderAttachInfo;
		SSRRenderAttachInfo.clearValue = clearColor;
		SSRRenderAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		SSRRenderAttachInfo.imageView = LightingPassImageData.imageView;
		SSRRenderAttachInfo.loadOp = vk::AttachmentLoadOp::eLoad;
		SSRRenderAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;
	
		vk::RenderingInfo SSRRenderInfo{};
		SSRRenderInfo.layerCount = 1;
		SSRRenderInfo.colorAttachmentCount = 1;
		SSRRenderInfo.pColorAttachments = &SSRRenderAttachInfo;
		SSRRenderInfo.renderArea.extent.width = vulkanContext->swapchainExtent.width;
		SSRRenderInfo.renderArea.extent.height = vulkanContext->swapchainExtent.height;

		commandBuffer.setViewport(0, 1, &viewport);
		commandBuffer.setScissor(0, 1, &scissor);
		commandBuffer.beginRendering(SSRRenderInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, SSRPipeline);
		ssr_FullScreenQuad->Draw(commandBuffer, SSRPipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}

	{
		vk::RenderingAttachmentInfo SkyBoxRenderAttachInfo;
		SkyBoxRenderAttachInfo.clearValue = clearColor;
		SkyBoxRenderAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		SkyBoxRenderAttachInfo.imageView = LightingPassImageData.imageView;
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
		SkyBoxRenderInfo.renderArea.extent.width = vulkanContext->swapchainExtent.width;
		SkyBoxRenderInfo.renderArea.extent.height = vulkanContext->swapchainExtent.height;


		commandBuffer.setViewport(0, 1, &viewport);
		commandBuffer.setScissor(0, 1, &scissor);
		commandBuffer.beginRendering(SkyBoxRenderInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, SkyBoxgraphicsPipeline);
		skyBox->Draw(commandBuffer, SkyBoxpipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}

	{
		vk::RenderingAttachmentInfo SSGIImageAttachInfo;
		SSGIImageAttachInfo.clearValue = clearColor;
		SSGIImageAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		SSGIImageAttachInfo.imageView = SSGI_FullScreenQuad->HalfRes_SSGIPassImage.imageView;
		SSGIImageAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
		SSGIImageAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingInfo SSGIImageImageInfo{};
		SSGIImageImageInfo.layerCount = 1;
		SSGIImageImageInfo.colorAttachmentCount = 1;
		SSGIImageImageInfo.pColorAttachments = &SSGIImageAttachInfo;
		SSGIImageImageInfo.renderArea.extent.width  = SSGI_FullScreenQuad->Swapchainextent_Half_Res.width  ;
		SSGIImageImageInfo.renderArea.extent.height = SSGI_FullScreenQuad->Swapchainextent_Half_Res.height ;


		vk::Viewport viewport50{};
		viewport50.x = 0.0f;
		viewport50.y = 0.0f;
		viewport50.width  = SSGI_FullScreenQuad->Swapchainextent_Half_Res.width ;
		viewport50.height = SSGI_FullScreenQuad->Swapchainextent_Half_Res.height ;
		viewport50.minDepth = 0.0f;
		viewport50.maxDepth = 1.0f;

		vk::Rect2D scissor50{};
		scissor50.offset = imageoffset;
		scissor50.extent.width  = SSGI_FullScreenQuad->Swapchainextent_Half_Res.width ;
		scissor50.extent.height = SSGI_FullScreenQuad->Swapchainextent_Half_Res.height ;

		commandBuffer.setViewport(0, 1, &viewport50);
		commandBuffer.setScissor(0, 1, &scissor50);
		commandBuffer.beginRendering(SSGIImageImageInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, SSGIPipeline);
		SSGI_FullScreenQuad->Draw(commandBuffer, SSGIPipelineLayout, currentFrame);
		commandBuffer.endRendering();

		ImageTransitionData TransitionDeptTODepthOptimal{};
		TransitionDeptTODepthOptimal.oldlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionDeptTODepthOptimal.newlayout = vk::ImageLayout::eDepthAttachmentOptimal;
		TransitionDeptTODepthOptimal.AspectFlag = vk::ImageAspectFlagBits::eDepth;
		TransitionDeptTODepthOptimal.SourceAccessflag = vk::AccessFlagBits::eShaderRead;
		TransitionDeptTODepthOptimal.DestinationAccessflag = vk::AccessFlagBits::eDepthStencilAttachmentWrite |vk::AccessFlagBits::eDepthStencilAttachmentRead;
		TransitionDeptTODepthOptimal.SourceOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;
		TransitionDeptTODepthOptimal.DestinationOnThePipeline = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
		bufferManger->TransitionImage(commandBuffer, &DepthTextureData, TransitionDeptTODepthOptimal);

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

		bufferManger->TransitionImage(commandBuffer, &SSGI_FullScreenQuad->HalfRes_SSGIAccumilationImage, TransitionTOSrc);

		ImageTransitionData TransitionTODst{};
		TransitionTODst.oldlayout = vk::ImageLayout::eGeneral;
		TransitionTODst.newlayout = vk::ImageLayout::eTransferDstOptimal;
		TransitionTODst.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionTODst.SourceAccessflag = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eShaderRead;
		TransitionTODst.DestinationAccessflag = vk::AccessFlagBits::eTransferWrite;
		TransitionTODst.SourceOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eFragmentShader;
		TransitionTODst.DestinationOnThePipeline = vk::PipelineStageFlagBits::eTransfer;


		bufferManger->TransitionImage(commandBuffer, &SSGI_FullScreenQuad->HalfRes_SSGIPassLastFrameImage, TransitionTODst);


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
			SSGI_FullScreenQuad->Swapchainextent_Half_Res.width ,
			SSGI_FullScreenQuad->Swapchainextent_Half_Res.height,
			1
		};

		bufferManger->CopyImageToAnotherImage(commandBuffer,
			                                  SSGI_FullScreenQuad->HalfRes_SSGIAccumilationImage,  vk::ImageLayout::eTransferSrcOptimal, SrcSubresourceLayers,
			                                  SSGI_FullScreenQuad->HalfRes_SSGIPassLastFrameImage, vk::ImageLayout::eTransferDstOptimal, DstSubresourceLayers,
			                                  swapchainExtenthalf, vulkanContext->graphicsQueue);






		ImageTransitionData TransitionSrcBack{};
		TransitionSrcBack.oldlayout = vk::ImageLayout::eTransferSrcOptimal;
		TransitionSrcBack.newlayout = vk::ImageLayout::eGeneral;
		TransitionSrcBack.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionSrcBack.SourceAccessflag = vk::AccessFlagBits::eTransferRead; 
		TransitionSrcBack.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eShaderRead;
		TransitionSrcBack.SourceOnThePipeline = vk::PipelineStageFlagBits::eTransfer;
		TransitionSrcBack.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eFragmentShader;

		bufferManger->TransitionImage(commandBuffer, &SSGI_FullScreenQuad->HalfRes_SSGIAccumilationImage, TransitionSrcBack);


		ImageTransitionData TransitionDstToSample{};
		TransitionDstToSample.oldlayout = vk::ImageLayout::eTransferDstOptimal;
		TransitionDstToSample.newlayout = vk::ImageLayout::eGeneral;
		TransitionDstToSample.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionDstToSample.SourceAccessflag = vk::AccessFlagBits::eTransferWrite;
		TransitionDstToSample.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eShaderRead;
		TransitionDstToSample.SourceOnThePipeline = vk::PipelineStageFlagBits::eTransfer;
		TransitionDstToSample.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eFragmentShader;

		bufferManger->TransitionImage(commandBuffer, &SSGI_FullScreenQuad->HalfRes_SSGIPassLastFrameImage, TransitionDstToSample);

	}

	{
		vk::RenderingAttachmentInfo TA_ImageAttachInfo;
		TA_ImageAttachInfo.clearValue = clearColor;
		TA_ImageAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		TA_ImageAttachInfo.imageView = SSGI_FullScreenQuad->HalfRes_SSGIAccumilationImage.imageView;
		TA_ImageAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
		TA_ImageAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingInfo SSGIImageInfo{};
		SSGIImageInfo.layerCount = 1;
		SSGIImageInfo.colorAttachmentCount = 1;
		SSGIImageInfo.pColorAttachments = &TA_ImageAttachInfo;
		SSGIImageInfo.renderArea.extent.width  = SSGI_FullScreenQuad->Swapchainextent_Half_Res.width ;
		SSGIImageInfo.renderArea.extent.height = SSGI_FullScreenQuad->Swapchainextent_Half_Res.height ;

		vk::Viewport viewport50{};
		viewport50.x = 0.0f;
		viewport50.y = 0.0f;
		viewport50.width  = SSGI_FullScreenQuad->Swapchainextent_Half_Res.width ;
		viewport50.height = SSGI_FullScreenQuad->Swapchainextent_Half_Res.height ;
		viewport50.minDepth = 0.0f;
		viewport50.maxDepth = 1.0f;

		vk::Rect2D scissor50{};
		scissor50.offset = imageoffset;
		scissor50.extent.width  = SSGI_FullScreenQuad->Swapchainextent_Half_Res.width ;
		scissor50.extent.height = SSGI_FullScreenQuad->Swapchainextent_Half_Res.height ;

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
		Blured_TA_ImageAttachInfo.imageView = SSGI_FullScreenQuad->HalfRes_HorizontalBluredSSGIAccumilationImage.imageView;
		Blured_TA_ImageAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
		Blured_TA_ImageAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingInfo BluredSSGIImageInfo{};
		BluredSSGIImageInfo.layerCount = 1;
		BluredSSGIImageInfo.colorAttachmentCount = 1;
		BluredSSGIImageInfo.pColorAttachments = &Blured_TA_ImageAttachInfo;
		BluredSSGIImageInfo.renderArea.extent.width = SSGI_FullScreenQuad->Swapchainextent_Half_Res.width;
		BluredSSGIImageInfo.renderArea.extent.height = SSGI_FullScreenQuad->Swapchainextent_Half_Res.height;

		vk::Viewport viewport50{};
		viewport50.x = 0.0f;
		viewport50.y = 0.0f;
		viewport50.width = SSGI_FullScreenQuad->Swapchainextent_Half_Res.width;
		viewport50.height = SSGI_FullScreenQuad->Swapchainextent_Half_Res.height;
		viewport50.minDepth = 0.0f;
		viewport50.maxDepth = 1.0f;

		vk::Rect2D scissor50{};
		scissor50.offset = imageoffset;
		scissor50.extent.width = SSGI_FullScreenQuad->Swapchainextent_Half_Res.width;
		scissor50.extent.height = SSGI_FullScreenQuad->Swapchainextent_Half_Res.height;

		commandBuffer.setViewport(0, 1, &viewport50);
		commandBuffer.setScissor(0, 1, &scissor50);
		commandBuffer.beginRendering(BluredSSGIImageInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, BluredSSGIPipeline);


		SSGI_FullScreenQuad->DrawTA_HorizontalBlur(commandBuffer, BluredSSGIPipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}


	{
		vk::RenderingAttachmentInfo Blured_TA_ImageAttachInfo;
		Blured_TA_ImageAttachInfo.clearValue = clearColor;
		Blured_TA_ImageAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		Blured_TA_ImageAttachInfo.imageView = SSGI_FullScreenQuad->HalfRes_BluredSSGIAccumilationImage.imageView;
		Blured_TA_ImageAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
		Blured_TA_ImageAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingInfo BluredSSGIImageInfo{};
		BluredSSGIImageInfo.layerCount = 1;
		BluredSSGIImageInfo.colorAttachmentCount = 1;
		BluredSSGIImageInfo.pColorAttachments = &Blured_TA_ImageAttachInfo;
		BluredSSGIImageInfo.renderArea.extent.width = SSGI_FullScreenQuad->Swapchainextent_Half_Res.width;
		BluredSSGIImageInfo.renderArea.extent.height = SSGI_FullScreenQuad->Swapchainextent_Half_Res.height;

		vk::Viewport viewport50{};
		viewport50.x = 0.0f;
		viewport50.y = 0.0f;
		viewport50.width = SSGI_FullScreenQuad->Swapchainextent_Half_Res.width;
		viewport50.height = SSGI_FullScreenQuad->Swapchainextent_Half_Res.height;
		viewport50.minDepth = 0.0f;
		viewport50.maxDepth = 1.0f;

		vk::Rect2D scissor50{};
		scissor50.offset = imageoffset;
		scissor50.extent.width = SSGI_FullScreenQuad->Swapchainextent_Half_Res.width;
		scissor50.extent.height = SSGI_FullScreenQuad->Swapchainextent_Half_Res.height;

		commandBuffer.setViewport(0, 1, &viewport50);
		commandBuffer.setScissor(0, 1, &scissor50);
		commandBuffer.beginRendering(BluredSSGIImageInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, BluredSSGIPipeline);


		SSGI_FullScreenQuad->DrawTA_VerticalBlur(commandBuffer, BluredSSGIPipelineLayout, currentFrame);
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
		CombinedImageInfo.renderArea.extent.width = vulkanContext->swapchainExtent.width;
		CombinedImageInfo.renderArea.extent.height = vulkanContext->swapchainExtent.height;

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
		renderingInfo.renderArea.extent.height = vulkanContext->swapchainExtent.height;
		renderingInfo.renderArea.extent.width = vulkanContext->swapchainExtent.width;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &LightPassColorAttachmentInfo;
		renderingInfo.pDepthAttachment = &depthStencilAttachment;

		if (bWireFrame)
		{
			vulkanContext->vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_LINE);
		}
		else
		{
			vulkanContext->vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_FILL);
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


	/////////////////// FORWARD PASS END ///////////////////////// 
	vulkanContext->vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_FILL);

	{
		vk::RenderingAttachmentInfo LightPassColorAttachmentInfo{};
		LightPassColorAttachmentInfo.imageView = fxaa_FullScreenQuad->FxaaImage.imageView;
		LightPassColorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		LightPassColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		LightPassColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		LightPassColorAttachmentInfo.clearValue = clearColor;

		vk::RenderingInfo renderingInfo{};
		renderingInfo.renderArea.offset = imageoffset;
		renderingInfo.renderArea.extent.height = vulkanContext->swapchainExtent.height;
		renderingInfo.renderArea.extent.width = vulkanContext->swapchainExtent.width;
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


	userinterface->RenderUi(commandBuffer, imageIndex);

}

void App::destroy_DepthImage()
{
	bufferManger->DestroyImage(DepthTextureData);
}

void App::destroy_GbufferImages()
{
	bufferManger->DestroyImage(gbuffer.Position);
	bufferManger->DestroyImage(gbuffer.ViewSpacePosition);
	bufferManger->DestroyImage(gbuffer.Normal);
	bufferManger->DestroyImage(gbuffer.ViewSpaceNormal);
	bufferManger->DestroyImage(gbuffer.Materials);
	bufferManger->DestroyImage(gbuffer.Albedo);
	bufferManger->DestroyImage(LightingPassImageData);
	bufferManger->DestroyImage(ReflectionMaskImageData);

	ssao_FullScreenQuad->DestroyImage();
	Raytracing_Shadows->DestroyStorageImage();
	fxaa_FullScreenQuad->DestroyImage();
	ssr_FullScreenQuad->DestroyImage();
	SSGI_FullScreenQuad->DestroyImage();
	Combined_FullScreenQuad->DestroyImage();
	//Raytracing_Reflections->DestroyStorageImage();
}

void App::recreateSwapChain() {
	
	int width = 0, height = 0;
	glfwGetFramebufferSize(window->GetWindow(), &width, &height);

	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window->GetWindow(), &width, &height);
		glfwWaitEvents();
	}

	vulkanContext->LogicalDevice.waitIdle();

	vulkanContext->destroy_swapchain();
	destroy_DepthImage();
	destroy_GbufferImages();

	vulkanContext->LogicalDevice.waitIdle();

	vulkanContext->create_swapchain();

	camera->SetSwapChainHeight(vulkanContext->swapchainExtent.height);
	camera->SetSwapChainWidth(vulkanContext->swapchainExtent.width);
	createDepthTextureImage();
	createGBuffer();

}

void App::recreatePipeline()
{
	vulkanContext->LogicalDevice.waitIdle();
	destroyPipeline();

	CreateGraphicsPipeline();
}


void App::DestroySyncObjects()
{
	for (auto& presentSemaphores : presentCompleteSemaphores)
	{
		vulkanContext->LogicalDevice.destroySemaphore(presentSemaphores);
	}

	for (auto& renderSemaphores : renderCompleteSemaphores)
	{
		vulkanContext->LogicalDevice.destroySemaphore(renderSemaphores);
	}

	for (auto& Fences : waitFences)
	{
		vulkanContext->LogicalDevice.destroyFence(Fences);
	}
}

void App::DestroyBuffers()
{

	destroy_DepthImage();
	destroy_GbufferImages();

	for (auto& model : Models)
	{
		model.reset();
	}

	for (auto& light : lights)
	{
		light.reset();
	}

	skyBox.reset();

	lighting_FullScreenQuad.reset();
	ssao_FullScreenQuad.reset();
	fxaa_FullScreenQuad.reset();
	ssr_FullScreenQuad.reset();
	Raytracing_Shadows.reset();
	SSGI_FullScreenQuad.reset();
	Combined_FullScreenQuad.reset();

	bufferManger->DestroyBuffer(TLAS_Buffer);
	bufferManger->DestroyBuffer(TLAS_SCRATCH_Buffer);
	bufferManger->DestroyBuffer(TLAS_InstanceData);
	vulkanContext->vkDestroyAccelerationStructureKHR(vulkanContext->LogicalDevice, static_cast<VkAccelerationStructureKHR>(TLAS),nullptr);
	DestroyShaderBindingTable();
	bufferManger.reset();
}

void App::destroyPipeline()
{
	vulkanContext->LogicalDevice.destroyPipeline(DeferedLightingPassPipeline);
	vulkanContext->LogicalDevice.destroyPipeline(FXAAPassPipeline);
	vulkanContext->LogicalDevice.destroyPipeline(LightgraphicsPipeline);
	vulkanContext->LogicalDevice.destroyPipeline(SkyBoxgraphicsPipeline);
	vulkanContext->LogicalDevice.destroyPipeline(geometryPassPipeline);
	vulkanContext->LogicalDevice.destroyPipeline(SSAOPipeline);
	vulkanContext->LogicalDevice.destroyPipeline(SSAOBlurPipeline);
	vulkanContext->LogicalDevice.destroyPipeline(SSRPipeline);
	vulkanContext->LogicalDevice.destroyPipeline(RT_ShadowsPassPipeline);
	//vulkanContext->LogicalDevice.destroyPipeline(RT_ReflectionPipeline);
	vulkanContext->LogicalDevice.destroyPipeline(SSGIPipeline);
	vulkanContext->LogicalDevice.destroyPipeline(BluredSSGIPipeline);
	vulkanContext->LogicalDevice.destroyPipeline(TA_SSGIPipeline);
	vulkanContext->LogicalDevice.destroyPipeline(CombinedImagePassPipeline);

	vulkanContext->LogicalDevice.destroyPipelineLayout(DeferedLightingPassPipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(FXAAPassPipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(LightpipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(SkyBoxpipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(geometryPassPipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(SSAOPipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(SSAOBlurPipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(SSRPipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(RT_ShadowsPipelineLayout);
	//vulkanContext->LogicalDevice.destroyPipelineLayout(RT_ReflectionPipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(SSGIPipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(TA_SSGIPipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(BluredSSGIPipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(CombinedImagePipelineLayout);

}


 App::~App()
{

	userinterface.reset();
	DestroyBuffers();

	if (!commandBuffers.empty()) {
		vulkanContext->LogicalDevice.freeCommandBuffers(commandPool, commandBuffers);
		commandBuffers.clear();
	}
	vulkanContext->LogicalDevice.destroyDescriptorPool(DescriptorPool);
	vulkanContext->LogicalDevice.destroyCommandPool(commandPool);

	destroyPipeline();

	DestroySyncObjects();
	vkb::destroy_debug_utils_messenger(vulkanContext->VulkanInstance, vulkanContext->Debug_Messenger);
	vulkanContext.reset();

	window.reset();
#ifndef NDEBUG
	_CrtDumpMemoryLeaks();  
#endif

}

void App::SwapchainResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}

