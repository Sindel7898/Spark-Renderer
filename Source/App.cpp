#include "App.h"
#include <optional>

void App::Initialisation()
{
	window = new Window(800, 800, "Vulkan Window");

	InitVulkan();
	createSurface();
	SelectGPU_CreateDevice();
	create_swapchain();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFramebuffers();
	createCommandPool();
	createCommandBuffer();
	createSyncObjects();
}


void App::InitVulkan()
{
	vkb::InstanceBuilder builder;

	// Create a Vulkan instance with basic debug features
	auto inst_ret = builder.set_app_name(" Vulkan Application")
		.request_validation_layers(enableValidationLayers)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();

	if (!inst_ret)
	{
		throw std::runtime_error("Failed to create Vulkan instance: ");
	}

	 VKB_Instance = inst_ret.value();

	 // Store the Vulkan instance and debug messenger
	VulkanInstance = VKB_Instance.instance;
	Debug_Messenger = VKB_Instance.debug_messenger;
}

void App::createSurface()
{

	if (glfwCreateWindowSurface(VulkanInstance, window->GetWindow(), nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface)) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface!");
	}

}

void App::SelectGPU_CreateDevice() {

	//Select Required Vulkan featuers 
	vk::PhysicalDeviceVulkan13Features features_1_3{};
	features_1_3.sType = vk::StructureType::ePhysicalDeviceVulkan13Features;
	features_1_3.dynamicRendering = VK_TRUE;
	features_1_3.synchronization2 = VK_TRUE;

	vk::PhysicalDeviceVulkan12Features features_1_2{};
	features_1_2.sType = vk::StructureType::ePhysicalDeviceVulkan12Features;
	features_1_2.bufferDeviceAddress = VK_TRUE;
	features_1_2.descriptorIndexing = VK_TRUE;


	vkb::PhysicalDeviceSelector selector{ VKB_Instance };

	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features_1_3)
		.set_required_features_12(features_1_2)
		.set_surface(surface)
		.select()
		.value();
	

	if (!physicalDevice)
	{
		throw std::runtime_error("Failed to select a suitable physical device!");
	}

	//From the selecte device create a logical device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };
	vkb::Device   VKB_Device = deviceBuilder.build().value();
	LogicalDevice = VKB_Device.device;


	if (LogicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Failed to create logical device!");
	}

	//Print out name of GPU being used
	PhysicalDevice = physicalDevice.physical_device;
	std::cout << "GPU: " << std::string_view(PhysicalDevice.getProperties().deviceName) << std::endl;


	//Information about queues
	graphicsQueue = VKB_Device.get_queue(vkb::QueueType::graphics).value();
	presentQueue = VKB_Device.get_queue(vkb::QueueType::present).value();
	graphicsQueueFamilyIndex = VKB_Device.get_queue_index(vkb::QueueType::graphics).value();
}



void App::create_swapchain()
{

	vkb::SwapchainBuilder swapChainBuilder(PhysicalDevice, LogicalDevice, surface);
	swapchainformat = vk::Format::eB8G8R8A8Srgb;
	                    

	vk::SurfaceFormatKHR format;
        format.format = swapchainformat;
	    format.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
	
		int Width  = 0;
		int Height = 0;

		glfwGetFramebufferSize(window->GetWindow(), &Width, &Height);


	vkb::Swapchain vkbswapChain = swapChainBuilder
		.set_desired_format(format)
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(Width, Height)
		.add_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	swapchainExtent = vkbswapChain.extent;
	swapChain = vkbswapChain.swapchain;
	swapchainImages = vkbswapChain.get_images().value();
	swapchainImageViews = vkbswapChain.get_image_views().value();

}

