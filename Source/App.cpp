#include "App.h"
#include <optional>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//std::vector<VmaAllocation> uniformBufferAllocations;

void App::Initialisation()
{
	window = std::make_unique<Window>(800, 800, "Vulkan Window");
	vulkanContext = std::make_unique<VulkanContext>(*window);
	bufferManger = std::make_unique<BufferManager>(vulkanContext->LogicalDevice, vulkanContext->PhysicalDevice, vulkanContext->VulkanInstance);

	//glfwSetFramebufferSizeCallback(window->GetWindow(),);
	glfwSetWindowUserPointer(window->GetWindow(), this);
	/////////////////////////
	createDescriptorSetLayout();
	createDescriptorPool();
	createSyncObjects();
	//////////////////////////
	CreateGraphicsPipeline();
	//////////////////////////
	createCommandPool();
	createCommandBuffer();
	/////////////////////////////////
	createDepthTextureImage();
	createTextureImage();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffer();
	createDescriptorSets();
	/////////////////////////////////

}

void App::createDescriptorSetLayout()
{
	vk::DescriptorSetLayoutBinding UniformBufferBinding{};
	UniformBufferBinding.binding = 0;
	UniformBufferBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	UniformBufferBinding.descriptorCount = 1;
	UniformBufferBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
	UniformBufferBinding.pImmutableSamplers = nullptr; // Optional

	vk::DescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

	std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { UniformBufferBinding, samplerLayoutBinding };

	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();


	if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &DescriptorSetLayout) != vk::Result::eSuccess)
	{
		throw std::runtime_error("Failed to create descriptorset layout!");

	}
}

void App::createDescriptorPool()
{
	vk::DescriptorPoolSize Uniformpoolsize;
	Uniformpoolsize.type = vk::DescriptorType::eUniformBuffer;
	Uniformpoolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	vk::DescriptorPoolSize Samplerpoolsize;
	Samplerpoolsize.type = vk::DescriptorType::eSampler;
	Samplerpoolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	std::array<	vk::DescriptorPoolSize, 2> poolSizes{ Uniformpoolsize ,Samplerpoolsize };

	vk::DescriptorPoolCreateInfo poolInfo{};
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());;
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	DescriptorPool = vulkanContext->LogicalDevice.createDescriptorPool(poolInfo, nullptr);

}

void App::createDescriptorSets()
{
	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, DescriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocinfo;
	allocinfo.descriptorPool = DescriptorPool;
	allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocinfo.pSetLayouts = layouts.data();

	DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, DescriptorSets.data());

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		vk::DescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers[i].buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		vk::DescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.imageView = TextureImageView;
		imageInfo.sampler = TextureSampler;

		vk::WriteDescriptorSet UniformdescriptorWrite{};
		UniformdescriptorWrite.dstSet = DescriptorSets[i];
		UniformdescriptorWrite.dstBinding = 0;
		UniformdescriptorWrite.dstArrayElement = 0;
		UniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		UniformdescriptorWrite.descriptorCount = 1;
		UniformdescriptorWrite.pBufferInfo = &bufferInfo;

		vk::WriteDescriptorSet SamplerdescriptorWrite{};
		SamplerdescriptorWrite.dstSet = DescriptorSets[i];
		SamplerdescriptorWrite.dstBinding = 1;
		SamplerdescriptorWrite.dstArrayElement = 0;
		SamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		SamplerdescriptorWrite.descriptorCount = 1;
		SamplerdescriptorWrite.pImageInfo = &imageInfo;

		std::array<vk::WriteDescriptorSet, 2> descriptorWrites{ UniformdescriptorWrite ,SamplerdescriptorWrite };

		vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}



void App::createTextureImage()
{
	 MeshTextureData = bufferManger->CreateTextureImage("../Textures/Dog.jpg", commandPool, vulkanContext->graphicsQueue);
	 TextureImage = MeshTextureData.image;
	 TextureImageView = MeshTextureData.imageView;
	 TextureSampler = MeshTextureData.imageSampler;
}

