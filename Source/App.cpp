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
#include "Grass.h"
#include "RayTracing.h"
#include "CombinedResult_FullScreenQuad.h"
#include "SSGI.h"

#include <crtdbg.h>

#define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)

 App::App()
{
	window        = std::shared_ptr<Window>(new Window(1920, 1080, "Impulsion Renderer"), WindowDeleter);
	vulkanContext = std::shared_ptr<VulkanContext>(new VulkanContext(*window), VulkanContextDeleter);
	bufferManger  = std::shared_ptr<BufferManager>(new BufferManager (vulkanContext->LogicalDevice, vulkanContext->PhysicalDevice, vulkanContext->VulkanInstance), BufferManagerDeleter);
  	camera        = std::shared_ptr<Camera>(new Camera (vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, window->GetWindow()));
	userinterface = std::shared_ptr<UserInterface>(new UserInterface(vulkanContext.get(), window.get(), bufferManger.get()),UserInterfaceDeleter);
	//glfwSetFramebufferSizeCallback(window->GetWindow(),);
	glfwSetWindowUserPointer(window->GetWindow(), this);

	createSyncObjects();	
	//////////////////////////
	createCommandPool();



	std::array<const char*, 6> filePaths{
		"../Textures/Skybox/px.png",  // +X (Right)
		"../Textures/Skybox/nx.png",  // -X (Left)
		"../Textures/Skybox/py.png",  // +Y (Top)
		"../Textures/Skybox/ny.png",  // -Y (Bottom)
		"../Textures/Skybox/pz.png",  // +Z (Front)
		"../Textures/Skybox/nz.png"   // -Z (Back)
	};

	skyBox = std::shared_ptr<SkyBox>(new SkyBox(filePaths, vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), SkyBoxDeleter);


	auto model  = std::shared_ptr<Model>(new Model("../Textures/Helmet/DamagedHelmet.gltf"   , vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), ModelDeleter);
	auto model2 = std::shared_ptr<Model>(new Model("../Textures/WaterBottle/WaterBottle.gltf", vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), ModelDeleter);
	auto model3 = std::shared_ptr<Model>(new Model("../Textures/ScifiHelmet/SciFiHelmet.gltf", vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), ModelDeleter);
	auto model4 = std::shared_ptr<Model>(new Model("../Textures/Wall/Cube.gltf"            , vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), ModelDeleter);
	auto model5 = std::shared_ptr<Model>(new Model("../Textures/Wall2/Cube.gltf", vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), ModelDeleter);
	auto model6 = std::shared_ptr<Model>(new Model("../Textures/Wall3/Cube.gltf", vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), ModelDeleter);
	auto model7 = std::shared_ptr<Model>(new Model("../Textures/Wall4/Cube.gltf", vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), ModelDeleter);
	auto model8 = std::shared_ptr<Model>(new Model("../Textures/Dragon/scene.gltf", vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), ModelDeleter);

	model.get()->CubeMapReflectiveSwitch(true);
	model2.get()->CubeMapReflectiveSwitch(true);
	model3.get()->CubeMapReflectiveSwitch(true);

	model.get()->SetPosition(glm::vec3(5.000, -1.202, 5.798));
	model.get()->SetScale(glm::vec3(1.100, 1.000, 1.050));

	model2.get()->SetPosition(glm::vec3(-5.329, 0.265, -4.715));
	model2.get()->SetScale(glm::vec3(8.000, 8.000, 8.000));
	model2.get()->SetRotation(glm::vec3(0.000, 0.000, 0.00));


	model3.get()->SetPosition(glm::vec3(-4.507, -0.488, 6.017));
	model3.get()->SetScale(glm::vec3(1.100, 1.000, 1.050));
	model3.get()->SetRotation(glm::vec3(0.000, 0.000, 0.00));


	model4.get()->SetPosition(glm::vec3(7.091, 1.704, 0.472));
	model4.get()->SetRotation(glm::vec3(90.000, 90.000, 0.00));
	model4.get()->SetScale(glm::vec3(7.000, 0.200, 5.000));
	model4.get()->ScreenSpaceReflectiveSwitch(false);
	model4.get()->CubeMapReflectiveSwitch(true);

	model5.get()->SetPosition(glm::vec3(-6.656, 1.672, 0.485));
	model5.get()->SetRotation(glm::vec3(-90.000, -90.000, 180.000));
	model5.get()->SetScale(glm::vec3(6.996, 0.200, 5.000));
	model5.get()->ScreenSpaceReflectiveSwitch(false);
	model5.get()->CubeMapReflectiveSwitch(true);

	model6.get()->SetPosition(glm::vec3(0.287, -3.482, 0.467));
	model6.get()->SetRotation(glm::vec3(0.000, 180.000, 0.00));
	model6.get()->SetScale(glm::vec3(7.000, 0.200, 7.000));
	model6.get()->ScreenSpaceReflectiveSwitch(false);
	model6.get()->CubeMapReflectiveSwitch(true);

	model7.get()->SetPosition(glm::vec3(-0.000, 1.741, -6.425));
	model7.get()->SetRotation(glm::vec3(0.000, 90.000, 0.000));
	model7.get()->SetScale(glm::vec3(0.100, 5.000, 7.500));
	model7.get()->ScreenSpaceReflectiveSwitch(false);
	model7.get()->CubeMapReflectiveSwitch(true);

	model8.get()->SetPosition(glm::vec3(0.957, -3.219, 1.225));
	model8.get()->SetRotation(glm::vec3(0.000, 20, 0.000));
	model8.get()->SetScale(glm::vec3(0.060, 0.060, 0.060));
	model8.get()->ScreenSpaceReflectiveSwitch(false);
	model8.get()->CubeMapReflectiveSwitch(true);

	Models.push_back(std::move(model));
	Models.push_back(std::move(model2));
	Models.push_back(std::move(model3));
	Models.push_back(std::move(model4));
	Models.push_back(std::move(model5));
	Models.push_back(std::move(model6));
	Models.push_back(std::move(model7));
	Models.push_back(std::move(model8));

	UserInterfaceItems.push_back(Models[0].get());
	UserInterfaceItems.push_back(Models[1].get());
	UserInterfaceItems.push_back(Models[2].get());
	UserInterfaceItems.push_back(Models[3].get());
	UserInterfaceItems.push_back(Models[4].get());
	UserInterfaceItems.push_back(Models[5].get());
	UserInterfaceItems.push_back(Models[6].get());
	UserInterfaceItems.push_back(Models[7].get());


	lighting_FullScreenQuad = std::shared_ptr<Lighting_FullScreenQuad>(new Lighting_FullScreenQuad(bufferManger.get(), vulkanContext.get(), camera.get(), commandPool, skyBox.get()), Lighting_FullScreenQuadDeleter);
	ssao_FullScreenQuad     = std::shared_ptr<SSA0_FullScreenQuad>(new SSA0_FullScreenQuad(bufferManger.get(), vulkanContext.get(), camera.get(), commandPool), SSA0_FullScreenQuadDeleter);
	ssaoBlur_FullScreenQuad = std::shared_ptr<SSAOBlur_FullScreenQuad>(new SSAOBlur_FullScreenQuad(bufferManger.get(), vulkanContext.get(), camera.get(), commandPool), SSAOBlur_FullScreenQuadDeleter);
	fxaa_FullScreenQuad     = std::shared_ptr<FXAA_FullScreenQuad>(new FXAA_FullScreenQuad(bufferManger.get(), vulkanContext.get(), camera.get(), commandPool), FXAA_FullScreenQuadDeleter);
	ssr_FullScreenQuad      = std::shared_ptr<SSR_FullScreenQuad>(new SSR_FullScreenQuad(bufferManger.get(), vulkanContext.get(), camera.get(), commandPool), SSR_FullScreenQuadDeleter);
	Combined_FullScreenQuad = std::shared_ptr<CombinedResult_FullScreenQuad>(new CombinedResult_FullScreenQuad(bufferManger.get(), vulkanContext.get(), camera.get(), commandPool), CombinedResult_FullScreenQuadDeleter);
	SSGI_FullScreenQuad     = std::shared_ptr<SSGI>(new SSGI(bufferManger.get(), vulkanContext.get(), camera.get(), commandPool), SSGIDeleter);

	Raytracing_Shadows      = std::shared_ptr<RayTracing>(new RayTracing(vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), RayTracingDeleter);

	lights.reserve(2);

	for (int i = 0; i < 3; i++) {
		std::shared_ptr<Light> light = std::shared_ptr<Light>(new Light(vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), LightDeleter);
		lights.push_back(std::move(light));
	}

	lights[0]->SetPosition(glm::vec3(-0.529, -0.678, -0.957));
	lights[0]->lightType = 0;
	lights[0]->lightIntensity = 9;
	lights[0]->CastShadowsSwitch(true);
	lights[0]->ambientStrength = 0.1;
	lights[0]->SetScale(glm::vec3(0.100, 0.100, 0.100));

	lights[1]->SetPosition(glm::vec3(0.598, 24.282, 0.477));
	lights[1]->SetScale(glm::vec3(0.100, 0.100, 0.100));
	lights[1]->CastShadowsSwitch(true);
	lights[1]->ambientStrength = 0.3;

	lights[2]->SetPosition(glm::vec3(-20.0f, -50, 0.0f));

	lights[0]->color = glm::vec3(1.0f, 1.0f, 1.0f);
	lights[1]->color = glm::vec3(1.0f, 0.0f, 0.0f);
	lights[2]->color = glm::vec3(1.0f, 1.0f, 1.0f);

	UserInterfaceItems.push_back(lights[0].get());
	UserInterfaceItems.push_back(lights[1].get());
	UserInterfaceItems.push_back(lights[2].get());


	createDescriptorPool();

	createCommandBuffer();
	CreateGraphicsPipeline();
	createShaderBindingTable();
	createDepthTextureImage();

	createTLAS();

	createGBuffer();
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

		 glm::mat ModelMatrix = Models[i]->GetModelMatrix();

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
	StorageImagepoolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

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
	//////////////////////////////////////////////////////////////////////////////////////////////////////
	gbuffer.SSAO.ImageID = "Gbuffer SSAO Texture";
	bufferManger->CreateImage(&gbuffer.SSAO,swapchainextent, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.SSAO.imageView = bufferManger->CreateImageView(&gbuffer.SSAO, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
	gbuffer.SSAO.imageSampler = bufferManger->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	gbuffer.SSAOBlured.ImageID = "Gbuffer SSAOBlured Texture";
	bufferManger->CreateImage(&gbuffer.SSAOBlured,swapchainextent, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.SSAOBlured.imageView = bufferManger->CreateImageView(&gbuffer.SSAOBlured, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
	gbuffer.SSAOBlured.imageSampler = bufferManger->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);
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
	Combined_FullScreenQuad->CreateImage(swapchainextent);


	lighting_FullScreenQuad->createDescriptorSetsBasedOnGBuffer(DescriptorPool, gbuffer, ReflectionMaskImageData);
	ssao_FullScreenQuad->createDescriptorSetsBasedOnGBuffer(DescriptorPool, gbuffer);
	ssaoBlur_FullScreenQuad->createDescriptorSetsBasedOnGBuffer(DescriptorPool, gbuffer);
	ssr_FullScreenQuad->createDescriptorSets(DescriptorPool, LightingPassImageData, gbuffer.ViewSpaceNormal,gbuffer.ViewSpacePosition, DepthTextureData, ReflectionMaskImageData,gbuffer.Materials);
	Combined_FullScreenQuad->createDescriptorSetsBasedOnGBuffer(DescriptorPool, LightingPassImageData, Raytracing_Shadows->ShadowPassImage);
	fxaa_FullScreenQuad->createDescriptorSets(DescriptorPool, Combined_FullScreenQuad->FinalResultImage);
	Raytracing_Shadows->createRaytracedDescriptorSets(DescriptorPool, TLAS, gbuffer);
	SSGI_FullScreenQuad->createDescriptorSets(DescriptorPool,gbuffer, LightingPassImageData,DepthTextureData);


	vk::CommandBuffer commandBuffer = bufferManger->CreateSingleUseCommandBuffer(commandPool);

	ImageTransitionData transitionInfo{};
	transitionInfo.oldlayout = vk::ImageLayout::eUndefined;
	transitionInfo.newlayout = vk::ImageLayout::eColorAttachmentOptimal;
	transitionInfo.AspectFlag = vk::ImageAspectFlagBits::eColor;
	transitionInfo.SourceAccessflag = vk::AccessFlagBits::eNone;
	transitionInfo.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
	transitionInfo.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	transitionInfo.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	bufferManger->TransitionImage(commandBuffer, &gbuffer.ViewSpacePosition, transitionInfo);
	bufferManger->TransitionImage(commandBuffer, &gbuffer.ViewSpaceNormal, transitionInfo);
	bufferManger->TransitionImage(commandBuffer, &gbuffer.Position, transitionInfo);
	bufferManger->TransitionImage(commandBuffer, &gbuffer.Normal, transitionInfo);
	bufferManger->TransitionImage(commandBuffer, &gbuffer.Albedo, transitionInfo);
	bufferManger->TransitionImage(commandBuffer, &LightingPassImageData, transitionInfo);
	bufferManger->TransitionImage(commandBuffer, &ReflectionMaskImageData, transitionInfo);
	bufferManger->TransitionImage(commandBuffer, &Combined_FullScreenQuad->FinalResultImage, transitionInfo);

	bufferManger->SubmitAndDestoyCommandBuffer(commandPool, commandBuffer, vulkanContext->graphicsQueue);


	FinalRenderTextureId = ImGui_ImplVulkan_AddTexture(fxaa_FullScreenQuad->FxaaImage.imageSampler,
		                                               fxaa_FullScreenQuad->FxaaImage.imageView,
		                                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	LightingAndReflectionsRenderTextureId = ImGui_ImplVulkan_AddTexture(LightingPassImageData.imageSampler,
		                                                                LightingPassImageData.imageView,
		                                                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	Shadow_TextureId       = ImGui_ImplVulkan_AddTexture (Raytracing_Shadows->ShadowPassImage.imageSampler,
		                                                  Raytracing_Shadows->ShadowPassImage.imageView,
		                                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


	SSAOTextureId           = ImGui_ImplVulkan_AddTexture(gbuffer.SSAOBlured.imageSampler,
		                                                  gbuffer.SSAOBlured.imageView,
		                                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


	PositionRenderTextureId = ImGui_ImplVulkan_AddTexture(gbuffer.Position.imageSampler,
		                                                  gbuffer.Position.imageView,
		                                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	NormalTextureId         = ImGui_ImplVulkan_AddTexture(gbuffer.Normal.imageSampler,
		                                                  gbuffer.Normal.imageView,
		                                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


	AlbedoTextureId         = ImGui_ImplVulkan_AddTexture(gbuffer.Albedo.imageSampler,
		                                                  gbuffer.Albedo.imageView,
		                                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	SSGITextureId            = ImGui_ImplVulkan_AddTexture(SSGI_FullScreenQuad->SSGIBlurPassImage.imageSampler,
		                                                   SSGI_FullScreenQuad->SSGIBlurPassImage.imageView,
		                                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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
    	auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/FullScreenQuad.vert.spv");
    	auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/DefferedLightingPass.frag.spv");
    
    	VkShaderModule VertShaderModule = createShaderModule(VertShaderCode);
    	VkShaderModule FragShaderModule = createShaderModule(FragShaderCode);
    
    	vk::PipelineShaderStageCreateInfo VertShaderStageInfo{};
    	VertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    	VertShaderStageInfo.module = VertShaderModule;
    	VertShaderStageInfo.pName = "main";
    
    	vk::PipelineShaderStageCreateInfo FragmentShaderStageInfo{};
    	FragmentShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    	FragmentShaderStageInfo.module = FragShaderModule;
    	FragmentShaderStageInfo.pName = "main";
    
    	vk::PipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo ,FragmentShaderStageInfo };
    
    	auto BindDesctiptions = FullScreenQuadDescription::GetBindingDescription();
    	auto attributeDescriptions = FullScreenQuadDescription::GetAttributeDescription();
    
    	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
    	vertexInputInfo.setVertexBindingDescriptionCount(1);
    	vertexInputInfo.setVertexAttributeDescriptionCount(2);
    	vertexInputInfo.setPVertexBindingDescriptions(&BindDesctiptions);
    	vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());
    
    	vk::PipelineDepthStencilStateCreateInfo depthStencilState;
    	depthStencilState.depthTestEnable = VK_FALSE;
    	depthStencilState.depthWriteEnable = VK_FALSE;
    	depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
    	depthStencilState.minDepthBounds = 0.0f;
    	depthStencilState.maxDepthBounds = 1.0f;
    	depthStencilState.stencilTestEnable = VK_FALSE;

		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanContext->swapchainformat;

    	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    	pipelineLayoutInfo.setLayoutCount = 1;
    	pipelineLayoutInfo.setSetLayouts(lighting_FullScreenQuad->descriptorSetLayout);
    	pipelineLayoutInfo.pushConstantRangeCount = 0;
    	pipelineLayoutInfo.pPushConstantRanges = nullptr;
    
		DeferedLightingPassPipelineLayout = vulkanContext->LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);
    
		DeferedLightingPassPipeline = vulkanContext->createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
    		                                                     viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, DeferedLightingPassPipelineLayout);
    
    	vulkanContext->LogicalDevice.destroyShaderModule(VertShaderModule);
    	vulkanContext->LogicalDevice.destroyShaderModule(FragShaderModule);
    }

	{
		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/FullScreenQuad.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/FXAA.frag.spv");

		VkShaderModule VertShaderModule = createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = createShaderModule(FragShaderCode);

		vk::PipelineShaderStageCreateInfo VertShaderStageInfo{};
		VertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		VertShaderStageInfo.module = VertShaderModule;
		VertShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo FragmentShaderStageInfo{};
		FragmentShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		FragmentShaderStageInfo.module = FragShaderModule;
		FragmentShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo ,FragmentShaderStageInfo };

		auto BindDesctiptions = FullScreenQuadDescription::GetBindingDescription();
		auto attributeDescriptions = FullScreenQuadDescription::GetAttributeDescription();

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.setVertexBindingDescriptionCount(1);
		vertexInputInfo.setVertexAttributeDescriptionCount(2);
		vertexInputInfo.setPVertexBindingDescriptions(&BindDesctiptions);
		vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());

		vk::PipelineDepthStencilStateCreateInfo depthStencilState;
		depthStencilState.depthTestEnable = VK_FALSE;
		depthStencilState.depthWriteEnable = VK_FALSE;
		depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.stencilTestEnable = VK_FALSE;

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

		FXAAPassPipelineLayout = vulkanContext->LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		FXAAPassPipeline = vulkanContext->createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, FXAAPassPipelineLayout);

		vulkanContext->LogicalDevice.destroyShaderModule(VertShaderModule);
		vulkanContext->LogicalDevice.destroyShaderModule(FragShaderModule);
	}


	
	{
		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/FullScreenQuad.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/SSAO_Shader.frag.spv");

		VkShaderModule VertShaderModule = createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = createShaderModule(FragShaderCode);

		vk::PipelineShaderStageCreateInfo VertShaderStageInfo{};
		VertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		VertShaderStageInfo.module = VertShaderModule;
		VertShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo FragShaderStageInfo{};
		FragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		FragShaderStageInfo.module = FragShaderModule;
		FragShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo,FragShaderStageInfo };

		auto BindDesctiptions      = FullScreenQuadDescription::GetBindingDescription();
		auto attributeDescriptions = FullScreenQuadDescription::GetAttributeDescription();

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.setVertexBindingDescriptionCount(1);
		vertexInputInfo.setVertexAttributeDescriptionCount(2);
		vertexInputInfo.setPVertexBindingDescriptions(&BindDesctiptions);
		vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());

		vk::PipelineDepthStencilStateCreateInfo depthStencilState;
		depthStencilState.depthTestEnable  = vk::False;
		depthStencilState.depthWriteEnable = vk::False;
		depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.stencilTestEnable = vk::False;

		std::array<vk::Format, 1> colorFormats = { vk::Format::eR8G8B8A8Unorm };

		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(ssao_FullScreenQuad->descriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;
		
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


		SSAOPipelineLayout = vulkanContext->LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		SSAOPipeline = vulkanContext->createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, SSAOPipelineLayout,2);

		vulkanContext->LogicalDevice.destroyShaderModule(VertShaderModule);
		vulkanContext->LogicalDevice.destroyShaderModule(FragShaderModule);
	}

	{
		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/FullScreenQuad.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/SSAOBlur_Shader.frag.spv");

		VkShaderModule VertShaderModule = createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = createShaderModule(FragShaderCode);

		vk::PipelineShaderStageCreateInfo VertShaderStageInfo{};
		VertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		VertShaderStageInfo.module = VertShaderModule;
		VertShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo FragShaderStageInfo{};
		FragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		FragShaderStageInfo.module = FragShaderModule;
		FragShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo,FragShaderStageInfo };

		auto BindDesctiptions = FullScreenQuadDescription::GetBindingDescription();
		auto attributeDescriptions = FullScreenQuadDescription::GetAttributeDescription();

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.setVertexBindingDescriptionCount(1);
		vertexInputInfo.setVertexAttributeDescriptionCount(2);
		vertexInputInfo.setPVertexBindingDescriptions(&BindDesctiptions);
		vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());

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

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(ssaoBlur_FullScreenQuad->descriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

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


		SSAOBlurPipelineLayout = vulkanContext->LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		SSAOBlurPipeline = vulkanContext->createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, SSAOBlurPipelineLayout, 2);

		vulkanContext->LogicalDevice.destroyShaderModule(VertShaderModule);
		vulkanContext->LogicalDevice.destroyShaderModule(FragShaderModule);

	}


	{
		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/FullScreenQuad.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/CombinedImage.frag.spv");

		VkShaderModule VertShaderModule = createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = createShaderModule(FragShaderCode);

		vk::PipelineShaderStageCreateInfo VertShaderStageInfo{};
		VertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		VertShaderStageInfo.module = VertShaderModule;
		VertShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo FragShaderStageInfo{};
		FragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		FragShaderStageInfo.module = FragShaderModule;
		FragShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo,FragShaderStageInfo };

		auto BindDesctiptions = FullScreenQuadDescription::GetBindingDescription();
		auto attributeDescriptions = FullScreenQuadDescription::GetAttributeDescription();

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.setVertexBindingDescriptionCount(1);
		vertexInputInfo.setVertexAttributeDescriptionCount(2);
		vertexInputInfo.setPVertexBindingDescriptions(&BindDesctiptions);
		vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());

		vk::PipelineDepthStencilStateCreateInfo depthStencilState;
		depthStencilState.depthTestEnable = vk::False;
		depthStencilState.depthWriteEnable = vk::False;
		depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.stencilTestEnable = vk::False;

		std::array<vk::Format, 1> colorFormats = { vulkanContext->swapchainformat };

		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(Combined_FullScreenQuad->descriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

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


		CombinedImagePipelineLayout = vulkanContext->LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		CombinedImagePassPipeline = vulkanContext->createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, CombinedImagePipelineLayout, 2);

		vulkanContext->LogicalDevice.destroyShaderModule(VertShaderModule);
		vulkanContext->LogicalDevice.destroyShaderModule(FragShaderModule);

	}

	{
		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/FullScreenQuad.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/SSR.frag.spv");

		VkShaderModule VertShaderModule = createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = createShaderModule(FragShaderCode);

		vk::PipelineShaderStageCreateInfo VertShaderStageInfo{};
		VertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		VertShaderStageInfo.module = VertShaderModule;
		VertShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo FragShaderStageInfo{};
		FragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		FragShaderStageInfo.module = FragShaderModule;
		FragShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo,FragShaderStageInfo };

		auto BindDesctiptions = FullScreenQuadDescription::GetBindingDescription();
		auto attributeDescriptions = FullScreenQuadDescription::GetAttributeDescription();

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.setVertexBindingDescriptionCount(1);
		vertexInputInfo.setVertexAttributeDescriptionCount(2);
		vertexInputInfo.setPVertexBindingDescriptions(&BindDesctiptions);
		vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());

		vk::PipelineDepthStencilStateCreateInfo depthStencilState;
		depthStencilState.depthTestEnable = vk::False;
		depthStencilState.depthWriteEnable = vk::False;
		depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.stencilTestEnable = vk::False;

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


		SSRPipelineLayout = vulkanContext->LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		SSRPipeline = vulkanContext->createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, SSRPipelineLayout, 2);

		vulkanContext->LogicalDevice.destroyShaderModule(VertShaderModule);
		vulkanContext->LogicalDevice.destroyShaderModule(FragShaderModule);

	}

	{
		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/Light_Shader.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/Light_Shader.frag.spv");

		VkShaderModule VertShaderModule = createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = createShaderModule(FragShaderCode);

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

		vk::PipelineDepthStencilStateCreateInfo depthStencilState;
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
		
		LightgraphicsPipeline = vulkanContext->createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			                                  viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, LightpipelineLayout);

		vulkanContext->LogicalDevice.destroyShaderModule(VertShaderModule);
		vulkanContext->LogicalDevice.destroyShaderModule(FragShaderModule);

	}

	{
		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/SkyBox_Shader.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/SkyBox_Shader.frag.spv");

		VkShaderModule VertShaderModule = createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = createShaderModule(FragShaderCode);

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

		 SkyBoxgraphicsPipeline =  vulkanContext->createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			                                  viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, SkyBoxpipelineLayout);

		 vulkanContext->LogicalDevice.destroyShaderModule(VertShaderModule);
		 vulkanContext->LogicalDevice.destroyShaderModule(FragShaderModule);

	}


	{
		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/GeometryPass.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/GeometryPass.frag.spv");

		VkShaderModule VertShaderModule = createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = createShaderModule(FragShaderCode);

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
		vertexInputInfo.setVertexAttributeDescriptionCount(5);
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

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(Models[0]->descriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

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

		geometryPassPipeline = vulkanContext->createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, geometryPassPipelineLayout);

		vulkanContext->LogicalDevice.destroyShaderModule(VertShaderModule);
		vulkanContext->LogicalDevice.destroyShaderModule(FragShaderModule);
	}

	///////////////////////////////////////////RAY TRACING PIPELINES////////////////////////////////////////////////////////////////

	{
		auto RayGen_ShaderCode        = readFile("../Shaders/Compiled_Shader_Files/raygen.rgen.spv");
		auto RayGenMiss_ShaderCode    = readFile("../Shaders/Compiled_Shader_Files/RayGenMiss.rmiss.spv");
		//auto ShadowMiss_ShaderCode    = readFile("../Shaders/Compiled_Shader_Files/ShadowMiss.rmiss.spv");
		//auto RayClosestHit_ShaderCode = readFile("../Shaders/Compiled_Shader_Files/closesthit.rchit.spv");

		VkShaderModule RayGen_ShaderModule        = createShaderModule(RayGen_ShaderCode);
		VkShaderModule RayMiss_ShaderModule       = createShaderModule(RayGenMiss_ShaderCode);
		//VkShaderModule ShadowMiss_ShaderModule    = createShaderModule(ShadowMiss_ShaderCode);
		//VkShaderModule RayClosestHit_ShaderModule = createShaderModule(RayClosestHit_ShaderCode);

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

		/*vk::PipelineShaderStageCreateInfo    ShadowMiss_ShaderStageInfo{};
		ShadowMiss_ShaderStageInfo.sType    = vk::StructureType::ePipelineShaderStageCreateInfo;
		ShadowMiss_ShaderStageInfo.stage    = vk::ShaderStageFlagBits::eMissKHR;
		ShadowMiss_ShaderStageInfo.module   = ShadowMiss_ShaderModule;
		ShadowMiss_ShaderStageInfo.pName    = "main";


		vk::PipelineShaderStageCreateInfo    RayClosestHit_ShaderStageInfo{};
		RayClosestHit_ShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		RayClosestHit_ShaderStageInfo.stage = vk::ShaderStageFlagBits::eClosestHitKHR;
		RayClosestHit_ShaderStageInfo.module = RayClosestHit_ShaderModule;
		RayClosestHit_ShaderStageInfo.pName = "main";*/

	   /*std::vector<vk::PipelineShaderStageCreateInfo> ShaderStages = { RayGen_ShaderStageInfo ,
			                                                           RayMiss_ShaderStageInfo,
																	   ShadowMiss_ShaderStageInfo ,
																	   RayClosestHit_ShaderStageInfo };*/

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

	   //vk::RayTracingShaderGroupCreateInfoKHR ShadowMiss_GroupInfo{};
	   //ShadowMiss_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
	   //ShadowMiss_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
	   //ShadowMiss_GroupInfo.generalShader = 2;
	   //ShadowMiss_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
	   //ShadowMiss_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
	   //ShadowMiss_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

	   vk::RayTracingShaderGroupCreateInfoKHR Hit_GroupInfo{};
	   Hit_GroupInfo.sType = vk::StructureType::eRayTracingShaderGroupCreateInfoKHR;
	   Hit_GroupInfo.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
	   Hit_GroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
	   Hit_GroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
	   Hit_GroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
	   Hit_GroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;


	   //std::vector<vk::RayTracingShaderGroupCreateInfoKHR> ShaderGroups = {
		  // RayGen_GroupInfo,
		  // Miss_GroupInfo,
		  // ShadowMiss_GroupInfo,
		  // Hit_GroupInfo,
	   //};


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
	   
	   RT_ShadowsPassPipeline = vulkanContext->createRayTracingGraphicsPipeline(RT_ShadowsPipelineLayout, ShaderStages, ShaderGroups);
	   
	   vulkanContext->LogicalDevice.destroyShaderModule(RayGen_ShaderModule);
	   vulkanContext->LogicalDevice.destroyShaderModule(RayMiss_ShaderModule);
	  /* vulkanContext->LogicalDevice.destroyShaderModule(RayClosestHit_ShaderModule);
	   vulkanContext->LogicalDevice.destroyShaderModule(ShadowMiss_ShaderModule);*/

	}


	{
		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/FullScreenQuad.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/SSGI.frag.spv");

		VkShaderModule VertShaderModule = createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = createShaderModule(FragShaderCode);

		vk::PipelineShaderStageCreateInfo VertShaderStageInfo{};
		VertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		VertShaderStageInfo.module = VertShaderModule;
		VertShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo FragShaderStageInfo{};
		FragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		FragShaderStageInfo.module = FragShaderModule;
		FragShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo,FragShaderStageInfo };

		auto BindDesctiptions = FullScreenQuadDescription::GetBindingDescription();
		auto attributeDescriptions = FullScreenQuadDescription::GetAttributeDescription();

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.setVertexBindingDescriptionCount(1);
		vertexInputInfo.setVertexAttributeDescriptionCount(2);
		vertexInputInfo.setPVertexBindingDescriptions(&BindDesctiptions);
		vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());

		vk::PipelineDepthStencilStateCreateInfo depthStencilState;
		depthStencilState.depthTestEnable = vk::False;
		depthStencilState.depthWriteEnable = vk::False;
		depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.stencilTestEnable = vk::False;

		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanContext->swapchainformat;

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(glm::mat4));
		range.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(SSGI_FullScreenQuad->descriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

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


		SSGIPipelineLayout = vulkanContext->LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		SSGIPipeline = vulkanContext->createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, SSGIPipelineLayout, 2);

		vulkanContext->LogicalDevice.destroyShaderModule(VertShaderModule);
		vulkanContext->LogicalDevice.destroyShaderModule(FragShaderModule);

	}


	{
		auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/FullScreenQuad.vert.spv");
		auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/BilateralFilter.frag.spv");

		VkShaderModule VertShaderModule = createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = createShaderModule(FragShaderCode);

		vk::PipelineShaderStageCreateInfo VertShaderStageInfo{};
		VertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		VertShaderStageInfo.module = VertShaderModule;
		VertShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo FragShaderStageInfo{};
		FragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		FragShaderStageInfo.module = FragShaderModule;
		FragShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo,FragShaderStageInfo };

		auto BindDesctiptions = FullScreenQuadDescription::GetBindingDescription();
		auto attributeDescriptions = FullScreenQuadDescription::GetAttributeDescription();

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.setVertexBindingDescriptionCount(1);
		vertexInputInfo.setVertexAttributeDescriptionCount(2);
		vertexInputInfo.setPVertexBindingDescriptions(&BindDesctiptions);
		vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());

		vk::PipelineDepthStencilStateCreateInfo depthStencilState;
		depthStencilState.depthTestEnable = vk::False;
		depthStencilState.depthWriteEnable = vk::False;
		depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.stencilTestEnable = vk::False;

		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanContext->swapchainformat;

		vk::PushConstantRange range{};
		range.setOffset(0);
		range.setSize(sizeof(glm::mat4));
		range.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.setSetLayouts(SSGI_FullScreenQuad->BilateralFilterDescriptorSetLayout);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

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


		BluredSSGIPipelineLayout = vulkanContext->LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		BluredSSGIPipeline = vulkanContext->createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, &vertexInputInfo, &inputAssembleInfo,
			viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, BluredSSGIPipelineLayout, 2);

		vulkanContext->LogicalDevice.destroyShaderModule(VertShaderModule);
		vulkanContext->LogicalDevice.destroyShaderModule(FragShaderModule);

	}

}

