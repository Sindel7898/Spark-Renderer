#define _CRTDBG_MAP_ALLOC
#include "App.h"
#include <optional>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Window.h"
#include "Camera.h"
#include "MeshLoader.h"
#include "BufferManager.h"
#include "VulkanContext.h"
#include "FramesPerSecondCounter.h"
#include <crtdbg.h>


#define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)

 App::App()
{
	window = std::make_unique<Window>(1920, 1080, "Vulkan Window");

	vulkanContext = std::make_shared<VulkanContext>(*window);
	bufferManger = std::make_shared<BufferManager>(vulkanContext->LogicalDevice, vulkanContext->PhysicalDevice, vulkanContext->VulkanInstance);
	camera = std::make_shared<Camera>(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, window->GetWindow());
	//glfwSetFramebufferSizeCallback(window->GetWindow(),);
	glfwSetWindowUserPointer(window->GetWindow(), this);

	createSyncObjects();	
	//////////////////////////
	createCommandPool();

	Models.reserve(3);

	for (int i = 0; i < 3; i++) {
		auto model = std::unique_ptr<Model, ModelDeleter>(new Model("../Textures/Helmet/model.obj", vulkanContext.get(), commandPool, camera.get(), bufferManger.get()));
		     model->LoadTextures("../Textures/Helmet/RGBDefault_albedo.jpeg");
			 Models.push_back(std::move(model));
	}

	std::array<const char*, 6> filePaths{
		"../Textures/skybox/right.jpg",  // +X (Right)
		"../Textures/skybox/left.jpg",  // -X (Left)
		"../Textures/skybox/top.jpg",  // +Y (Top)
		"../Textures/skybox/bottom.jpg",  // -Y (Bottom)
		"../Textures/skybox/front.jpg",  // +Z (Front)
		"../Textures/skybox/back.jpg"   // -Z (Back)
	};

	skyBox = std::unique_ptr<SkyBox, SkyBoxDeleter>(new SkyBox(filePaths, vulkanContext.get(), commandPool, camera.get(), bufferManger.get()));

	createDescriptorPool();

	createCommandBuffer();
	CreateGraphicsPipeline();
	CreateSkyboxGraphicsPipeline();
	/////////////////////////////////

	InitImgui();
	CreateImguiViewPortRenderTexture();
	createDepthTextureImage();

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

}

