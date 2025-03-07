#include "App.h"

void App::Initialisation()
{
	window = new Window(800, 800, "Vulkan Window"); // For pointer

	InitVulkan();
	createSurface();
	SelectGPU_CreateDevice();
	create_swapchain();
}

void App::InitVulkan()
{
	vkb::InstanceBuilder builder;

	//make the vulkan instance, with basic debug features
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

	//grab the instance 
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

	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	VKB_Device = deviceBuilder.build().value();

	LogicalDevice = VKB_Device.device;

	if (LogicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Failed to create logical device!");
	}

	PhysicalDevice = physicalDevice.physical_device;

	std::cout << "GPU: " << std::string_view(PhysicalDevice.getProperties().deviceName) << std::endl;
}



void App::create_swapchain()
{
	vkb::SwapchainBuilder swapChainBuilder(PhysicalDevice, LogicalDevice, surface);

	//vk::Format swapchainImageFormat = vk::Format::eB8G8R8A8Srgb;
	                    

	vk::SurfaceFormatKHR format;
        format.format = vk::Format::eB8G8R8A8Srgb;
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

void App::CreateGraphicsPipeline()
{
	auto VertShaderCode = readFile("shaders/vert.spv");
	auto FragShaderCode = readFile("shaders/frag.spv");
	
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

	vk::PipelineInputAssemblyStateCreateInfo inputAssembleInfo{};
	inputAssembleInfo.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembleInfo.primitiveRestartEnable = vk::False;

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


	std::vector<vk::DynamicState> DynamicStates = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};

	vk::PipelineDynamicStateCreateInfo DynamicState{};
	DynamicState.dynamicStateCount = static_cast<uint32_t>(DynamicStates.size());
	DynamicState.pDynamicStates = DynamicStates.data();

	vk::PipelineViewportStateCreateInfo viewportState{};
	viewportState.setViewportCount(1);
	viewportState.setViewports(viewport);
	viewportState.setScissorCount(1);
	viewportState.setScissors(scissor);






	LogicalDevice.destroyShaderModule(VertShaderModule);
	LogicalDevice.destroyShaderModule(FragShaderModule);
}

vk::ShaderModule App::createShaderModule(const std::vector<char>& code)
{
	vk::ShaderModuleCreateInfo createInfo{};
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	vk::ShaderModule Shader;

	if ( LogicalDevice.createShaderModule(&createInfo, nullptr, &Shader) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create shader module!");
	}
	
	return Shader;
}


void App::Run()
{
	MainLoop();
	Draw();
}

void App::MainLoop()
{
	while (!window->shouldClose())
	{
		glfwPollEvents();
	}

}

void App::Draw()
{

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

void App::getqueue()
{
	graphicsQueue = VKB_Device.get_queue(vkb::QueueType::graphics).value();
	graphicsQueueFamily = VKB_Device.get_queue_index(vkb::QueueType::graphics).value();
	
}


void App::CleanUp()
{
	destroy_swapchain();
	VulkanInstance.destroySurfaceKHR(surface, nullptr);
	LogicalDevice.destroy(nullptr);
	vkb::destroy_debug_utils_messenger(VulkanInstance, Debug_Messenger);
	VulkanInstance.destroy(nullptr);
	delete window; 
}