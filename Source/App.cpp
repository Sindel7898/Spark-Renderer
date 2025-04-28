#define _CRTDBG_MAP_ALLOC
#include "App.h"
#include <optional>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include "Window.h"
#include "Camera.h"
#include "MeshLoader.h"
#include "BufferManager.h"
#include "VulkanContext.h"
#include "FramesPerSecondCounter.h"
#include "Light.h"

#include <crtdbg.h>


#define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)

 App::App()
{
	window        = std::shared_ptr<Window>(new Window(1920, 1080, "Vulkan Window"), WindowDeleter);
	vulkanContext = std::shared_ptr<VulkanContext>(new VulkanContext(*window), VulkanContextDeleter);
	bufferManger  = std::shared_ptr<BufferManager>(new BufferManager (vulkanContext->LogicalDevice, vulkanContext->PhysicalDevice, vulkanContext->VulkanInstance), BufferManagerDeleter);
  	camera        = std::shared_ptr<Camera>(new Camera (vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, window->GetWindow()));
	userinterface = std::shared_ptr<UserInterface>(new UserInterface(vulkanContext.get(), window.get(), bufferManger.get()),UserInterfaceDeleter);
	//glfwSetFramebufferSizeCallback(window->GetWindow(),);
	glfwSetWindowUserPointer(window->GetWindow(), this);

	createSyncObjects();	
	//////////////////////////
	createCommandPool();

	Models.reserve(1);

	for (int i = 0; i < 1; i++) {
		auto model = std::shared_ptr<Model>(new Model("../Textures/Helmet/DamagedHelmet.gltf", vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), ModelDeleter);
		Models.push_back(std::move(model));
	}

	std::array<const char*, 6> filePaths{
		"../Textures/skybox/posx.jpg",  // +X (Right)
		"../Textures/skybox/negx.jpg",  // -X (Left)
		"../Textures/skybox/posy.jpg",  // +Y (Top)
		"../Textures/skybox/negy.jpg",  // -Y (Bottom)
		"../Textures/skybox/posz.jpg",  // +Z (Front)
		"../Textures/skybox/negz.jpg"   // -Z (Back)
	};

	skyBox = std::shared_ptr<SkyBox>(new SkyBox(filePaths, vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), SkyBoxDeleter);
	
	
	fullScreenQuad = std::shared_ptr<FullScreenQuad>(new FullScreenQuad(bufferManger.get(), vulkanContext.get(), commandPool), FullScreenQuadDeleter);

	lights.reserve(0);

	for (int i = 0; i < 1; i++) {
		std::shared_ptr<Light> light = std::shared_ptr<Light>(new Light(vulkanContext.get(), commandPool, camera.get(), bufferManger.get()), LightDeleter);
		lights.push_back(std::move(light));
	}

	//createTLAS();
	createDescriptorPool();

	createCommandBuffer();
	CreateGraphicsPipeline();
	ViewPortImageData = userinterface->CreateViewPortRenderTexture(1920, 1080);

	createDepthTextureImage();
	createGBuffer();
}


void App::createDescriptorPool()
{
	vk::DescriptorPoolSize Uniformpoolsize;
	Uniformpoolsize.type = vk::DescriptorType::eUniformBuffer;
	Uniformpoolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) *  1000;

	vk::DescriptorPoolSize Samplerpoolsize;
	Samplerpoolsize.type = vk::DescriptorType::eCombinedImageSampler;
	Samplerpoolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 1000;

	std::array<	vk::DescriptorPoolSize, 2> poolSizes{ Uniformpoolsize ,Samplerpoolsize };

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