uint32_t App::alignedSize(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

void App::createShaderBindingTable() {

	const size_t   handleSize        = vulkanContext->RayTracingPipelineProperties.shaderGroupHandleSize;
	const size_t   handleSizeAligned = alignedSize(handleSize, vulkanContext->RayTracingPipelineProperties.shaderGroupHandleAlignment);
	const uint32_t groupCount        = 3;  // Adjust based on your actual shader groups
	const uint32_t sbtSize           = groupCount * handleSizeAligned;

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
	missShaderBindingTableBuffer.BufferID   = "miss Shader Binding Table Buffer";
	hitShaderBindingTableBuffer.BufferID    = "hit Shader Binding Table Buffer";

	bufferManger->CreateBuffer(&raygenShaderBindingTableBuffer, handleSizeAligned     , vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext->graphicsQueue);
	bufferManger->CreateBuffer(&missShaderBindingTableBuffer  , handleSizeAligned     , vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext->graphicsQueue);
	bufferManger->CreateBuffer(&hitShaderBindingTableBuffer   , handleSizeAligned     , vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR, commandPool, vulkanContext->graphicsQueue);

	bufferManger->CopyDataToBuffer(shaderHandleStorage.data()                        , raygenShaderBindingTableBuffer);
	bufferManger->CopyDataToBuffer(shaderHandleStorage.data() + handleSizeAligned    , missShaderBindingTableBuffer);
	bufferManger->CopyDataToBuffer(shaderHandleStorage.data() + handleSizeAligned * 2, hitShaderBindingTableBuffer);

}

vk::ShaderModule App::createShaderModule(const std::vector<char>& code)
{
	vk::ShaderModuleCreateInfo createInfo{};
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	vk::ShaderModule Shader;

	if (vulkanContext->LogicalDevice.createShaderModule(&createInfo, nullptr, &Shader) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create shader module!");
	}

	return Shader;
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

		userinterface->DrawUi(this);
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
    SSGI_FullScreenQuad->UpdateUniformBuffer(currentImage, lights);

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
		ImageTransitionData TransitionColorAttachmentOptimal{};
		TransitionColorAttachmentOptimal.oldlayout = vk::ImageLayout::eUndefined;
		TransitionColorAttachmentOptimal.newlayout = vk::ImageLayout::eColorAttachmentOptimal;
		TransitionColorAttachmentOptimal.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionColorAttachmentOptimal.SourceAccessflag = vk::AccessFlagBits::eNone;
		TransitionColorAttachmentOptimal.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
		TransitionColorAttachmentOptimal.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
		TransitionColorAttachmentOptimal.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		bufferManger->TransitionImage(commandBuffer, &gbuffer.Position, TransitionColorAttachmentOptimal);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.ViewSpacePosition, TransitionColorAttachmentOptimal);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.Normal, TransitionColorAttachmentOptimal);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.ViewSpaceNormal, TransitionColorAttachmentOptimal);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.Albedo, TransitionColorAttachmentOptimal);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.Materials, TransitionColorAttachmentOptimal);
		bufferManger->TransitionImage(commandBuffer, &LightingPassImageData, TransitionColorAttachmentOptimal);
		bufferManger->TransitionImage(commandBuffer, &ssr_FullScreenQuad->SSRImage, TransitionColorAttachmentOptimal);
		bufferManger->TransitionImage(commandBuffer, &ReflectionMaskImageData, TransitionColorAttachmentOptimal);
		bufferManger->TransitionImage(commandBuffer, &Combined_FullScreenQuad->FinalResultImage, TransitionColorAttachmentOptimal);
		bufferManger->TransitionImage(commandBuffer, &SSGI_FullScreenQuad->SSGIPassImage, TransitionColorAttachmentOptimal);
		bufferManger->TransitionImage(commandBuffer, &SSGI_FullScreenQuad->SSGIBlurPassImage,TransitionColorAttachmentOptimal);


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
		//transiiton position and normal in prep of ssao
		ImageTransitionData transitiontoshaderInfo{};
		transitiontoshaderInfo.oldlayout = vk::ImageLayout::eColorAttachmentOptimal;
		transitiontoshaderInfo.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		transitiontoshaderInfo.AspectFlag = vk::ImageAspectFlagBits::eColor;
		transitiontoshaderInfo.SourceAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
		transitiontoshaderInfo.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
		transitiontoshaderInfo.SourceOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		transitiontoshaderInfo.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

		bufferManger->TransitionImage(commandBuffer, &gbuffer.ViewSpacePosition, transitiontoshaderInfo);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.ViewSpaceNormal, transitiontoshaderInfo);

		ImageTransitionData transitionInfo{};
		transitionInfo.oldlayout = vk::ImageLayout::eUndefined;
		transitionInfo.newlayout = vk::ImageLayout::eColorAttachmentOptimal;
		transitionInfo.AspectFlag = vk::ImageAspectFlagBits::eColor;
		transitionInfo.SourceAccessflag = vk::AccessFlagBits::eNone;
		transitionInfo.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
		transitionInfo.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
		transitionInfo.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		bufferManger->TransitionImage(commandBuffer, &gbuffer.SSAO, transitionInfo);

		vk::RenderingAttachmentInfo SSAOColorAttachment{};
		SSAOColorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		SSAOColorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		SSAOColorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		SSAOColorAttachment.imageView = gbuffer.SSAO.imageView;
		SSAOColorAttachment.clearValue = clearColor;

		vk::RenderingInfo renderingInfo{};
		renderingInfo.renderArea.offset = imageoffset;
		renderingInfo.renderArea.extent.height = vulkanContext->swapchainExtent.height;
		renderingInfo.renderArea.extent.width = vulkanContext->swapchainExtent.width;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &SSAOColorAttachment;


		commandBuffer.beginRendering(renderingInfo);
		commandBuffer.setViewport(0, 1, &viewport);
		commandBuffer.setScissor(0, 1, &scissor);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, SSAOPipeline);

		ssao_FullScreenQuad->Draw(commandBuffer, SSAOPipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}

	{
		//transiiton SSAO in prep for blur
		ImageTransitionData gbufferTransition{};
		gbufferTransition.oldlayout = vk::ImageLayout::eColorAttachmentOptimal;
		gbufferTransition.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		gbufferTransition.AspectFlag = vk::ImageAspectFlagBits::eColor;
		gbufferTransition.SourceAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
		gbufferTransition.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
		gbufferTransition.SourceOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		gbufferTransition.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

		bufferManger->TransitionImage(commandBuffer, &gbuffer.SSAO, gbufferTransition);

		ImageTransitionData transitionInfo{};
		transitionInfo.oldlayout = vk::ImageLayout::eUndefined;
		transitionInfo.newlayout = vk::ImageLayout::eColorAttachmentOptimal;
		transitionInfo.AspectFlag = vk::ImageAspectFlagBits::eColor;
		transitionInfo.SourceAccessflag = vk::AccessFlagBits::eNone;
		transitionInfo.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
		transitionInfo.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
		transitionInfo.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		bufferManger->TransitionImage(commandBuffer, &gbuffer.SSAOBlured, transitionInfo);

		vk::RenderingAttachmentInfo SSAOBluredColorAttachment{};
		SSAOBluredColorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		SSAOBluredColorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		SSAOBluredColorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		SSAOBluredColorAttachment.imageView = gbuffer.SSAOBlured.imageView;
		SSAOBluredColorAttachment.clearValue = clearColor;

		vk::RenderingInfo renderingInfo{};
		renderingInfo.renderArea.offset = imageoffset;
		renderingInfo.renderArea.extent.height = vulkanContext->swapchainExtent.height;
		renderingInfo.renderArea.extent.width = vulkanContext->swapchainExtent.width;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &SSAOBluredColorAttachment;


		commandBuffer.beginRendering(renderingInfo);
		commandBuffer.setViewport(0, 1, &viewport);
		commandBuffer.setScissor(0, 1, &scissor);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, SSAOBlurPipeline);

		ssaoBlur_FullScreenQuad->Draw(commandBuffer, SSAOBlurPipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}

    /////////////////// LIGHTING PASS ///////////////////////// 
	{
		ImageTransitionData gbufferTransition{};
		gbufferTransition.oldlayout = vk::ImageLayout::eColorAttachmentOptimal;
		gbufferTransition.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		gbufferTransition.AspectFlag = vk::ImageAspectFlagBits::eColor;
		gbufferTransition.SourceAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
		gbufferTransition.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
		gbufferTransition.SourceOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		gbufferTransition.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

		bufferManger->TransitionImage(commandBuffer, &gbuffer.Position, gbufferTransition);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.Normal  , gbufferTransition);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.Albedo  , gbufferTransition);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.SSAOBlured, gbufferTransition);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.Materials, gbufferTransition);
		bufferManger->TransitionImage(commandBuffer, &ReflectionMaskImageData, gbufferTransition);


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


		commandBuffer.beginRendering(renderingInfo);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, DeferedLightingPassPipeline);
		lighting_FullScreenQuad->Draw(commandBuffer, DeferedLightingPassPipelineLayout, currentFrame);

		commandBuffer.endRendering();

		//////////////////////////////////////////////////////////////

		//This is a little bit unnessesery but it helps with consistency 
		ImageTransitionData TransitionBacktoColorOutput{};
		TransitionBacktoColorOutput.oldlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionBacktoColorOutput.newlayout = vk::ImageLayout::eColorAttachmentOptimal;
		TransitionBacktoColorOutput.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionBacktoColorOutput.SourceAccessflag = vk::AccessFlagBits::eShaderRead;
		TransitionBacktoColorOutput.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
		TransitionBacktoColorOutput.SourceOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;
		TransitionBacktoColorOutput.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		bufferManger->TransitionImage(commandBuffer, &gbuffer.ViewSpacePosition, TransitionBacktoColorOutput);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.ViewSpaceNormal, TransitionBacktoColorOutput);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.Position, TransitionBacktoColorOutput);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.Normal, TransitionBacktoColorOutput);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.Albedo, TransitionBacktoColorOutput);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.SSAOBlured, TransitionBacktoColorOutput);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.Materials, TransitionBacktoColorOutput);
		bufferManger->TransitionImage(commandBuffer, &ReflectionMaskImageData, TransitionBacktoColorOutput);


	}
	 /////////////////// LIGHTING PASS END ///////////////////////// 

	{

		ImageTransitionData TransitionTOShaderOptimal{};
		TransitionTOShaderOptimal.oldlayout = vk::ImageLayout::eColorAttachmentOptimal;
		TransitionTOShaderOptimal.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionTOShaderOptimal.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionTOShaderOptimal.SourceAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
		TransitionTOShaderOptimal.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
		TransitionTOShaderOptimal.SourceOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		TransitionTOShaderOptimal.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

		bufferManger->TransitionImage(commandBuffer, &gbuffer.ViewSpaceNormal, TransitionTOShaderOptimal);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.ViewSpacePosition, TransitionTOShaderOptimal);
		bufferManger->TransitionImage(commandBuffer, &LightingPassImageData, TransitionTOShaderOptimal);
		bufferManger->TransitionImage(commandBuffer, &ReflectionMaskImageData, TransitionTOShaderOptimal);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.Materials, TransitionTOShaderOptimal);

	
		ImageTransitionData TransitionDepthtTOShaderOptimal{};
		TransitionDepthtTOShaderOptimal.oldlayout = vk::ImageLayout::eDepthAttachmentOptimal;
		TransitionDepthtTOShaderOptimal.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionDepthtTOShaderOptimal.AspectFlag = vk::ImageAspectFlagBits::eDepth;
		TransitionDepthtTOShaderOptimal.SourceAccessflag = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
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


		ImageTransitionData TransitionBacktoColorOutput{};
		TransitionBacktoColorOutput.oldlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionBacktoColorOutput.newlayout = vk::ImageLayout::eColorAttachmentOptimal;
		TransitionBacktoColorOutput.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionBacktoColorOutput.SourceAccessflag = vk::AccessFlagBits::eShaderRead;
		TransitionBacktoColorOutput.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
		TransitionBacktoColorOutput.SourceOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;
		TransitionBacktoColorOutput.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		bufferManger->TransitionImage(commandBuffer, &gbuffer.ViewSpaceNormal, TransitionBacktoColorOutput);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.ViewSpacePosition, TransitionBacktoColorOutput);
		bufferManger->TransitionImage(commandBuffer, &LightingPassImageData, TransitionBacktoColorOutput);
		bufferManger->TransitionImage(commandBuffer, &ReflectionMaskImageData, TransitionBacktoColorOutput);


		ImageTransitionData TransitionDeptTODepthOptimal{};
		TransitionDeptTODepthOptimal.oldlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionDeptTODepthOptimal.newlayout = vk::ImageLayout::eDepthAttachmentOptimal;
		TransitionDeptTODepthOptimal.AspectFlag = vk::ImageAspectFlagBits::eDepth;
		TransitionDeptTODepthOptimal.SourceAccessflag = vk::AccessFlagBits::eShaderRead;
		TransitionDeptTODepthOptimal.DestinationAccessflag = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		TransitionDeptTODepthOptimal.SourceOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;
		TransitionDeptTODepthOptimal.DestinationOnThePipeline = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		bufferManger->TransitionImage(commandBuffer, &DepthTextureData, TransitionDeptTODepthOptimal);
	}

	{
		ImageTransitionData TransitionTOShaderOptimal{};
		TransitionTOShaderOptimal.oldlayout = vk::ImageLayout::eColorAttachmentOptimal;
		TransitionTOShaderOptimal.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionTOShaderOptimal.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionTOShaderOptimal.SourceAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
		TransitionTOShaderOptimal.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
		TransitionTOShaderOptimal.SourceOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		TransitionTOShaderOptimal.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

		bufferManger->TransitionImage(commandBuffer, &gbuffer.Position, TransitionTOShaderOptimal);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.Normal, TransitionTOShaderOptimal);

		ImageTransitionData TransitiontoGeneral{};
		TransitiontoGeneral.oldlayout = vk::ImageLayout::eUndefined;
		TransitiontoGeneral.newlayout = vk::ImageLayout::eGeneral;
		TransitiontoGeneral.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitiontoGeneral.SourceAccessflag = vk::AccessFlagBits::eNone;
		TransitiontoGeneral.DestinationAccessflag = vk::AccessFlagBits::eShaderWrite;
		TransitiontoGeneral.SourceOnThePipeline = vk::PipelineStageFlagBits::eNone;
		TransitiontoGeneral.DestinationOnThePipeline = vk::PipelineStageFlagBits::eRayTracingShaderKHR;

		bufferManger->TransitionImage(commandBuffer, &Raytracing_Shadows->ShadowPassImage, TransitiontoGeneral);


		commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, RT_ShadowsPassPipeline);

		Raytracing_Shadows->Draw(
			raygenShaderBindingTableBuffer,
			hitShaderBindingTableBuffer,
			missShaderBindingTableBuffer,
			commandBuffer,
			RT_ShadowsPipelineLayout,
			currentFrame);



		ImageTransitionData TransitiontoShaderOutput{};
		TransitiontoShaderOutput.oldlayout = vk::ImageLayout::eGeneral;
		TransitiontoShaderOutput.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitiontoShaderOutput.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitiontoShaderOutput.SourceAccessflag = vk::AccessFlagBits::eShaderWrite;
		TransitiontoShaderOutput.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentRead;
		TransitiontoShaderOutput.SourceOnThePipeline = vk::PipelineStageFlagBits::eRayTracingShaderKHR;
		TransitiontoShaderOutput.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

		bufferManger->TransitionImage(commandBuffer, &Raytracing_Shadows->ShadowPassImage, TransitiontoShaderOutput);

		ImageTransitionData TransitionBacktoColorOutput{};
		TransitionBacktoColorOutput.oldlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionBacktoColorOutput.newlayout = vk::ImageLayout::eColorAttachmentOptimal;
		TransitionBacktoColorOutput.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionBacktoColorOutput.SourceAccessflag = vk::AccessFlagBits::eShaderRead;
		TransitionBacktoColorOutput.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
		TransitionBacktoColorOutput.SourceOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;
		TransitionBacktoColorOutput.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		bufferManger->TransitionImage(commandBuffer, &gbuffer.Position, TransitionBacktoColorOutput);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.Normal, TransitionBacktoColorOutput);

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


		ImageTransitionData TransitionDepthtTOShaderOptimal{};
		TransitionDepthtTOShaderOptimal.oldlayout = vk::ImageLayout::eDepthAttachmentOptimal;
		TransitionDepthtTOShaderOptimal.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionDepthtTOShaderOptimal.AspectFlag = vk::ImageAspectFlagBits::eDepth;
		TransitionDepthtTOShaderOptimal.SourceAccessflag = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		TransitionDepthtTOShaderOptimal.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
		TransitionDepthtTOShaderOptimal.SourceOnThePipeline = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		TransitionDepthtTOShaderOptimal.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;
		bufferManger->TransitionImage(commandBuffer, &DepthTextureData, TransitionDepthtTOShaderOptimal);

		ImageTransitionData TransitionTOShaderOptimal{};
		TransitionTOShaderOptimal.oldlayout = vk::ImageLayout::eColorAttachmentOptimal;
		TransitionTOShaderOptimal.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionTOShaderOptimal.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionTOShaderOptimal.SourceAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
		TransitionTOShaderOptimal.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
		TransitionTOShaderOptimal.SourceOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		TransitionTOShaderOptimal.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

		bufferManger->TransitionImage(commandBuffer, &LightingPassImageData, TransitionTOShaderOptimal);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.ViewSpacePosition, TransitionTOShaderOptimal);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.ViewSpaceNormal, TransitionTOShaderOptimal);

		vk::RenderingAttachmentInfo SSGIImageAttachInfo;
		SSGIImageAttachInfo.clearValue = clearColor;
		SSGIImageAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		SSGIImageAttachInfo.imageView = SSGI_FullScreenQuad->SSGIPassImage.imageView;
		SSGIImageAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
		SSGIImageAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingInfo SSGIImageImageInfo{};
		SSGIImageImageInfo.layerCount = 1;
		SSGIImageImageInfo.colorAttachmentCount = 1;
		SSGIImageImageInfo.pColorAttachments = &SSGIImageAttachInfo;
		SSGIImageImageInfo.renderArea.extent.width = vulkanContext->swapchainExtent.width;
		SSGIImageImageInfo.renderArea.extent.height = vulkanContext->swapchainExtent.height;

		commandBuffer.setViewport(0, 1, &viewport);
		commandBuffer.setScissor(0, 1, &scissor);
		commandBuffer.beginRendering(SSGIImageImageInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, SSGIPipeline);
		SSGI_FullScreenQuad->Draw(commandBuffer, SSGIPipelineLayout, currentFrame);
		commandBuffer.endRendering();

		ImageTransitionData TransitionBacktoColorOutput{};
		TransitionBacktoColorOutput.oldlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionBacktoColorOutput.newlayout = vk::ImageLayout::eColorAttachmentOptimal;
		TransitionBacktoColorOutput.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionBacktoColorOutput.SourceAccessflag = vk::AccessFlagBits::eShaderRead;
		TransitionBacktoColorOutput.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
		TransitionBacktoColorOutput.SourceOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;
		TransitionBacktoColorOutput.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		bufferManger->TransitionImage(commandBuffer, &gbuffer.ViewSpaceNormal, TransitionBacktoColorOutput);
		bufferManger->TransitionImage(commandBuffer, &LightingPassImageData, TransitionBacktoColorOutput);
		bufferManger->TransitionImage(commandBuffer, &gbuffer.ViewSpacePosition, TransitionBacktoColorOutput);

		ImageTransitionData TransitionDeptTODepthOptimal{};
		TransitionDeptTODepthOptimal.oldlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionDeptTODepthOptimal.newlayout = vk::ImageLayout::eDepthAttachmentOptimal;
		TransitionDeptTODepthOptimal.AspectFlag = vk::ImageAspectFlagBits::eDepth;
		TransitionDeptTODepthOptimal.SourceAccessflag = vk::AccessFlagBits::eShaderRead;
		TransitionDeptTODepthOptimal.DestinationAccessflag = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		TransitionDeptTODepthOptimal.SourceOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;
		TransitionDeptTODepthOptimal.DestinationOnThePipeline = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		bufferManger->TransitionImage(commandBuffer, &DepthTextureData, TransitionDeptTODepthOptimal);

	}


	{
		ImageTransitionData TransitionTOShaderOptimal{};
		TransitionTOShaderOptimal.oldlayout = vk::ImageLayout::eColorAttachmentOptimal;
		TransitionTOShaderOptimal.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionTOShaderOptimal.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionTOShaderOptimal.SourceAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
		TransitionTOShaderOptimal.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
		TransitionTOShaderOptimal.SourceOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		TransitionTOShaderOptimal.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

		bufferManger->TransitionImage(commandBuffer, &SSGI_FullScreenQuad->SSGIPassImage, TransitionTOShaderOptimal);


		vk::RenderingAttachmentInfo BluredImageAttachInfo;
		BluredImageAttachInfo.clearValue = clearColor;
		BluredImageAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		BluredImageAttachInfo.imageView = SSGI_FullScreenQuad->SSGIBlurPassImage.imageView;
		BluredImageAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
		BluredImageAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingInfo SSGIImageInfo{};
		SSGIImageInfo.layerCount = 1;
		SSGIImageInfo.colorAttachmentCount = 1;
		SSGIImageInfo.pColorAttachments = &BluredImageAttachInfo;
		SSGIImageInfo.renderArea.extent.width = vulkanContext->swapchainExtent.width;
		SSGIImageInfo.renderArea.extent.height = vulkanContext->swapchainExtent.height;

		commandBuffer.setViewport(0, 1, &viewport);
		commandBuffer.setScissor(0, 1, &scissor);
		commandBuffer.beginRendering(SSGIImageInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, BluredSSGIPipeline);
		SSGI_FullScreenQuad->DrawBilateralFilter(commandBuffer, BluredSSGIPipelineLayout, currentFrame);
		commandBuffer.endRendering();

		ImageTransitionData TransitionBacktoColorOutput{};
		TransitionBacktoColorOutput.oldlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionBacktoColorOutput.newlayout = vk::ImageLayout::eColorAttachmentOptimal;
		TransitionBacktoColorOutput.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionBacktoColorOutput.SourceAccessflag = vk::AccessFlagBits::eShaderRead;
		TransitionBacktoColorOutput.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
		TransitionBacktoColorOutput.SourceOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;
		TransitionBacktoColorOutput.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		bufferManger->TransitionImage(commandBuffer, &SSGI_FullScreenQuad->SSGIPassImage, TransitionBacktoColorOutput);


	}



	{

		ImageTransitionData TransitionTOShaderOptimal{};
		TransitionTOShaderOptimal.oldlayout = vk::ImageLayout::eColorAttachmentOptimal;
		TransitionTOShaderOptimal.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionTOShaderOptimal.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionTOShaderOptimal.SourceAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
		TransitionTOShaderOptimal.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
		TransitionTOShaderOptimal.SourceOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		TransitionTOShaderOptimal.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

		bufferManger->TransitionImage(commandBuffer, &LightingPassImageData, TransitionTOShaderOptimal);

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



    /////////////////// FORWARD PASS ///////////////////////// 
	{
		
		vk::RenderingAttachmentInfo LightPassColorAttachmentInfo{};
		LightPassColorAttachmentInfo.imageView = Combined_FullScreenQuad->FinalResultImage.imageView;
		LightPassColorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		LightPassColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eLoad;
		LightPassColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		LightPassColorAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfo depthStencilAttachment;
		depthStencilAttachment.imageView = DepthTextureData.imageView;
		depthStencilAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		depthStencilAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
		depthStencilAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
		depthStencilAttachment.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

		vk::RenderingInfo renderingInfo{};
		renderingInfo.renderArea.offset = imageoffset;
		renderingInfo.renderArea.extent.height = vulkanContext->swapchainExtent.height;
		renderingInfo.renderArea.extent.width =  vulkanContext->swapchainExtent.width;
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
		ImageTransitionData TransitionColorOutputOptimal{};
		TransitionColorOutputOptimal.oldlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionColorOutputOptimal.newlayout = vk::ImageLayout::eColorAttachmentOptimal;
		TransitionColorOutputOptimal.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionColorOutputOptimal.SourceAccessflag = vk::AccessFlagBits::eShaderRead;
		TransitionColorOutputOptimal.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
		TransitionColorOutputOptimal.SourceOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;
		TransitionColorOutputOptimal.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		bufferManger->TransitionImage(commandBuffer, &fxaa_FullScreenQuad->FxaaImage, TransitionColorOutputOptimal);


		ImageTransitionData TransitionShaderReadOptimal{};
		TransitionShaderReadOptimal.oldlayout = vk::ImageLayout::eColorAttachmentOptimal;
		TransitionShaderReadOptimal.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionShaderReadOptimal.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionShaderReadOptimal.SourceAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
		TransitionShaderReadOptimal.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
		TransitionShaderReadOptimal.SourceOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		TransitionShaderReadOptimal.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

		bufferManger->TransitionImage(commandBuffer, &Combined_FullScreenQuad->FinalResultImage, TransitionShaderReadOptimal);

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

		commandBuffer.beginRendering(renderingInfo);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, FXAAPassPipeline);
		fxaa_FullScreenQuad->Draw(commandBuffer, FXAAPassPipelineLayout, currentFrame);
		commandBuffer.endRendering();


		ImageTransitionData TransitionBacktoColorOutput{};
		TransitionBacktoColorOutput.oldlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		TransitionBacktoColorOutput.newlayout = vk::ImageLayout::eColorAttachmentOptimal;
		TransitionBacktoColorOutput.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitionBacktoColorOutput.SourceAccessflag = vk::AccessFlagBits::eShaderRead;
		TransitionBacktoColorOutput.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
		TransitionBacktoColorOutput.SourceOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;
		TransitionBacktoColorOutput.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		bufferManger->TransitionImage(commandBuffer, &Combined_FullScreenQuad->FinalResultImage, TransitionBacktoColorOutput);
	}

	


	////////// Transition image in prep for GUI ////////////////////
	ImageTransitionData TransitiontoShader{};
	TransitiontoShader.oldlayout = vk::ImageLayout::eColorAttachmentOptimal;
	TransitiontoShader.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	TransitiontoShader.AspectFlag = vk::ImageAspectFlagBits::eColor;
	TransitiontoShader.SourceAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
	TransitiontoShader.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentRead;
	TransitiontoShader.SourceOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	TransitiontoShader.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

	bufferManger->TransitionImage(commandBuffer, &gbuffer.Position, TransitiontoShader);
	bufferManger->TransitionImage(commandBuffer, &gbuffer.Normal, TransitiontoShader);
	bufferManger->TransitionImage(commandBuffer, &gbuffer.Albedo, TransitiontoShader); 
	bufferManger->TransitionImage(commandBuffer, &gbuffer.SSAOBlured, TransitiontoShader);
	bufferManger->TransitionImage(commandBuffer, &fxaa_FullScreenQuad->FxaaImage, TransitiontoShader);
	bufferManger->TransitionImage(commandBuffer, &ssr_FullScreenQuad->SSRImage, TransitiontoShader);
	bufferManger->TransitionImage(commandBuffer, &Combined_FullScreenQuad->FinalResultImage, TransitiontoShader);
	bufferManger->TransitionImage(commandBuffer, &SSGI_FullScreenQuad->SSGIPassImage, TransitiontoShader);
	bufferManger->TransitionImage(commandBuffer, &SSGI_FullScreenQuad->SSGIBlurPassImage, TransitiontoShader);




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
	bufferManger->DestroyImage(gbuffer.SSAO);
	bufferManger->DestroyImage(gbuffer.SSAOBlured);
	bufferManger->DestroyImage(gbuffer.Materials);
	bufferManger->DestroyImage(gbuffer.Albedo);
	bufferManger->DestroyImage(LightingPassImageData);
	bufferManger->DestroyImage(ReflectionMaskImageData);

	Raytracing_Shadows->DestroyStorageImage();
	fxaa_FullScreenQuad->DestroyImage();
	ssr_FullScreenQuad->DestroyImage();
	SSGI_FullScreenQuad->DestroyImage();
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
	createDepthTextureImage();
	createGBuffer();


	camera->SetSwapChainHeight(vulkanContext->swapchainExtent.height);
	camera->SetSwapChainWidth(vulkanContext->swapchainExtent.width);
}

