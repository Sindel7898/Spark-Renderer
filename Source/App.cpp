#include "App.h"
#include <optional>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

VmaAllocator Allocator;
VmaAllocation VertexBufferAllocation;
VmaAllocation IndexBufferAllocation;

void App::Initialisation()
{
	window = new Window(800, 800, "Vulkan Window");

	//glfwSetFramebufferSizeCallback(window->GetWindow(),);
	glfwSetWindowUserPointer(window->GetWindow(), this);

	InitVulkan();
	createSurface();
	SelectGPU_CreateDevice();
	InitMemAllocator();
	create_swapchain();
	createDescriptorSetLayout();
	CreateGraphicsPipeline();
	createCommandPool();
	createCommandBuffer();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffer();
	createDescriptorPool();
	createDescriptorSets();
	createSyncObjects();

}
void App::createDescriptorSetLayout()
{
	vk::DescriptorSetLayoutBinding UniformBufferBinding{};
	UniformBufferBinding.binding = 0;
	UniformBufferBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	UniformBufferBinding.descriptorCount = 1;
	UniformBufferBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
	UniformBufferBinding.pImmutableSamplers = nullptr; // Optional

	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &UniformBufferBinding;

	LogicalDevice.createDescriptorSetLayout(&layoutInfo,nullptr, &DescriptorSetLayout);
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

void App::InitMemAllocator()
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3; // Or your Vulkan version
	allocatorInfo.physicalDevice = static_cast<VkPhysicalDevice>(PhysicalDevice);
	allocatorInfo.device = static_cast<VkDevice>(LogicalDevice);
	allocatorInfo.instance = static_cast<VkInstance>(VulkanInstance);

	vmaCreateAllocator(&allocatorInfo, &Allocator);
}



void App::recreateSwapChain() {

	int width = 0, height = 0;
	glfwGetFramebufferSize(window->GetWindow(), &width, &height);
	
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window->GetWindow(), &width, &height);
		glfwWaitEvents();
	}

	LogicalDevice.waitIdle();


	destroy_swapchain();

	create_swapchain();
	//CreateFramebuffers();
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