void App::createTLAS()
{
		vk::TransformMatrixKHR transformMatrix{};
		glm::mat4 glmMatrix = Models[0]->GetModelMatrix();

		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				transformMatrix.matrix[i][j] = glmMatrix[j][i]; // transpose access
			}
		}

		vk::AccelerationStructureInstanceKHR instance{};
		instance.transform = transformMatrix;
		instance.instanceCustomIndex = 0;
		instance.mask = 0xFF;
		instance.instanceShaderBindingTableRecordOffset = 0;
		instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		instance.accelerationStructureReference = Models[0]->GetBLASAddressInfo();


	VkDeviceSize InstanceBufferSize = sizeof(instance);

	BufferData instancingBuffer = bufferManger->CreateBuffer(InstanceBufferSize,
		vk::BufferUsageFlagBits::eShaderDeviceAddress |
		vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
		commandPool, vulkanContext->graphicsQueue);;

	vk::BufferDeviceAddressInfoKHR InstanceBufferAddressInfo{};
	InstanceBufferAddressInfo.buffer = instancingBuffer.buffer;

	vk::DeviceAddress InstanceAddress = vulkanContext->LogicalDevice.getBufferAddress(&InstanceBufferAddressInfo);

	vk::AccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.geometryType = vk::GeometryTypeKHR::eInstances;
	accelerationStructureGeometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
	accelerationStructureGeometry.geometry.instances.sType = vk::StructureType::eAccelerationStructureGeometryInstancesDataKHR;
	accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	accelerationStructureGeometry.geometry.instances.data = InstanceAddress;


	vk::AccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
	accelerationStructureBuildGeometryInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
	accelerationStructureBuildGeometryInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
	accelerationStructureBuildGeometryInfo.geometryCount = 1;
	accelerationStructureBuildGeometryInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
	accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

	uint32_t primitive_count = 1;

	vk::AccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};

	vulkanContext->vkGetAccelerationStructureBuildSizesKHR(
		vulkanContext->LogicalDevice,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		(VkAccelerationStructureBuildGeometryInfoKHR*)&accelerationStructureBuildGeometryInfo,
		&primitive_count,
		(VkAccelerationStructureBuildSizesInfoKHR*)&accelerationStructureBuildSizesInfo);


	BufferData TlasBuffer = bufferManger->CreateBuffer(accelerationStructureBuildSizesInfo.accelerationStructureSize,
		vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
		vk::BufferUsageFlagBits::eShaderDeviceAddress,
		commandPool, vulkanContext->graphicsQueue);


	vk::AccelerationStructureCreateInfoKHR    accelerationStructureCreate_info{};
	accelerationStructureCreate_info.buffer = TlasBuffer.buffer;
	accelerationStructureCreate_info.size   = TlasBuffer.size;
	accelerationStructureCreate_info.type   = vk::AccelerationStructureTypeKHR::eTopLevel;

	vk::AccelerationStructureKHR TopLevelAS;

	vulkanContext->vkCreateAccelerationStructureKHR(vulkanContext->LogicalDevice, (VkAccelerationStructureCreateInfoKHR*)&accelerationStructureCreate_info, nullptr, (VkAccelerationStructureKHR*)&TopLevelAS);


	BufferData scratchBuffer = bufferManger->CreateBuffer(accelerationStructureBuildSizesInfo.buildScratchSize,
		vk::BufferUsageFlagBits::eStorageBuffer |
		vk::BufferUsageFlagBits::eShaderDeviceAddress,
		commandPool, vulkanContext->graphicsQueue);

	vk::BufferDeviceAddressInfoKHR StorageBufferAddressInfo{};
	StorageBufferAddressInfo.buffer = scratchBuffer.buffer;

	vk::DeviceAddress  StorageAddress = vulkanContext->LogicalDevice.getBufferAddress(&StorageBufferAddressInfo);

	vk::AccelerationStructureBuildGeometryInfoKHR acceleraitonBuildGeometryInfo{};
	acceleraitonBuildGeometryInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
	acceleraitonBuildGeometryInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
	acceleraitonBuildGeometryInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
	acceleraitonBuildGeometryInfo.dstAccelerationStructure = TopLevelAS;
	acceleraitonBuildGeometryInfo.geometryCount = 1;
	acceleraitonBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
	acceleraitonBuildGeometryInfo.scratchData.deviceAddress = StorageAddress;

	vk::AccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
	accelerationStructureBuildRangeInfo.primitiveCount = 1;
	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
	accelerationStructureBuildRangeInfo.firstVertex = 0;
	accelerationStructureBuildRangeInfo.transformOffset = 0;

	std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

	vk::CommandBuffer commandbuffer = bufferManger->CreateSingleUseCommandBuffer(commandPool);

	vulkanContext->vkCmdBuildAccelerationStructuresKHR(
		commandbuffer, 1,
		reinterpret_cast<const VkAccelerationStructureBuildGeometryInfoKHR*>(&acceleraitonBuildGeometryInfo),
		reinterpret_cast<const VkAccelerationStructureBuildRangeInfoKHR* const*>(accelerationBuildStructureRangeInfos.data()));


	bufferManger->SubmitAndDestoyCommandBuffer(commandPool, commandbuffer, vulkanContext->graphicsQueue);

}