void App::createDepthTextureImage()
{
	vk::Extent3D imageExtent = { vulkanContext->swapchainExtent.width,vulkanContext->swapchainExtent.height,1 };

	DepthTextureData = bufferManger->CreateImage(imageExtent, vk::Format::eD32SfloatS8Uint, vk::ImageUsageFlagBits::eDepthStencilAttachment);
	DepthTextureData.imageView = bufferManger->CreateImageView(DepthTextureData.image, vk::Format::eD32SfloatS8Uint, vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil);

	DepthImageView = DepthTextureData.imageView;

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

void App::createVertexBuffer()
{
	VkDeviceSize VertexBufferSize = sizeof(vertices[0]) * vertices.size();
	VertexBufferData =  bufferManger->CreateGPUOptimisedBuffer(vertices.data(), VertexBufferSize,vk::BufferUsageFlagBits::eVertexBuffer ,commandPool, vulkanContext->graphicsQueue);
	VertexBuffer = VertexBufferData.buffer;
   
}

void App::createIndexBuffer()
{

	VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
	IndexBufferData = bufferManger->CreateGPUOptimisedBuffer(indices.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer, commandPool, vulkanContext->graphicsQueue);
	IndexBuffer = IndexBufferData.buffer;

}

void App::createUniformBuffer()
{
	uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	uniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

	VkDeviceSize UniformBufferSize = sizeof(UniformBufferObject);

	for (size_t i = 0; i < uniformBuffers.size(); i++)
	{

		BufferData bufferdata  = bufferManger->CreateBuffer(UniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
		uniformBuffers[i] = bufferdata;

		uniformBuffersMappedMem[i] = bufferManger->MapMemory(bufferdata);
	}
}

void App::CopyBuffer(vk::Buffer Buffer1, vk::Buffer Buffer2, VkDeviceSize size) {

	
	vk::CommandBufferAllocateInfo allocateinfo{};
	allocateinfo.commandPool = commandPool;
	allocateinfo.commandBufferCount = 1;
	allocateinfo.level = vk::CommandBufferLevel::ePrimary;

	vk::CommandBuffer commandBuffer = vulkanContext->LogicalDevice.allocateCommandBuffers(allocateinfo)[0];

	vk::CommandBufferBeginInfo CBBegininfo{};
	CBBegininfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	commandBuffer.begin(CBBegininfo);

	vk::BufferCopy copyregion{};
	copyregion.srcOffset = 0;
	copyregion.dstOffset = 0;
	copyregion.size = size;

	commandBuffer.copyBuffer(Buffer1, Buffer2, copyregion);
	
	commandBuffer.end();

	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vulkanContext->graphicsQueue.submit(1, &submitInfo, nullptr);
	vulkanContext->graphicsQueue.waitIdle(); // Wait for the task to be done 

	vulkanContext->LogicalDevice.freeCommandBuffers(commandPool, 1, &commandBuffer);
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


	auto BindDesctiptions = Vertex::GetBindingDescription();
	auto attributeDescriptions = Vertex::GetAttributeDescription();

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.setVertexBindingDescriptionCount(1);
	vertexInputInfo.setVertexAttributeDescriptionCount(3);
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
	rasterizerinfo.cullMode = vk::CullModeFlagBits::eBack;
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
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &DescriptorSetLayout; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
 
	pipelineLayout = vulkanContext->LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

	vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
	pipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = &vulkanContext->swapchainformat;
	pipelineRenderingCreateInfo.depthAttachmentFormat = vk::Format::eD32SfloatS8Uint;

	vk::PipelineDepthStencilStateCreateInfo depthStencilState;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = vk::CompareOp::eLess;
	depthStencilState.stencilTestEnable = VK_TRUE;
	depthStencilState.front.compareOp = vk::CompareOp::eAlways; // Always pass stencil test
	depthStencilState.front.failOp = vk::StencilOp::eKeep; // Keep the stencil value on fail
	depthStencilState.front.depthFailOp = vk::StencilOp::eIncrementAndClamp; // Increment on depth fail
	depthStencilState.front.passOp = vk::StencilOp::eKeep; // Keep the stencil value on pass
	
	// Stencil operations for back-facing polygons
	depthStencilState.back = depthStencilState.front; // Use the same operations for back faces

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
	while (!window->shouldClose())
	{
		glfwPollEvents();
		StartFrame();
		Draw();
		std::cout << "FPS: " << fps << std::endl;
	}

	//waits for the device to finnish all its processes before shutting down
	vulkanContext->LogicalDevice.waitIdle();
}

void App::Draw()
{
	if (vulkanContext->LogicalDevice.waitForFences(1, &inFlightFences[currentFrame], vk::True, UINT64_MAX) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to wait for fence");

	}

	uint32_t imageIndex;

	try {
		vk::Result result = vulkanContext->LogicalDevice.acquireNextImageKHR(
			vulkanContext->swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], nullptr, &imageIndex
		);

	}
	catch (const std::exception& e) {
		recreateSwapChain();
		std::cerr << "Exception: " << e.what() << std::endl;
	}



	vulkanContext->LogicalDevice.resetFences(1, &inFlightFences[currentFrame]);

	commandBuffers[currentFrame].reset();
	recordCommandBuffer(commandBuffers[currentFrame], imageIndex);
	updateUniformBuffer(currentFrame);

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

	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), vulkanContext->swapchainExtent.width / (float)vulkanContext->swapchainExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	memcpy(uniformBuffersMappedMem[currentImage], &ubo, sizeof(ubo));
}