void App::CreateImguiViewPortRenderTexture()
{
	RenderTextureExtent = vk::Extent3D(500, 500, 1);
	ImguiViewPortRenderTextureData = bufferManger->CreateImage(RenderTextureExtent,vulkanContext->swapchainformat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	ImguiViewPortRenderTextureData.imageView = bufferManger->CreateImageView(ImguiViewPortRenderTextureData.image, vulkanContext->swapchainformat,vk::ImageAspectFlagBits::eColor);
	ImguiViewPortRenderTextureData.imageSampler = bufferManger->CreateImageSampler();

	RenderTextureId = ImGui_ImplVulkan_AddTexture(ImguiViewPortRenderTextureData.imageSampler,
		ImguiViewPortRenderTextureData.imageView,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void App::ReCreateImguiViewPortRenderTexture(uint32_t X, uint32_t Y)
{
	RenderTextureExtent = vk::Extent3D(X, Y, 1);
	ImguiViewPortRenderTextureData = bufferManger->CreateImage(RenderTextureExtent, vulkanContext->swapchainformat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	ImguiViewPortRenderTextureData.imageView = bufferManger->CreateImageView(ImguiViewPortRenderTextureData.image, vulkanContext->swapchainformat, vk::ImageAspectFlagBits::eColor);
	ImguiViewPortRenderTextureData.imageSampler = bufferManger->CreateImageSampler();

	RenderTextureId = ImGui_ImplVulkan_AddTexture(ImguiViewPortRenderTextureData.imageSampler,
		ImguiViewPortRenderTextureData.imageView,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}


void App::createDepthTextureImage()
{
	DepthTextureData = bufferManger->CreateImage(RenderTextureExtent, vulkanContext->FindCompatableDepthFormat(), vk::ImageUsageFlagBits::eDepthStencilAttachment);
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

void App::CreateGraphicsPipeline() 
{
	//Read Vertex & Fragment shader  and create modules
	auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/Simple_Shader.vert.spv");
	auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/Simple_Shader.frag.spv");
	
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
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	auto BindDesctiptions = ModelVertex::GetBindingDescription();
	auto attributeDescriptions = ModelVertex::GetAttributeDescription();

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.setVertexBindingDescriptionCount(1);
	vertexInputInfo.setVertexAttributeDescriptionCount(2);
	vertexInputInfo.setPVertexBindingDescriptions(&BindDesctiptions);
	vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	vk::PipelineInputAssemblyStateCreateInfo inputAssembleInfo{};
	inputAssembleInfo.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembleInfo.primitiveRestartEnable = vk::False;

	//Create ViewPort and scissor
	vk::Viewport viewport{};
	viewport.setX(0.0f);
	viewport.setY(0.0f);
	viewport.setHeight((float)vulkanContext->swapchainExtent.height);
	viewport.setWidth ((float)vulkanContext->swapchainExtent.width );
	viewport.setMinDepth(0.0f);
	viewport.setMaxDepth(1.0f);

	vk::Offset2D scissorOffset = { 0,0 };

	vk::Rect2D scissor{};
	scissor.setOffset(scissorOffset);
	scissor.setExtent(vulkanContext->swapchainExtent);


	// set states that can be dynamic without having to recreate the whole pipeline
	std::vector<vk::DynamicState> DynamicStates = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
	};
	
	vk::PipelineDynamicStateCreateInfo DynamicState{};
	DynamicState.dynamicStateCount = static_cast<uint32_t>(DynamicStates.size());
	DynamicState.pDynamicStates = DynamicStates.data();
	/////////////////////////////////////////////////////////////////////////////

	vk::PipelineViewportStateCreateInfo viewportState{};
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

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.setLayoutCount = 1; 
	pipelineLayoutInfo.setSetLayouts(Models[1]->descriptorSetLayout); 
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
 
	pipelineLayout = vulkanContext->LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

	vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
	pipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanContext->swapchainformat;
	pipelineRenderingCreateInfo.depthAttachmentFormat = vulkanContext->FindCompatableDepthFormat();

	vk::PipelineDepthStencilStateCreateInfo depthStencilState;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = vk::CompareOp::eLess;
	depthStencilState.minDepthBounds = 0.0f;
	depthStencilState.maxDepthBounds = 1.0f;
	depthStencilState.stencilTestEnable = VK_FALSE;

	//depthStencilState.front.compareOp = vk::CompareOp::eAlways;
	//depthStencilState.front.failOp = vk::StencilOp::eKeep; 
	//depthStencilState.front.depthFailOp = vk::StencilOp::eIncrementAndClamp; 
	//depthStencilState.front.passOp = vk::StencilOp::eKeep; // Keep the stencil value on pass
	// Stencil operations for back-facing polygons
	//depthStencilState.back = depthStencilState.front; // Use the same operations for back faces

	vk::GraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.pNext = &pipelineRenderingCreateInfo; // Add this line
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = ShaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembleInfo;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizerinfo;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencilState;
	pipelineInfo.pColorBlendState = &colorBlend;
	pipelineInfo.pDynamicState = &DynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = VK_NULL_HANDLE;
	pipelineInfo.subpass = 0;


	vk::Result result = vulkanContext->LogicalDevice.createGraphicsPipelines(nullptr, 1, &pipelineInfo, nullptr, &graphicsPipeline);

	if (result != vk::Result::eSuccess)
	   {
			throw std::runtime_error("failed to create graphics pipeline!");
		}

	vulkanContext->LogicalDevice.destroyShaderModule(VertShaderModule);
	vulkanContext->LogicalDevice.destroyShaderModule(FragShaderModule);
}

void App::CreateSkyboxGraphicsPipeline()
{
	//Read Vertex & Fragment shader  and create modules
	auto VertShaderCode = readFile("../Shaders/Compiled_Shader_Files/SkyBox_Shader.vert.spv");
	auto FragShaderCode = readFile("../Shaders/Compiled_Shader_Files/SkyBox_Shader.frag.spv");

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
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	auto BindDesctiptions = SkyBoxVertex::GetBindingDescription();
	auto attributeDescriptions = SkyBoxVertex::GetAttributeDescription();

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.setVertexBindingDescriptionCount(1);
	vertexInputInfo.setVertexAttributeDescriptionCount(1);
	vertexInputInfo.setPVertexBindingDescriptions(&BindDesctiptions);
	vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	vk::PipelineInputAssemblyStateCreateInfo inputAssembleInfo{};
	inputAssembleInfo.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembleInfo.primitiveRestartEnable = vk::False;

	//Create ViewPort and scissor
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


	// set states that can be dynamic without having to recreate the whole pipeline
	std::vector<vk::DynamicState> DynamicStates = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
	};

	vk::PipelineDynamicStateCreateInfo DynamicState{};
	DynamicState.dynamicStateCount = static_cast<uint32_t>(DynamicStates.size());
	DynamicState.pDynamicStates = DynamicStates.data();
	/////////////////////////////////////////////////////////////////////////////

	vk::PipelineViewportStateCreateInfo viewportState{};
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
	rasterizerinfo.frontFace = vk::FrontFace::eClockwise;
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

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &skyBox->descriptorSetLayout; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	SkyBoxpipelineLayout = vulkanContext->LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

	vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
	pipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanContext->swapchainformat;
	pipelineRenderingCreateInfo.depthAttachmentFormat = vulkanContext->FindCompatableDepthFormat();

	vk::PipelineDepthStencilStateCreateInfo depthStencilState;
	depthStencilState.depthTestEnable = vk::False;
	depthStencilState.depthWriteEnable = vk::False;
	depthStencilState.depthCompareOp = vk::CompareOp::eLess;
	depthStencilState.minDepthBounds = 0.0f;
	depthStencilState.maxDepthBounds = 1.0f;
	depthStencilState.stencilTestEnable = VK_FALSE;

	vk::GraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.pNext = &pipelineRenderingCreateInfo; 
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = ShaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembleInfo;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizerinfo;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencilState;
	pipelineInfo.pColorBlendState = &colorBlend;
	pipelineInfo.pDynamicState = &DynamicState;
	pipelineInfo.layout = SkyBoxpipelineLayout;
	pipelineInfo.renderPass = VK_NULL_HANDLE;
	pipelineInfo.subpass = 0;


	vk::Result result = vulkanContext->LogicalDevice.createGraphicsPipelines(nullptr, 1, &pipelineInfo, nullptr, &SkyBoxgraphicsPipeline);

	if (result != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vulkanContext->LogicalDevice.destroyShaderModule(VertShaderModule);
	vulkanContext->LogicalDevice.destroyShaderModule(FragShaderModule);
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

void App::InitImgui()
{
	//Imgui Initialisation
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
    ///////////////////////////////////////////////////////
	//Imgui Style Setup
	ImGui::StyleColorsDark();
	if (io.ConfigFlags)
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 10.0f;
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 100);
	}
    /////////////////////////////////////////////////////
	
	// Create descriptor pool for ImGui
	std::vector<vk::DescriptorPoolSize> pool_sizes =
	{
        { vk::DescriptorType::eSampler, 1000 },
	    { vk::DescriptorType::eCombinedImageSampler, 1000 },
	    { vk::DescriptorType::eSampledImage, 1000 },
	    { vk::DescriptorType::eStorageImage, 1000 },
	    { vk::DescriptorType::eUniformTexelBuffer, 1000 },
	    { vk::DescriptorType::eStorageTexelBuffer, 1000 },
	    { vk::DescriptorType::eUniformBuffer, 1000 },
	    { vk::DescriptorType::eStorageBuffer, 1000 },
	    { vk::DescriptorType::eUniformBufferDynamic, 1000 },
	    { vk::DescriptorType::eStorageBufferDynamic, 1000 },
	    { vk::DescriptorType::eInputAttachment, 1000 }
	};

	vk::DescriptorPoolCreateInfo pool_info;
	pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	pool_info.pPoolSizes = pool_sizes.data();

	ImGuiDescriptorPool = vulkanContext->LogicalDevice.createDescriptorPool(pool_info);
	// Initialize ImGui for Vulkan
	vk::PipelineRenderingCreateInfoKHR pipeline_rendering_create_info;
	pipeline_rendering_create_info.colorAttachmentCount = 1;
	pipeline_rendering_create_info.pColorAttachmentFormats = &vulkanContext->swapchainformat;
	pipeline_rendering_create_info.depthAttachmentFormat = vk::Format::eUndefined;
	pipeline_rendering_create_info.stencilAttachmentFormat = vk::Format::eUndefined;

	ImGui_ImplGlfw_InitForVulkan(window->GetWindow(), true);

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = vulkanContext->VulkanInstance;
	init_info.PhysicalDevice = vulkanContext->PhysicalDevice;
	init_info.Device = vulkanContext->LogicalDevice;
	init_info.QueueFamily = vulkanContext->graphicsQueueFamilyIndex;
	init_info.Queue = vulkanContext->graphicsQueue;
	init_info.DescriptorPool = ImGuiDescriptorPool;
	init_info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
	init_info.ImageCount = vulkanContext->swapchainImages.size();
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.UseDynamicRendering = true;
	init_info.PipelineRenderingCreateInfo = pipeline_rendering_create_info;

	ImGui_ImplVulkan_Init(&init_info);
	ImGui_ImplVulkan_CreateFontsTexture();

}

void App::Run()
{
	FramesPerSecondCounter fpsCounter(0.1f);

	while (!window->shouldClose())
	{
		glfwPollEvents();
		CalculateFps(fpsCounter);
		camera->Update(deltaTime);

		// Start the ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		SetupDockingEnvironment();
		ShowImGuiDemoWindow();

		Draw();

		ImGui::Render();
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	//waits for the device to finnish all its processes before shutting down
	vulkanContext->LogicalDevice.waitIdle();
}

void App::SetupDockingEnvironment()
{
	// Get the main viewport
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	// Set up the main dockspace window
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);

	// Set up the window flags
	ImGuiWindowFlags window_flags =
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground;


	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(-0.5f, -0.5f));
	ImGui::Begin("DockSpace", nullptr, window_flags);
	ImGui::PopStyleVar();

	// Submit the DockSpace
	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	// Set up the initial layout (only once)
	static bool first_time = true;
	if (first_time)
	{
		first_time = false;

		// Clear out any existing layout
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

		// Split the dockspace
	    dock_main_id = dockspace_id;
		ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);

		// Dock windows
		ImGui::DockBuilderDockWindow("Hello, world!", dock_left_id);
		ImGui::DockBuilderDockWindow("Another Window", dock_left_id);
		ImGui::DockBuilderDockWindow("Main Viewport", dock_main_id);

		ImGui::DockBuilderFinish(dockspace_id);
	}

	ImGui::End();
}

void App::ShowImGuiDemoWindow()
{
	static bool show_another_window = true;

	{
		ImGui::Begin("Hello, world!");

		ImGuiIO& io = ImGui::GetIO();
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		ImGui::End();
	}

	if (show_another_window)
	{
		ImGui::Begin("Another Window", &show_another_window);
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}

	ImGui::SetNextWindowSize(ImVec2(400, 300)); 
	ImGui::Begin("Main Viewport", nullptr, ImGuiWindowFlags_NoBackground);
	ImGui::Text("Main ViewPort");
	ImguiViewPortRenderTextureSizeDecider();
	ImGui::End();
}

void App::ImguiViewPortRenderTextureSizeDecider()
{
	ImVec2 viewportSize;

	if (ImGui::GetMainViewport())
	{
		viewportSize = ImGui::GetContentRegionAvail();
		std::cout << "Viewport size: (" << viewportSize.x << ", " << viewportSize.y << ")" << std::endl;
	}

	static ImVec2 lastSize = ImVec2(0.0f, 0.0f);

	if (viewportSize.x != lastSize.x || viewportSize.y != lastSize.y)
	{
		if (viewportSize.x <= 5 || viewportSize.y <= 5)
		{
			viewportSize = ImVec2(5.0f, 5.0f);
		}

		vulkanContext->LogicalDevice.waitIdle();
		ImGui_ImplVulkan_RemoveTexture(RenderTextureId);
		bufferManger->DestroyImage(ImguiViewPortRenderTextureData);
		ReCreateImguiViewPortRenderTexture(static_cast<uint32_t>(viewportSize.x), static_cast<uint32_t>(viewportSize.y));
		destroy_DepthImage();
		createDepthTextureImage();
		lastSize = viewportSize;
	}

	ImGui::Image((ImTextureID)RenderTextureId, viewportSize);
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

	const float SPACING = 2.0f;
	int modelIndex = 0;

	for (int z = 0; z < 20; ++z) {
			for (int x = 0; x < 20; ++x) {

				if (modelIndex == Models.size())
				{
					break;
				}

				float offsetX = (x) * SPACING;
				float offsetZ = (z) * SPACING;

				Models[modelIndex++]->UpdateUniformBuffer(currentImage,offsetX,0,offsetZ);
			}
	}

	skyBox->UpdateUniformBuffer(currentImage);

}

void  App::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) {

	vk::ClearValue clearColor{};
	clearColor.color = { 0.0f, 0.0f, 0.0f, 1.0f };

	vk::CommandBufferBeginInfo begininfo{};
	begininfo.pInheritanceInfo = nullptr;
	begininfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	//////////////////////////////////////////////////////////////////////////
	commandBuffer.begin(begininfo);

	ImageTransitionData DataToTransitionInfo{};
	DataToTransitionInfo.oldlayout = vk::ImageLayout::eUndefined;
	DataToTransitionInfo.newlayout = vk::ImageLayout::eColorAttachmentOptimal;
	DataToTransitionInfo.AspectFlag = vk::ImageAspectFlagBits::eColor;
	DataToTransitionInfo.SourceAccessflag = vk::AccessFlagBits::eNone;
	DataToTransitionInfo.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
	DataToTransitionInfo.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	DataToTransitionInfo.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	bufferManger->TransitionImage(commandBuffer, ImguiViewPortRenderTextureData.image, DataToTransitionInfo);

	VkOffset2D imageoffset = { 0, 0 };

	vk::RenderingAttachmentInfoKHR colorAttachmentInfo{};
	colorAttachmentInfo.imageView = ImguiViewPortRenderTextureData.imageView;
	colorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	colorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachmentInfo.clearValue = clearColor;

	vk::RenderingAttachmentInfo depthStencilAttachment;
	depthStencilAttachment.imageView = DepthTextureData.imageView;
	depthStencilAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	depthStencilAttachment.loadOp = vk::AttachmentLoadOp::eClear; 
	depthStencilAttachment.storeOp = vk::AttachmentStoreOp::eDontCare; 
	depthStencilAttachment.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0); 

	vk::RenderingInfoKHR renderingInfo{};
	renderingInfo.renderArea.offset = imageoffset;
	renderingInfo.renderArea.extent.height = RenderTextureExtent.height;
	renderingInfo.renderArea.extent.width = RenderTextureExtent.width;
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachmentInfo;
	renderingInfo.pDepthAttachment = &depthStencilAttachment;

	vk::Viewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(RenderTextureExtent.width);
	viewport.height = static_cast<float>(RenderTextureExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor{};
	scissor.offset = imageoffset;
	scissor.extent.width = RenderTextureExtent.width;
	scissor.extent.height = RenderTextureExtent.height;

	vk::DeviceSize offsets[] = { 0 };

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, SkyBoxgraphicsPipeline);
	commandBuffer.beginRendering(renderingInfo);
	commandBuffer.setViewport(0, 1, &viewport);
	commandBuffer.setScissor(0, 1, &scissor);

	skyBox->Draw(commandBuffer, SkyBoxpipelineLayout, currentFrame);
    
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

	for (auto& model : Models)
	{
		model->Draw(commandBuffer, pipelineLayout, currentFrame);
	}

	commandBuffer.endRendering();
	//////////////////////////////////////////////////////////////////////////////////
	ImageTransitionData TransitiontoShaderSample{};
	TransitiontoShaderSample.oldlayout = vk::ImageLayout::eColorAttachmentOptimal;
	TransitiontoShaderSample.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	TransitiontoShaderSample.AspectFlag = vk::ImageAspectFlagBits::eColor;
	TransitiontoShaderSample.SourceAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
	TransitiontoShaderSample.SourceOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	TransitiontoShaderSample.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;
	TransitiontoShaderSample.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;

	bufferManger->TransitionImage(commandBuffer, ImguiViewPortRenderTextureData.image, TransitiontoShaderSample);


	ImageTransitionData TransitionSwapchainToWriteData;
	TransitionSwapchainToWriteData.oldlayout = vk::ImageLayout::eUndefined;
	TransitionSwapchainToWriteData.newlayout = vk::ImageLayout::eColorAttachmentOptimal;
	TransitionSwapchainToWriteData.SourceAccessflag = vk::AccessFlagBits::eNone;
	TransitionSwapchainToWriteData.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
	TransitionSwapchainToWriteData.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	TransitionSwapchainToWriteData.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	TransitionSwapchainToWriteData.AspectFlag = vk::ImageAspectFlagBits::eColor;

	bufferManger->TransitionImage(commandBuffer, vulkanContext->swapchainImages[imageIndex], TransitionSwapchainToWriteData);

	ImGui::Render();

    //// Begin rendering for ImGui
    vk::RenderingAttachmentInfoKHR imguiColorAttachment{};
    imguiColorAttachment.imageView = vulkanContext->swapchainImageViews[imageIndex];
    imguiColorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    imguiColorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    imguiColorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    imguiColorAttachment.clearValue.color = vk::ClearColorValue();

    vk::RenderingInfoKHR imguiRenderingInfo{};
    imguiRenderingInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
    imguiRenderingInfo.renderArea.extent = vulkanContext->swapchainExtent;
    imguiRenderingInfo.layerCount = 1;
    imguiRenderingInfo.colorAttachmentCount = 1;
    imguiRenderingInfo.pColorAttachments = &imguiColorAttachment;

    commandBuffer.beginRendering(imguiRenderingInfo);

	vk::Viewport ImguiViewPort{};
	ImguiViewPort.x = 0.0f;
	ImguiViewPort.y = 0.0f;
	ImguiViewPort.width = static_cast<float>(vulkanContext->swapchainExtent.width);
	ImguiViewPort.height = static_cast<float>(vulkanContext->swapchainExtent.height);
	ImguiViewPort.minDepth = 0.0f;
	ImguiViewPort.maxDepth = 1.0f;


	vk::Rect2D ImguiScissor{};
	scissor.offset = imageoffset;
	scissor.extent = vulkanContext->swapchainExtent;

	commandBuffer.setViewport(0, 1, &ImguiViewPort);
	commandBuffer.setScissor(0, 1, &ImguiScissor);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    commandBuffer.endRendering();
	
	ImageTransitionData TransitionSwapchainToPresentData;
	TransitionSwapchainToPresentData.oldlayout = vk::ImageLayout::eColorAttachmentOptimal;
	TransitionSwapchainToPresentData.newlayout = vk::ImageLayout::ePresentSrcKHR;
	TransitionSwapchainToPresentData.SourceAccessflag = vk::AccessFlagBits::eColorAttachmentWrite;
	TransitionSwapchainToPresentData.DestinationAccessflag = vk::AccessFlagBits::eMemoryRead;
	TransitionSwapchainToPresentData.SourceOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	TransitionSwapchainToPresentData.DestinationOnThePipeline = vk::PipelineStageFlagBits::eBottomOfPipe;
	TransitionSwapchainToPresentData.AspectFlag = vk::ImageAspectFlagBits::eColor;

	bufferManger->TransitionImage(commandBuffer, vulkanContext->swapchainImages[imageIndex], TransitionSwapchainToPresentData);

	commandBuffer.end();
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


void App::destroy_swapchain()
{

	for (size_t i = 0; i < vulkanContext->swapchainImageViews.size(); i++)
	{
		vulkanContext->LogicalDevice.destroyImageView(vulkanContext->swapchainImageViews[i], nullptr);
	}

	vulkanContext->LogicalDevice.destroySwapchainKHR(vulkanContext->swapChain, nullptr);

	vulkanContext->swapchainImageViews.clear();

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

	destroy_swapchain();

	vulkanContext->create_swapchain();

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(
		static_cast<float>(vulkanContext->swapchainExtent.width),
		static_cast<float>(vulkanContext->swapchainExtent.height)
	);

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

	skyBox.reset();

	bufferManger->DestroyImage(DepthTextureData);
	vmaDestroyAllocator(bufferManger->allocator);
}


 App::~App()
{
	 ImGui_ImplVulkan_Shutdown();   
	 ImGui_ImplGlfw_Shutdown();    
	 ImGui::DestroyContext();      

	 bufferManger->DestroyImage(ImguiViewPortRenderTextureData);

	destroy_swapchain(); 
	DestroyBuffers();

	if (!commandBuffers.empty()) {
		vulkanContext->LogicalDevice.freeCommandBuffers(commandPool, commandBuffers);
		commandBuffers.clear();
	}
	vulkanContext->LogicalDevice.destroyDescriptorPool(ImGuiDescriptorPool);
	vulkanContext->LogicalDevice.destroyCommandPool(commandPool);
	vulkanContext->VulkanInstance.destroySurfaceKHR(vulkanContext->surface);
	vulkanContext->LogicalDevice.destroyPipeline(graphicsPipeline);
	vulkanContext->LogicalDevice.destroyPipeline(SkyBoxgraphicsPipeline);

	vulkanContext->LogicalDevice.destroyDescriptorPool(DescriptorPool);
	vulkanContext->LogicalDevice.destroyPipelineLayout(pipelineLayout);
	vulkanContext->LogicalDevice.destroyPipelineLayout(SkyBoxpipelineLayout);

	DestroySyncObjects();
	vulkanContext->LogicalDevice.destroy();
	vkb::destroy_debug_utils_messenger(vulkanContext->VulkanInstance, vulkanContext->Debug_Messenger);
	vulkanContext->VulkanInstance.destroy();

#ifndef NDEBUG
	_CrtDumpMemoryLeaks();  // Windows-only dev mode memory tracking
#endif

}

void App::SwapchainResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}