void App::createDepthTextureImage()
{

	DepthTextureData = bufferManger->CreateImage(userinterface->GetRenderTextureExtent(), vulkanContext->FindCompatableDepthFormat(), vk::ImageUsageFlagBits::eDepthStencilAttachment);
	DepthTextureData.imageView = bufferManger->CreateImageView(DepthTextureData.image, vulkanContext->FindCompatableDepthFormat(), vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil);

	vk::CommandBuffer commandBuffer = bufferManger->CreateSingleUseCommandBuffer(commandPool);

	ImageTransitionData DataToTransitionInfo;
	DataToTransitionInfo.oldlayout = vk::ImageLayout::eUndefined;
	DataToTransitionInfo.newlayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	DataToTransitionInfo.AspectFlag = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
	//////////////////////////////////////////////////////////////////////////////
	DataToTransitionInfo.SourceAccessflag = vk::AccessFlagBits::eNone;
	DataToTransitionInfo.DestinationAccessflag = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	///////////////////////////////////////////////////////////////////////////////
	DataToTransitionInfo.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	DataToTransitionInfo.DestinationOnThePipeline = vk::PipelineStageFlagBits::eEarlyFragmentTests;

	bufferManger->TransitionImage(commandBuffer, DepthTextureData.image, DataToTransitionInfo);

	bufferManger->SubmitAndDestoyCommandBuffer(commandPool, commandBuffer, vulkanContext->graphicsQueue);

}