void App::recreatePipeline()
{
	vulkanContext->LogicalDevice.waitIdle();
	destroyPipeline();

	CreateGraphicsPipeline();
}


void App::DestroySyncObjects()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vulkanContext->LogicalDevice.destroySemaphore(presentCompleteSemaphores[i]);
		vulkanContext->LogicalDevice.destroySemaphore(renderCompleteSemaphores[i]);
		vulkanContext->LogicalDevice.destroyFence(waitFences[i]);
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
	ssaoBlur_FullScreenQuad.reset();
	fxaa_FullScreenQuad.reset();
	ssr_FullScreenQuad.reset();

	bufferManger->DestroyBuffer(TLAS_Buffer);
	bufferManger->DestroyBuffer(TLAS_SCRATCH_Buffer);
	bufferManger->DestroyBuffer(TLAS_InstanceData);
	vulkanContext->vkDestroyAccelerationStructureKHR(vulkanContext->LogicalDevice, static_cast<VkAccelerationStructureKHR>(TLAS),nullptr);
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
	vulkanContext->LogicalDevice.destroyPipeline(SSGIPipeline);
	vulkanContext->LogicalDevice.destroyPipeline(BluredSSGIPipeline);

	vulkanContext->LogicalDevice.destroyPipelineLayout(DeferedLightingPassPipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(FXAAPassPipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(LightpipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(SkyBoxpipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(geometryPassPipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(SSAOPipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(SSAOBlurPipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(SSRPipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(RT_ShadowsPipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(SSGIPipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(BluredSSGIPipelineLayout);

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