void App::createVertexBuffer()
{
   ///Creates slow cpu accessed buffer to transfer it to fast gpu buffer
	VmaAllocationCreateInfo AllocationInfo = {};
	AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
	AllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
	                       VMA_ALLOCATION_CREATE_MAPPED_BIT; // Make it CPU-visible but sllow

	//////////////////////////////////////////////////////////////////////////
	VmaAllocation StagingBufferAllocation;
	vk::Buffer StagingBuffer;

	vk::BufferCreateInfo StagingBufferCreateInfo;
	StagingBufferCreateInfo.size = sizeof(vertices[0]) * vertices.size();
	StagingBufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	StagingBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

	VkBufferCreateInfo cStagingBufferInfo = static_cast<VkBufferCreateInfo>(StagingBufferCreateInfo);
	VkBuffer cstagingbuffer;

	if (vmaCreateBuffer(Allocator, &cStagingBufferInfo, &AllocationInfo, &cstagingbuffer, &StagingBufferAllocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	StagingBuffer = static_cast<vk::Buffer>(cstagingbuffer);

	void* data;
	vmaMapMemory(Allocator, StagingBufferAllocation, &data);
	memcpy(data, vertices.data(), StagingBufferCreateInfo.size); 
	vmaUnmapMemory(Allocator, StagingBufferAllocation);
	/////////////////////////////////////////////////////////////////////////////////////////////

    ///Create GPU OPTIMISED VertexBuffer
	vk::BufferCreateInfo VertexBufferInfo;
	VertexBufferInfo.size = sizeof(vertices[0]) * vertices.size();
	VertexBufferInfo.usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer;
	VertexBufferInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo vertexAllocInfo = {};
	vertexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	vertexAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT; // GPU-optimal memory

	VkBufferCreateInfo cVertexBufferInfo = static_cast<VkBufferCreateInfo>(VertexBufferInfo);
	VkBuffer cVertexbuffer;

	if (vmaCreateBuffer(Allocator, &cVertexBufferInfo, &vertexAllocInfo, &cVertexbuffer, &VertexBufferAllocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	VertexBuffer = static_cast<vk::Buffer>(cVertexbuffer);
	
	//Copy Data from the stagineBuffer to the VertexBuffer
	CopyBuffer(StagingBuffer, VertexBuffer, VertexBufferInfo.size);
	//Clear DestryStagingBuffer
	vmaDestroyBuffer(Allocator, static_cast<VkBuffer>(StagingBuffer), StagingBufferAllocation);
}

void App::createIndexBuffer()
{
	///Creates slow cpu accessed buffer to transfer it to fast gpu buffer
	VmaAllocationCreateInfo AllocationInfo = {};
	AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
	AllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		VMA_ALLOCATION_CREATE_MAPPED_BIT; // Make it CPU-visible but sllow

	//////////////////////////////////////////////////////////////////////////
	VmaAllocation StagingBufferAllocation;
	vk::Buffer StagingBuffer;

	vk::BufferCreateInfo StagingBufferCreateInfo;
	StagingBufferCreateInfo.size = sizeof(indices[0]) * indices.size();
	StagingBufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	StagingBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

	VkBufferCreateInfo cStagingBufferInfo = static_cast<VkBufferCreateInfo>(StagingBufferCreateInfo);
	VkBuffer cstagingbuffer;

	if (vmaCreateBuffer(Allocator, &cStagingBufferInfo, &AllocationInfo, &cstagingbuffer, &StagingBufferAllocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	StagingBuffer = static_cast<vk::Buffer>(cstagingbuffer);

	void* data;
	vmaMapMemory(Allocator, StagingBufferAllocation, &data);
	memcpy(data, indices.data(), StagingBufferCreateInfo.size);
	vmaUnmapMemory(Allocator, StagingBufferAllocation);
	/////////////////////////////////////////////////////////////////////////////////////////////

	///Create GPU OPTIMISED VertexBuffer
	vk::BufferCreateInfo IndexBufferInfo;
	IndexBufferInfo.size = sizeof(indices[0]) * indices.size();
	IndexBufferInfo.usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer;
	IndexBufferInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo IndexAllocInfo = {};
	IndexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	IndexAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT; // GPU-optimal memory

	VkBufferCreateInfo cIndexBufferInfo = static_cast<VkBufferCreateInfo>(IndexBufferInfo);
	VkBuffer cIndexbuffer;

	if (vmaCreateBuffer(Allocator, &cIndexBufferInfo, &IndexAllocInfo, &cIndexbuffer, &IndexBufferAllocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	IndexBuffer = static_cast<vk::Buffer>(cIndexbuffer);

	//Copy Data from the stagineBuffer to the VertexBuffer
	CopyBuffer(StagingBuffer, IndexBuffer, IndexBufferInfo.size);
	//Clear DestryStagingBuffer
	vmaDestroyBuffer(Allocator, static_cast<VkBuffer>(StagingBuffer), StagingBufferAllocation);
}

void App::createUniformBuffer()
{

   std::vector<VmaAllocation> uniformBufferAllocations;

	uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
	uniformBufferAllocations.resize(MAX_FRAMES_IN_FLIGHT);

	VmaAllocationCreateInfo AllocationInfo = {};
	AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
	AllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		VMA_ALLOCATION_CREATE_MAPPED_BIT; 

	vk::BufferCreateInfo UniformBufferCreateInfo;
	UniformBufferCreateInfo.size = sizeof(UniformBufferObject);
	UniformBufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	UniformBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;


	for (size_t i = 0; i < uniformBuffers.size(); i++)
	{
		VkBufferCreateInfo cUniformBufferCreateInfo = static_cast<VkBufferCreateInfo>(UniformBufferCreateInfo);
		VkBuffer cUniformBuffer;

		VmaAllocation vkAllocation;


		if (vmaCreateBuffer(Allocator, &cUniformBufferCreateInfo, &AllocationInfo, &cUniformBuffer, &vkAllocation, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create buffer!");
		}

		uniformBufferAllocations[i] = vkAllocation;

		uniformBuffers[i] = static_cast<vk::Buffer>(cUniformBuffer);

		vmaMapMemory(Allocator, uniformBufferAllocations[i], &uniformBuffersMapped[i]);
	}
}

void App::createDescriptorPool()
{
	vk::DescriptorPoolSize poolsize;
	poolsize.type = vk::DescriptorType::eUniformBuffer; 
	poolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	vk::DescriptorPoolCreateInfo poolInfo{};
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolsize;
	poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	DescriptorPool = LogicalDevice.createDescriptorPool(poolInfo, nullptr);

}

void App::createDescriptorSets()
{
	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, DescriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocinfo;
	allocinfo.descriptorPool = DescriptorPool;
	allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocinfo.pSetLayouts = layouts.data();

	DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	LogicalDevice.allocateDescriptorSets(&allocinfo, DescriptorSets.data());

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::DescriptorBufferInfo bufferInfo{};

		bufferInfo.buffer = uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);
	
		vk::WriteDescriptorSet descriptorWrite{};
		descriptorWrite.dstSet = DescriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		LogicalDevice.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
	}
}

void App::CopyBuffer(vk::Buffer Buffer1, vk::Buffer Buffer2, VkDeviceSize size) {

	vk::CommandBufferAllocateInfo allocateinfo{};
	allocateinfo.commandPool = commandPool;
	allocateinfo.commandBufferCount = 1;
	allocateinfo.level = vk::CommandBufferLevel::ePrimary;

	vk::CommandBuffer commandBuffer = LogicalDevice.allocateCommandBuffers(allocateinfo)[0];

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

	graphicsQueue.submit(1, &submitInfo, nullptr);
	graphicsQueue.waitIdle(); // Wait for the transfer to complete

	LogicalDevice.freeCommandBuffers(commandPool, 1, &commandBuffer);
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
 
	pipelineLayout = LogicalDevice.createPipelineLayout(pipelineLayoutInfo, nullptr);

	vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
	pipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainformat;

	vk::GraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.pNext = &pipelineRenderingCreateInfo; // Add this line
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
	pipelineInfo.renderPass = VK_NULL_HANDLE;
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
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	vk::CommandBufferAllocateInfo allocateInfo{};
	allocateInfo.commandPool = commandPool;
	allocateInfo.level = vk::CommandBufferLevel::ePrimary;
	allocateInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	commandBuffers = LogicalDevice.allocateCommandBuffers(allocateInfo);

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
		if (LogicalDevice.createSemaphore(&semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != vk::Result::eSuccess) {
			throw std::runtime_error("failed to create image available semaphore!");
		}


		if (LogicalDevice.createSemaphore(&semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != vk::Result::eSuccess) {
			throw std::runtime_error("failed to create render finished semaphore!");
		}

		if (LogicalDevice.createFence(&fenceInfo, nullptr, &inFlightFences[i]) != vk::Result::eSuccess) {
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
	LogicalDevice.waitIdle();
}

void App::Draw()
{
	if (LogicalDevice.waitForFences(1, &inFlightFences[currentFrame], vk::True, UINT64_MAX) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to wait for fence");

	}

	uint32_t imageIndex;

	try {
		vk::Result result = LogicalDevice.acquireNextImageKHR(
			swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], nullptr, &imageIndex
		);

	}
	catch (const std::exception& e) {
		recreateSwapChain();
		std::cerr << "Exception: " << e.what() << std::endl;
	}



	LogicalDevice.resetFences(1, &inFlightFences[currentFrame]);

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
		if (graphicsQueue.submit(1, &submitInfo, inFlightFences[currentFrame]) != vk::Result::eSuccess)
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
   try {
	   vk::Result result = presentQueue.presentKHR(presentInfo);
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
	ubo.proj = glm::perspective(glm::radians(45.0f), swapchainExtent.width / (float)swapchainExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));


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
	acquireBarrier.image = swapchainImages[imageIndex];
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
	colorAttachmentInfo.imageView = swapchainImageViews[imageIndex];
	colorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	colorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachmentInfo.clearValue = clearColor;


	vk::RenderingInfoKHR renderingInfo{};
	renderingInfo.renderArea.offset = imageoffset;
	renderingInfo.renderArea.extent = swapchainExtent;
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachmentInfo;

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

	commandBuffer.beginRendering(renderingInfo);


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
	barrier.image = swapchainImages[imageIndex];
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


	for (size_t i = 0; i < swapchainImageViews.size(); i++)
	{
		LogicalDevice.destroyImageView(swapchainImageViews[i], nullptr);
	}

	LogicalDevice.destroySwapchainKHR(swapChain, nullptr);

	swapchainImageViews.clear();

}



void  App::StartFrame() {
	auto now = std::chrono::high_resolution_clock::now();
	deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(now - lastFrameTime).count();
	lastFrameTime = now;

	if (deltaTime > 0.0f) {
		fps = 1.0f / deltaTime;
	}
}

void App::DestroySyncObjects()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		LogicalDevice.destroySemaphore(imageAvailableSemaphores[i]);
		LogicalDevice.destroySemaphore(renderFinishedSemaphores[i]);
		LogicalDevice.destroyFence(inFlightFences[i]);
	}
}

void App::CleanUp()
{
	destroy_swapchain(); 
	vmaDestroyBuffer(Allocator, static_cast<VkBuffer>(VertexBuffer), VertexBufferAllocation);
	vmaDestroyBuffer(Allocator, static_cast<VkBuffer>(IndexBuffer), IndexBufferAllocation);
	
	vmaDestroyAllocator(Allocator);
	LogicalDevice.destroyCommandPool(commandPool);
	VulkanInstance.destroySurfaceKHR(surface);
	LogicalDevice.destroyDescriptorSetLayout(DescriptorSetLayout);
	LogicalDevice.destroyPipeline(graphicsPipeline);
	LogicalDevice.destroyDescriptorPool(DescriptorPool);
	LogicalDevice.destroyPipelineLayout(pipelineLayout);
	LogicalDevice.destroyRenderPass(renderPass);
	DestroySyncObjects();
	LogicalDevice.destroy();
	vkb::destroy_debug_utils_messenger(VulkanInstance, Debug_Messenger);
	VulkanInstance.destroy();
	delete window; 
}

void App::SwapchainResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}