void App::CreateRenderPass()
{
	vk::AttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subpass{};
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	
	vk::SubpassDependency dependency{};
	dependency.srcSubpass = vk::SubpassExternal;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.srcAccessMask = vk::AccessFlagBits::eNone;
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

	vk::AttachmentDescription colorAttachment{};
	colorAttachment.format = swapchainformat;
	colorAttachment.samples = vk::SampleCountFlagBits::e1;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;


	vk::RenderPassCreateInfo renderpassInfo{};
	renderpassInfo.attachmentCount = 1;
	renderpassInfo.pAttachments = &colorAttachment;
	renderpassInfo.subpassCount = 1;
	renderpassInfo.pSubpasses = &subpass;
	renderpassInfo.dependencyCount = 1;
	renderpassInfo.pDependencies = &dependency;

	renderPass = LogicalDevice.createRenderPass(renderpassInfo);
	if (!renderPass)
	{
		throw std::runtime_error("failed to create render pass!");
	}
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

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.setVertexBindingDescriptionCount(0);
	vertexInputInfo.setPVertexBindingDescriptions(nullptr);
	vertexInputInfo.setVertexAttributeDescriptionCount(0);
	vertexInputInfo.setPVertexAttributeDescriptions(nullptr);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	vk::PipelineInputAssemblyStateCreateInfo inputAssembleInfo{};
	inputAssembleInfo.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembleInfo.primitiveRestartEnable = vk::False;


	//Create ViewPort and scissor
	vk::Viewport viewport{};
	viewport.setX(0.0f);
	viewport.setY(0.0f);
	viewport.setHeight((float)swapchainExtent.height);
	viewport.setWidth ((float)swapchainExtent.width );
	viewport.setMinDepth(0.0f);
	viewport.setMaxDepth(1.0f);

	vk::Offset2D scissorOffset = { 0,0 };

	vk::Rect2D scissor{};
	scissor.setOffset(scissorOffset);
	scissor.setExtent(swapchainExtent);


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
	viewportState.setViewports(viewport);
	viewportState.setScissorCount(1);
	viewportState.setScissors(scissor);
	////////////////////////////////////////////////////////////////////////////////

	// Rasteriser information
	vk::PipelineRasterizationStateCreateInfo rasterizerinfo{};
	rasterizerinfo.depthClampEnable = vk::False;
	rasterizerinfo.rasterizerDiscardEnable = vk::False;
	rasterizerinfo.polygonMode = vk::PolygonMode::eFill;
    rasterizerinfo.lineWidth = 1.0f;
	rasterizerinfo.cullMode = vk::CullModeFlagBits::eBack;
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
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
 
	pipelineLayout = LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

	vk::GraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = ShaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembleInfo;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizerinfo;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlend;
	pipelineInfo.pDynamicState = &DynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;


	vk::Result result = LogicalDevice.createGraphicsPipelines(nullptr, 1, &pipelineInfo, nullptr, &graphicsPipeline);

	if (result != vk::Result::eSuccess)
	   {
			throw std::runtime_error("failed to create graphics pipeline!");
		}

	LogicalDevice.destroyShaderModule(VertShaderModule);
	LogicalDevice.destroyShaderModule(FragShaderModule);
}

vk::ShaderModule App::createShaderModule(const std::vector<char>& code)
{
	vk::ShaderModuleCreateInfo createInfo{};
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	vk::ShaderModule Shader;

	if (LogicalDevice.createShaderModule(&createInfo, nullptr, &Shader) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create shader module!");
	}

	return Shader;
}