void  App::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) {

	vk::ClearValue clearColor{};
	clearColor.color = { 0.0f, 0.0f, 0.0f, 1.0f };

	vk::CommandBufferBeginInfo begininfo{};
	begininfo.pInheritanceInfo = nullptr;

	commandBuffer.begin(begininfo);

	////Transition image from undifined to ac olorattachment so it can be modified
	vk::ImageMemoryBarrier acquireBarrier{};
	acquireBarrier.oldLayout = vk::ImageLayout::eUndefined;
	acquireBarrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
	acquireBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	acquireBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	acquireBarrier.image = vulkanContext->swapchainImages[imageIndex];
	acquireBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	acquireBarrier.subresourceRange.baseMipLevel = 0;
	acquireBarrier.subresourceRange.levelCount = 1;
	acquireBarrier.subresourceRange.baseArrayLayer = 0;
	acquireBarrier.subresourceRange.layerCount = 1;

	acquireBarrier.srcAccessMask = vk::AccessFlagBits::eNone;
	acquireBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

	// Bind the transitioned image to the pipeline using a pipeline barrier
	commandBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::DependencyFlags(),
		0, nullptr,
		0, nullptr,
		1, &acquireBarrier
	);

	//////////////////////////////////////////////////////////////////////
	VkOffset2D imageoffset = { 0, 0 };

	vk::RenderingAttachmentInfoKHR colorAttachmentInfo{};
	colorAttachmentInfo.imageView = vulkanContext->swapchainImageViews[imageIndex];
	colorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	colorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachmentInfo.clearValue = clearColor;

	vk::RenderingAttachmentInfo depthStencilAttachment;
	depthStencilAttachment.imageView = DepthImageView;
	depthStencilAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	depthStencilAttachment.loadOp = vk::AttachmentLoadOp::eClear; // Clear the depth-stencil buffer
	depthStencilAttachment.storeOp = vk::AttachmentStoreOp::eStore; // Store the depth-stencil buffer
	depthStencilAttachment.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0); 

	vk::RenderingInfoKHR renderingInfo{};
	renderingInfo.renderArea.offset = imageoffset;
	renderingInfo.renderArea.extent = vulkanContext->swapchainExtent;
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachmentInfo;
	renderingInfo.pDepthAttachment = &depthStencilAttachment;

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

	commandBuffer.beginRendering(renderingInfo);


	vk::Viewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(vulkanContext->swapchainExtent.width);
	viewport.height = static_cast<float>(vulkanContext->swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	commandBuffer.setViewport(0, 1, &viewport);

	vk::Rect2D scissor{};
	scissor.offset = imageoffset;
	scissor.extent = vulkanContext->swapchainExtent;
	commandBuffer.setScissor(0, 1, &scissor);


	vk::Buffer VertexBuffers[] = { VertexBuffer };
	vk::DeviceSize offsets[] = { 0 };

	commandBuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets); 
	commandBuffer.bindIndexBuffer(IndexBuffer, 0, vk::IndexType::eUint16);

	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &DescriptorSets[currentFrame], 0, nullptr);


	commandBuffer.drawIndexed(indices.size(), 1, 0, 0, 0);
	commandBuffer.endRendering();


	// Create an image memory barrier to transition the image layout
	// from a color attachment optimal layout to a present source layout,
	// so it can be presented to the screen.
	vk::ImageMemoryBarrier barrier{};
	barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
	barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = vulkanContext->swapchainImages[imageIndex];
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eMemoryRead;

	// Bind the transitioned image to the pipeline using a pipeline barrier
	commandBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::PipelineStageFlagBits::eBottomOfPipe,
		vk::DependencyFlags(),
		0, nullptr,
		0, nullptr,
		1, &barrier
	);


	commandBuffer.end();

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