void App::createGBuffer()
{

	gbuffer.Position = bufferManger->CreateImage(userinterface->GetRenderTextureExtent(), vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.Position.imageView = bufferManger->CreateImageView(gbuffer.Position.image, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	gbuffer.Position.imageSampler = bufferManger->CreateImageSampler();

	gbuffer.Normal = bufferManger->CreateImage(userinterface->GetRenderTextureExtent(), vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.Normal.imageView = bufferManger->CreateImageView(gbuffer.Normal.image, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	gbuffer.Normal.imageSampler = bufferManger->CreateImageSampler();

	gbuffer.Albedo = bufferManger->CreateImage(userinterface->GetRenderTextureExtent(), vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.Albedo.imageView = bufferManger->CreateImageView(gbuffer.Albedo.image, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
	gbuffer.Albedo.imageSampler = bufferManger->CreateImageSampler();

	LightingPassImageData = bufferManger->CreateImage(userinterface->GetRenderTextureExtent(), vulkanContext->swapchainformat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	LightingPassImageData.imageView = bufferManger->CreateImageView(LightingPassImageData.image, vulkanContext->swapchainformat, vk::ImageAspectFlagBits::eColor);
	LightingPassImageData.imageSampler = bufferManger->CreateImageSampler();

	fullScreenQuad->createDescriptorSetsBasedOnGBuffer(DescriptorPool, gbuffer);
	vk::CommandBuffer commandBuffer = bufferManger->CreateSingleUseCommandBuffer(commandPool);

	ImageTransitionData transitionInfo{};
	transitionInfo.oldlayout = vk::ImageLayout::eUndefined;
	transitionInfo.newlayout = vk::ImageLayout::eColorAttachmentOptimal;
	transitionInfo.AspectFlag = vk::ImageAspectFlagBits::eColor;
	transitionInfo.SourceAccessflag = vk::AccessFlagBits::eNone;
	transitionInfo.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
	transitionInfo.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	transitionInfo.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	bufferManger->TransitionImage(commandBuffer, gbuffer.Position.image, transitionInfo);
	bufferManger->TransitionImage(commandBuffer, gbuffer.Normal.image, transitionInfo);
	bufferManger->TransitionImage(commandBuffer, gbuffer.Albedo.image, transitionInfo);
	bufferManger->TransitionImage(commandBuffer, LightingPassImageData.image, transitionInfo);

	bufferManger->SubmitAndDestoyCommandBuffer(commandPool, commandBuffer, vulkanContext->graphicsQueue);


	FinalRenderTextureId = ImGui_ImplVulkan_AddTexture(LightingPassImageData.imageSampler,
		                                          LightingPassImageData.imageView,
		                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	PositionRenderTextureId = ImGui_ImplVulkan_AddTexture(gbuffer.Position.imageSampler,
		                                          gbuffer.Position.imageView,
		                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	NormalTextureId = ImGui_ImplVulkan_AddTexture(gbuffer.Normal.imageSampler,
		                                          gbuffer.Normal.imageView,
		                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


	AlbedoTextureId = ImGui_ImplVulkan_AddTexture(gbuffer.Albedo.imageSampler,
		                                          gbuffer.Albedo.imageView,
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
	};

	vk::PipelineDynamicStateCreateInfo DynamicState{};
	DynamicState.dynamicStateCount = static_cast<uint32_t>(DynamicStates.size());
	DynamicState.pDynamicStates = DynamicStates.data();


    {
    	auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/DefferedLightingPass.vert.spv");
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
    	depthStencilState.depthCompareOp = vk::CompareOp::eLess;
    	depthStencilState.minDepthBounds = 0.0f;
    	depthStencilState.maxDepthBounds = 1.0f;
    	depthStencilState.stencilTestEnable = VK_FALSE;

		vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanContext->swapchainformat;

    	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    	pipelineLayoutInfo.setLayoutCount = 1;
    	pipelineLayoutInfo.setSetLayouts(fullScreenQuad->descriptorSetLayout);
    	pipelineLayoutInfo.pushConstantRangeCount = 0;
    	pipelineLayoutInfo.pPushConstantRanges = nullptr;
    
		FullScreenQuadgraphicsPipelineLayout = vulkanContext->LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);
    
		FullScreenQuadgraphicsPipeline = vulkanContext->createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, vertexInputInfo, inputAssembleInfo,
    		                                                     viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, FullScreenQuadgraphicsPipelineLayout);
    
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
		                                        depthStencilState.depthCompareOp = vk::CompareOp::eLess;
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
		
		LightgraphicsPipeline = vulkanContext->createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, vertexInputInfo, inputAssembleInfo,
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
		  										depthStencilState.depthTestEnable = vk::False;
												depthStencilState.depthWriteEnable = vk::False;
												depthStencilState.depthCompareOp = vk::CompareOp::eLess;
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

		 SkyBoxgraphicsPipeline =  vulkanContext->createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, vertexInputInfo, inputAssembleInfo,
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
		depthStencilState.depthCompareOp = vk::CompareOp::eLess;
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

		std::array<vk::Format, 3> colorFormats = {
	                             vk::Format::eR16G16B16A16Sfloat, // Position
	                             vk::Format::eR16G16B16A16Sfloat, // Normal
	                             vk::Format::eR8G8B8A8Unorm       // Albedo
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

		std::array<vk::PipelineColorBlendAttachmentState, 3> colorBlendAttachments = {
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
				.setBlendEnable(VK_FALSE)
		};

		vk::PipelineColorBlendStateCreateInfo colorBlend{};
		colorBlend.setLogicOpEnable(VK_FALSE);
		colorBlend.setAttachmentCount(colorBlendAttachments.size());
		colorBlend.setPAttachments(colorBlendAttachments.data());

		geometryPassPipelineLayout = vulkanContext->LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

		geometryPassPipeline = vulkanContext->createGraphicsPipeline(pipelineRenderingCreateInfo, ShaderStages, vertexInputInfo, inputAssembleInfo,
			viewportState, rasterizerinfo, multisampling, depthStencilState, colorBlend, DynamicState, geometryPassPipelineLayout);

		vulkanContext->LogicalDevice.destroyShaderModule(VertShaderModule);
		vulkanContext->LogicalDevice.destroyShaderModule(FragShaderModule);
	}

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


	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);


	vk::SemaphoreCreateInfo semaphoreInfo{};

	vk::FenceCreateInfo fenceInfo{};
	fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vulkanContext->LogicalDevice.createSemaphore(&semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != vk::Result::eSuccess) {
			throw std::runtime_error("failed to create image available semaphore!");
		}


		if (vulkanContext->LogicalDevice.createSemaphore(&semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != vk::Result::eSuccess) {
			throw std::runtime_error("failed to create render finished semaphore!");
		}

		if (vulkanContext->LogicalDevice.createFence(&fenceInfo, nullptr, &inFlightFences[i]) != vk::Result::eSuccess) {
			throw std::runtime_error("failed to create fence!");
		}

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

		//ZoneScopedN("UI");
		if (bRecreateDepth)
		{
			destroy_DepthImage();
			createDepthTextureImage();
			bRecreateDepth = false;
		}

		userinterface->DrawUi(bRecreateDepth, DefferedDecider, FinalRenderTextureId, PositionRenderTextureId, 
			                                                   NormalTextureId, AlbedoTextureId, camera.get(), Models, lights);
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
	//ZoneScopedN("render");
	if (vulkanContext->LogicalDevice.waitForFences(1, &inFlightFences[currentFrame], vk::True, UINT64_MAX) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to wait for fence");

	}

	uint32_t imageIndex;

	try {
		vk::Result result = vulkanContext->LogicalDevice.acquireNextImageKHR(vulkanContext->swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], nullptr, &imageIndex);

	}
	catch (const std::exception& e) {
		recreateSwapChain();
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	updateUniformBuffer(currentFrame);
	recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

	vulkanContext->LogicalDevice.resetFences(1, &inFlightFences[currentFrame]);
	
	 // Step 5: Set up synchronization for rendering
     // The GPU will wait for the imageAvailableSemaphore to be signaled before starting rendering.
    // It will wait at the eColorAttachmentOutput stage, which is where color attachment writes occur
	vk::Semaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame]};
	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

	// The GPU will signal the renderFinishedSemaphore when rendering is complete.	
	vk::Semaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame]};

	vk::SubmitInfo submitInfo{};
	submitInfo.sType = vk::StructureType::eSubmitInfo;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;


	// The inFlightFence will be signaled when the GPU is done with the command buffer.
		if (vulkanContext->graphicsQueue.submit(1, &submitInfo, inFlightFences[currentFrame]) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to submit info");

	}

	vk::SwapchainKHR swapChains[] = { vulkanContext->swapChain };
	vk::PresentInfoKHR presentInfo{};
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	
	//wait on renderFinishedSemaphore before this is ran
   try {
	   vk::Result result = vulkanContext->presentQueue.presentKHR(presentInfo);
   }
   catch (const std::exception& e) {
	   std::cerr << "Exception: " << e.what() << std::endl;
	   std::cerr << "Attempting to recreate swap chain..." << std::endl;
	   recreateSwapChain();
	   framebufferResized = false;

   }

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void App::updateUniformBuffer(uint32_t currentImage) {
	
	for (auto& light : lights)
	{
		light->UpdateUniformBuffer(currentImage, nullptr);
	}


	for (auto& model : Models)
	{
		model->UpdateUniformBuffer(currentImage, lights[0].get());
	}

	skyBox->UpdateUniformBuffer(currentImage,lights[0].get());

	fullScreenQuad->UpdateUniformBuffer(currentImage, lights);
}

void  App::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) {

	vk::ClearValue clearColor{};
	clearColor.color = { 0.0f, 0.0f, 0.0f, 1.0f };

	VkOffset2D imageoffset = { 0, 0 };

	vk::Viewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(userinterface->GetRenderTextureExtent().width);
	viewport.height = static_cast<float>(userinterface->GetRenderTextureExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor{};
	scissor.offset = imageoffset;
	scissor.extent.width = userinterface->GetRenderTextureExtent().width;
	scissor.extent.height = userinterface->GetRenderTextureExtent().height;

	vk::DeviceSize offsets[] = { 0 };

	ImageTransitionData TransitionColorAttachmentOptimal{};
	TransitionColorAttachmentOptimal.oldlayout = vk::ImageLayout::eUndefined;
	TransitionColorAttachmentOptimal.newlayout = vk::ImageLayout::eColorAttachmentOptimal;
	TransitionColorAttachmentOptimal.AspectFlag = vk::ImageAspectFlagBits::eColor;
	TransitionColorAttachmentOptimal.SourceAccessflag = vk::AccessFlagBits::eNone;
	TransitionColorAttachmentOptimal.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
	TransitionColorAttachmentOptimal.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	TransitionColorAttachmentOptimal.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	
	vk::CommandBufferBeginInfo begininfo;
	                          begininfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	commandBuffer.begin(begininfo);
	bufferManger->TransitionImage(commandBuffer, gbuffer.Position.image, TransitionColorAttachmentOptimal);
	bufferManger->TransitionImage(commandBuffer, gbuffer.Normal.image, TransitionColorAttachmentOptimal);
	bufferManger->TransitionImage(commandBuffer, gbuffer.Albedo.image, TransitionColorAttachmentOptimal);
	bufferManger->TransitionImage(commandBuffer, LightingPassImageData.image, TransitionColorAttachmentOptimal);


	{
		vk::RenderingAttachmentInfoKHR PositioncolorAttachmentInfo{};
		PositioncolorAttachmentInfo.imageView = gbuffer.Position.imageView;
		PositioncolorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		PositioncolorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		PositioncolorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		PositioncolorAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfoKHR NormalcolorAttachmentInfo{};
		NormalcolorAttachmentInfo.imageView = gbuffer.Normal.imageView;
		NormalcolorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		NormalcolorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		NormalcolorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		NormalcolorAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfoKHR AlbedocolorAttachmentInfo{};
		AlbedocolorAttachmentInfo.imageView = gbuffer.Albedo.imageView;
		AlbedocolorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		AlbedocolorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		AlbedocolorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		AlbedocolorAttachmentInfo.clearValue = clearColor;
		
		std::array<vk::RenderingAttachmentInfoKHR, 3> ColorAttachments{ PositioncolorAttachmentInfo ,NormalcolorAttachmentInfo,AlbedocolorAttachmentInfo };

		vk::RenderingAttachmentInfo depthStencilAttachment;
		depthStencilAttachment.imageView = DepthTextureData.imageView;
		depthStencilAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		depthStencilAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		depthStencilAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		depthStencilAttachment.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

		vk::RenderingInfoKHR renderingInfo{};
		renderingInfo.renderArea.offset = imageoffset;
		renderingInfo.renderArea.extent.height = userinterface->GetRenderTextureExtent().height;
		renderingInfo.renderArea.extent.width = userinterface->GetRenderTextureExtent().width;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 3;
		renderingInfo.pColorAttachments = ColorAttachments.data();
		renderingInfo.pDepthAttachment = &depthStencilAttachment;


		commandBuffer.beginRendering(renderingInfo);
		commandBuffer.setViewport(0, 1, &viewport);
		commandBuffer.setScissor(0, 1, &scissor);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, geometryPassPipeline);

		for (auto& model : Models)
		{
			model->Draw(commandBuffer, geometryPassPipelineLayout, currentFrame);
		}
		commandBuffer.endRendering();

		{
			ImageTransitionData gbufferTransition{};
			gbufferTransition.oldlayout = vk::ImageLayout::eColorAttachmentOptimal;
			gbufferTransition.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			gbufferTransition.AspectFlag = vk::ImageAspectFlagBits::eColor;
			gbufferTransition.SourceAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
			gbufferTransition.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
			gbufferTransition.SourceOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			gbufferTransition.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

			bufferManger->TransitionImage(commandBuffer, gbuffer.Position.image, gbufferTransition);
			bufferManger->TransitionImage(commandBuffer, gbuffer.Normal.image, gbufferTransition);
			bufferManger->TransitionImage(commandBuffer, gbuffer.Albedo.image, gbufferTransition);

			vk::RenderingAttachmentInfoKHR LightPassColorAttachmentInfo{};
			LightPassColorAttachmentInfo.imageView = LightingPassImageData.imageView;
			LightPassColorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
			LightPassColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
			LightPassColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
			LightPassColorAttachmentInfo.clearValue = clearColor;


			vk::RenderingInfoKHR renderingInfo{};
			renderingInfo.renderArea.offset = imageoffset;
			renderingInfo.renderArea.extent.height = userinterface->GetRenderTextureExtent().height;
			renderingInfo.renderArea.extent.width = userinterface->GetRenderTextureExtent().width;
			renderingInfo.layerCount = 1;
			renderingInfo.colorAttachmentCount = 1;
			renderingInfo.pColorAttachments = &LightPassColorAttachmentInfo;


			commandBuffer.beginRendering(renderingInfo);

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, FullScreenQuadgraphicsPipeline);
			fullScreenQuad->Draw(commandBuffer, FullScreenQuadgraphicsPipelineLayout, currentFrame);

			commandBuffer.endRendering();

			ImageTransitionData TransitionBacktoColorOutput{};
			TransitionBacktoColorOutput.oldlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			TransitionBacktoColorOutput.newlayout = vk::ImageLayout::eColorAttachmentOptimal;
			TransitionBacktoColorOutput.AspectFlag = vk::ImageAspectFlagBits::eColor;
			TransitionBacktoColorOutput.SourceAccessflag = vk::AccessFlagBits::eShaderRead;
			TransitionBacktoColorOutput.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
			TransitionBacktoColorOutput.SourceOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;
			TransitionBacktoColorOutput.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;

			bufferManger->TransitionImage(commandBuffer, gbuffer.Position.image, TransitionBacktoColorOutput);
			bufferManger->TransitionImage(commandBuffer, gbuffer.Normal.image, TransitionBacktoColorOutput);
			bufferManger->TransitionImage(commandBuffer, gbuffer.Albedo.image, TransitionBacktoColorOutput);

		}

		{
			vk::RenderingAttachmentInfoKHR LightPassColorAttachmentInfo{};
			LightPassColorAttachmentInfo.imageView = LightingPassImageData.imageView;
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

			vk::RenderingInfoKHR renderingInfo{};
			renderingInfo.renderArea.offset = imageoffset;
			renderingInfo.renderArea.extent.height = userinterface->GetRenderTextureExtent().height;
			renderingInfo.renderArea.extent.width = userinterface->GetRenderTextureExtent().width;
			renderingInfo.layerCount = 1;
			renderingInfo.colorAttachmentCount = 1;
			renderingInfo.pColorAttachments = &LightPassColorAttachmentInfo;
			renderingInfo.pDepthAttachment = &depthStencilAttachment;

			commandBuffer.beginRendering(renderingInfo);

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, LightgraphicsPipeline);

			for (auto& light : lights)
			{
				light->Draw(commandBuffer, LightpipelineLayout, currentFrame);
			}
			commandBuffer.endRendering();
		}
	}



	ImageTransitionData TransitiontoShader{};
	TransitiontoShader.oldlayout = vk::ImageLayout::eColorAttachmentOptimal;
	TransitiontoShader.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	TransitiontoShader.AspectFlag = vk::ImageAspectFlagBits::eColor;
	TransitiontoShader.SourceAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
	TransitiontoShader.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentRead;
	TransitiontoShader.SourceOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	TransitiontoShader.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

	if (DefferedDecider == 1)
	{
		bufferManger->TransitionImage(commandBuffer, LightingPassImageData.image, TransitiontoShader);
	}
	if (DefferedDecider == 2)
	{
		bufferManger->TransitionImage(commandBuffer, gbuffer.Position.image, TransitiontoShader);
	}
	if (DefferedDecider == 3)
	{
		bufferManger->TransitionImage(commandBuffer, gbuffer.Position.image, TransitiontoShader);
	}

	if (DefferedDecider == 4)
	{
		bufferManger->TransitionImage(commandBuffer, gbuffer.Position.image, TransitiontoShader);
	}

	userinterface->RenderUi(commandBuffer, imageIndex);
}

void App::destroy_DepthImage()
{
	bufferManger->DestroyImage(DepthTextureData);
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

	vulkanContext->create_swapchain();


	camera->SetSwapChainHeight(vulkanContext->swapchainExtent.height);
	camera->SetSwapChainWidth(vulkanContext->swapchainExtent.width);
}

void App::DestroySyncObjects()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vulkanContext->LogicalDevice.destroySemaphore(imageAvailableSemaphores[i]);
		vulkanContext->LogicalDevice.destroySemaphore(renderFinishedSemaphores[i]);
		vulkanContext->LogicalDevice.destroyFence(inFlightFences[i]);
	}
}

void App::DestroyBuffers()
{
	for (auto& model : Models)
	{
		model.reset();
	}

	for (auto& light : lights)
	{
		light.reset();
	}

	skyBox.reset();

	fullScreenQuad.reset();
	bufferManger->DestroyImage(DepthTextureData);
	bufferManger->DestroyImage(LightingPassImageData);
	bufferManger->DestroyImage(gbuffer.Position);
	bufferManger->DestroyImage(gbuffer.Albedo);
	bufferManger->DestroyImage(gbuffer.Normal);
	bufferManger.reset();
}


 App::~App()
{
	userinterface.reset();
	DestroyBuffers();

	if (!commandBuffers.empty()) {
		vulkanContext->LogicalDevice.freeCommandBuffers(commandPool, commandBuffers);
		commandBuffers.clear();
	}

	vulkanContext->LogicalDevice.destroyCommandPool(commandPool);
	vulkanContext->LogicalDevice.destroyPipeline(FullScreenQuadgraphicsPipeline);
	vulkanContext->LogicalDevice.destroyPipeline(SkyBoxgraphicsPipeline);
	vulkanContext->LogicalDevice.destroyPipeline(LightgraphicsPipeline);
	vulkanContext->LogicalDevice.destroyPipeline(geometryPassPipeline);

	vulkanContext->LogicalDevice.destroyDescriptorPool(DescriptorPool);
	vulkanContext->LogicalDevice.destroyPipelineLayout(FullScreenQuadgraphicsPipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(SkyBoxpipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(LightpipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(geometryPassPipelineLayout);

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