void App::CreateFramebuffers()
{
	swapChainFramebuffers.resize(swapchainImageViews.size());

	for (size_t i = 0; i < swapchainImageViews.size(); i++) {

		vk::ImageView attachments[] = {
			swapchainImageViews[i]
		};

		vk::FramebufferCreateInfo frameBufferInfo{};
		frameBufferInfo.renderPass = renderPass;
		frameBufferInfo.attachmentCount = 1;
		frameBufferInfo.pAttachments = attachments;
		frameBufferInfo.width = swapchainExtent.width;
		frameBufferInfo.height = swapchainExtent.height;
		frameBufferInfo.layers = 1;

		swapChainFramebuffers[i] = LogicalDevice.createFramebuffer(frameBufferInfo);

		if (!swapChainFramebuffers[i])
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void App::createCommandPool()
{ 

	vk::CommandPoolCreateInfo poolInfo{};
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;

	commandPool = LogicalDevice.createCommandPool(poolInfo);

	if (!commandPool)
	{
		throw std::runtime_error("failed to create command pool!");

	}
}



void App::createCommandBuffer()
{
	vk::CommandBufferAllocateInfo allocateInfo{};
	allocateInfo.commandPool = commandPool;
	allocateInfo.level = vk::CommandBufferLevel::ePrimary;
	allocateInfo.commandBufferCount = 1;

	 LogicalDevice.allocateCommandBuffers(&allocateInfo, &commandBuffer);

	if (!commandBuffer)
	{
		throw std::runtime_error("failed to create command Buffer!");

	}


}


void App::createSyncObjects() {

	vk::SemaphoreCreateInfo semaphoreInfo{};

	if (LogicalDevice.createSemaphore(&semaphoreInfo, nullptr, &imageAvailableSemaphore) != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create image available semaphore!");
	}


	if (LogicalDevice.createSemaphore(&semaphoreInfo, nullptr, &renderFinishedSemaphore) != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create render finished semaphore!");
	}

	vk::FenceCreateInfo fenceInfo{};
	fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
	if (LogicalDevice.createFence(&fenceInfo, nullptr, &inFlightFence) != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create fence!");
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

}

void App::Draw()
{
	// Step 1: Wait for the inFlightFence to be signaled
   // This ensures that the previous frame has finished rendering before starting a new one.
   // The CPU waits here until the GPU signals that it's done with the previous frame.
	if (LogicalDevice.waitForFences(1, &inFlightFence, vk::True, UINT64_MAX) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to wait for fence");

	}

	// Step 2: Reset the inFlightFence to the unsignaled state
    // This prepares the fence for reuse in the current frame.
	if (LogicalDevice.resetFences(1, &inFlightFence) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to restFences");

	}

	// Step 3: Acquire the next available swapchain image
    // The GPU will signal the imageAvailableSemaphore when the image is ready for rendering.
    // imageIndex will store the index of the acquired swapchain image.
	uint32_t imageIndex;
	if (LogicalDevice.acquireNextImageKHR(swapChain, UINT64_MAX, imageAvailableSemaphore, nullptr, &imageIndex) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to acquire Next Image");
	}

	// Step 4: Reset the command buffer and record rendering commands for the current frame
	// This prepares the command buffer for reuse and records the rendering commands.
	commandBuffer.reset();
	recordCommandBuffer(commandBuffer, imageIndex);


	 // Step 5: Set up synchronization for rendering
     // The GPU will wait for the imageAvailableSemaphore to be signaled before starting rendering.
    // It will wait at the eColorAttachmentOutput stage, which is where color attachment writes occur
	vk::Semaphore waitSemaphores[] = { imageAvailableSemaphore };
	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

	// The GPU will signal the renderFinishedSemaphore when rendering is complete.	
	vk::Semaphore signalSemaphores[] = { renderFinishedSemaphore };

	vk::SubmitInfo submitInfo{};
	submitInfo.sType = vk::StructureType::eSubmitInfo;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;


	// The inFlightFence will be signaled when the GPU is done with the command buffer.
		if (graphicsQueue.submit(1, &submitInfo, inFlightFence) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to submit info");

	}

	vk::SwapchainKHR swapChains[] = { swapChain };
	vk::PresentInfoKHR presentInfo{};
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	
	//wait on renderFinishedSemaphore before this is ran
	if (presentQueue.presentKHR(presentInfo) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to submit info");

	}

}


void  App::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) {

	vk::ClearValue clearColor{};
	clearColor.color = { 0.0f, 0.0f, 0.0f, 1.0f };

	vk::CommandBufferBeginInfo begininfo{};
	begininfo.pInheritanceInfo = nullptr;

	commandBuffer.begin(begininfo);

	//////////////////////////////////////////////////////////////////////
	VkOffset2D imageoffset = { 0, 0 };
	vk::RenderPassBeginInfo renderPassInfo{};
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = imageoffset;
	renderPassInfo.renderArea.extent = swapchainExtent;
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
	commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

	vk::Viewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapchainExtent.width);
	viewport.height = static_cast<float>(swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	commandBuffer.setViewport(0, 1, &viewport);

	vk::Rect2D scissor{};
	scissor.offset = imageoffset;
	scissor.extent = swapchainExtent;
	commandBuffer.setScissor(0, 1, &scissor);
	commandBuffer.draw(3, 1, 0, 0);

	commandBuffer.endRenderPass();
	commandBuffer.end();

}


void App::destroy_swapchain()
{
	LogicalDevice.destroySwapchainKHR(swapChain, nullptr);

	for (size_t i = 0; i < swapchainImageViews.size(); i++)
	{
		LogicalDevice.destroyImageView(swapchainImageViews[i], nullptr);
	}

	swapchainImageViews.clear();
	swapchainImages.clear();
}

void App::destroy_frameBuffers()
{

	for (auto framebuffer : swapChainFramebuffers) {
		LogicalDevice.destroyFramebuffer(framebuffer);
	}

}


void  App::StartFrame() {
	auto now = std::chrono::high_resolution_clock::now();
	deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(now - lastFrameTime).count();
	lastFrameTime = now;

	if (deltaTime > 0.0f) {
		fps = 1.0f / deltaTime;
	}
}

void App::CleanUp()
{
	destroy_swapchain();
	LogicalDevice.destroyCommandPool(commandPool);
	VulkanInstance.destroySurfaceKHR(surface);
	LogicalDevice.destroyPipeline(graphicsPipeline);
	LogicalDevice.destroyPipelineLayout(pipelineLayout);
	LogicalDevice.destroyRenderPass(renderPass);
	destroy_frameBuffers();
	LogicalDevice.destroySemaphore(imageAvailableSemaphore); 
	LogicalDevice.destroySemaphore(renderFinishedSemaphore);
	LogicalDevice.destroyFence(inFlightFence);
	LogicalDevice.destroy();
	vkb::destroy_debug_utils_messenger(VulkanInstance, Debug_Messenger);
	VulkanInstance.destroy();
	delete window; 
}