void  App::StartFrame() {

	auto now = std::chrono::high_resolution_clock::now();
	deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(now - lastFrameTime).count();
	lastFrameTime = now;

	if (deltaTime > 0.0f) {
		fps = 1.0f / deltaTime;
	}
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
	vulkanContext->LogicalDevice.destroyImageView(DepthImageView, nullptr);

	vulkanContext->create_swapchain();
	createDepthTextureImage();

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

	//vmaDestroyBuffer(bufferManger->allocator, static_cast<VkBuffer>(VertexBuffer), VertexBufferAllocation);
	//vmaDestroyBuffer(bufferManger->allocator, static_cast<VkBuffer>(IndexBuffer), IndexBufferAllocation);

	vulkanContext->LogicalDevice.destroyImageView(TextureImageView);
	vulkanContext->LogicalDevice.destroyImageView(DepthImageView);
	vulkanContext->LogicalDevice.destroySampler(TextureSampler);

	bufferManger->DestroyImage(MeshTextureData);
	bufferManger->DestroyImage(DepthTextureData);
	bufferManger->DestroyBuffer(VertexBufferData);
	bufferManger->DestroyBuffer(IndexBufferData);

	for (size_t i = 0; i < uniformBuffers.size(); i++)
	{
		vmaUnmapMemory(bufferManger->allocator, uniformBuffers[i].allocation);
		vmaDestroyBuffer(bufferManger->allocator, static_cast<VkBuffer>(uniformBuffers[i].buffer), uniformBuffers[i].allocation);
	}

	vmaDestroyAllocator(bufferManger->allocator);
}


void App::CleanUp()
{
	destroy_swapchain(); 
	DestroyBuffers();
	vulkanContext->LogicalDevice.destroyCommandPool(commandPool);
	vulkanContext->VulkanInstance.destroySurfaceKHR(vulkanContext->surface);
	vulkanContext->LogicalDevice.destroyDescriptorSetLayout(DescriptorSetLayout);
	vulkanContext->LogicalDevice.destroyPipeline(graphicsPipeline);
	vulkanContext->LogicalDevice.destroyDescriptorPool(DescriptorPool);
	vulkanContext->LogicalDevice.destroyPipelineLayout(pipelineLayout);
	DestroySyncObjects();
	vulkanContext->LogicalDevice.destroy();
	vkb::destroy_debug_utils_messenger(vulkanContext->VulkanInstance, vulkanContext->Debug_Messenger);
	vulkanContext->VulkanInstance.destroy();
}

void App::SwapchainResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}

