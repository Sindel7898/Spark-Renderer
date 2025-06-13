#include "VulkanContext.h"

VulkanContext::VulkanContext(Window& Window) : window(Window){
	InitVulkan();
	createSurface();
	SelectGPU_CreateDevice();
	create_swapchain();
}

void VulkanContext::InitVulkan()
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
		throw std::runtime_error("Failed to create Vulkan instance: " + inst_ret.error().message());
	}

	VKB_Instance = inst_ret.value();

	// Store the Vulkan instance and debug messenger
	VulkanInstance = VKB_Instance.instance;
	Debug_Messenger = VKB_Instance.debug_messenger;


}

void VulkanContext::SelectGPU_CreateDevice()
{
	vk::PhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.fillModeNonSolid = VK_TRUE;

	std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
	    VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
	};

	vkb::PhysicalDeviceSelector selector{ VKB_Instance };

	   auto physicalDeviceResult = selector
		.set_minimum_version(1, 3)
		.set_required_features(deviceFeatures)
		.add_required_extensions(deviceExtensions)
		.set_surface(surface)
		.select();

	if (!physicalDeviceResult)
	{
		throw std::runtime_error("Failed to select a suitable physical device!" + physicalDeviceResult.error().message());
	}

	vkb::PhysicalDevice physicalDevice = physicalDeviceResult.value();


	vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3Features{};
	extendedDynamicState3Features.extendedDynamicState3PolygonMode = vk::True;
	extendedDynamicState3Features.pNext = nullptr;

	vk::PhysicalDeviceVulkan12Features features_1_2{};
	features_1_2.sType = vk::StructureType::ePhysicalDeviceVulkan12Features;
	features_1_2.bufferDeviceAddress = vk::True;
	features_1_2.descriptorIndexing  = vk::True;
	features_1_2.bufferDeviceAddress = vk::True;
	features_1_2.pNext = &extendedDynamicState3Features;
	//Select Required Vulkan featuers 
	vk::PhysicalDeviceVulkan13Features features_1_3{};
	features_1_3.sType = vk::StructureType::ePhysicalDeviceVulkan13Features;
	features_1_3.dynamicRendering = vk::True;
	features_1_3.synchronization2 = vk::True;
	features_1_3.pNext = &features_1_2;

	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	auto deviceResult = deviceBuilder
		.add_pNext(&features_1_3)
		.build();

	if (!deviceResult) {
		throw std::runtime_error("Failed to create logical device: " + deviceResult.error().message());
	}

	vkb::Device VKB_Device = deviceResult.value();

	LogicalDevice = VKB_Device.device;


	if (LogicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Failed to create logical device!");
	}

	//Print out name of GPU being used
	PhysicalDevice = physicalDevice.physical_device;
	std::cout << "GPU: " << std::string_view(PhysicalDevice.getProperties().deviceName) << std::endl;


	if (!PhysicalDevice.getFeatures().samplerAnisotropy) {
		throw std::runtime_error("Anisotropic filtering is not supported on this device!");
	}

	//Information about queues
	graphicsQueue = VKB_Device.get_queue(vkb::QueueType::graphics).value();
	presentQueue = VKB_Device.get_queue(vkb::QueueType::present).value();
	graphicsQueueFamilyIndex = VKB_Device.get_queue_index(vkb::QueueType::graphics).value();


	vkCmdSetPolygonModeEXT = reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(vkGetDeviceProcAddr(LogicalDevice, "vkCmdSetPolygonModeEXT"));
	
	
	/*vkCreateAccelerationStructureKHR        = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(LogicalDevice, "vkCreateAccelerationStructureKHR"));
	vkDestroyAccelerationStructureKHR       = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(LogicalDevice, "vkDestroyAccelerationStructureKHR"));
	vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(LogicalDevice, "vkGetAccelerationStructureBuildSizesKHR"));
	vkCmdBuildAccelerationStructuresKHR     = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(LogicalDevice, "vkCmdBuildAccelerationStructuresKHR"));
	vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(LogicalDevice, "vkGetAccelerationStructureDeviceAddressKHR"));*/

}

void VulkanContext::createSurface()
{

	if (glfwCreateWindowSurface(VulkanInstance, window.GetWindow(), nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface)) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface!");
	}

}

void VulkanContext::create_swapchain()
{
	
	vkb::SwapchainBuilder swapChainBuilder(PhysicalDevice, LogicalDevice, surface);


	swapchainformat = vk::Format::eB8G8R8A8Srgb;
	                    

	vk::SurfaceFormatKHR format;
        format.format = swapchainformat;
	    format.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
	
		int Width  = 0;
		int Height = 0;

		glfwGetFramebufferSize(window.GetWindow(), &Width, &Height);


	vkb::Swapchain vkbswapChain = swapChainBuilder
		.set_desired_format(format)
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(Width, Height)
		.add_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();


	
	swapchainExtent = vkbswapChain.extent;
	swapChain = vkbswapChain.swapchain;
	
	auto imageVector = vkbswapChain.get_images().value(); 

	swapchainImages = std::vector<vk::Image>(imageVector.begin(), imageVector.end());


	auto imageViews = vkbswapChain.get_image_views().value();

	swapchainImageViews = std::vector<vk::ImageView>(imageViews.begin(), imageViews.end());
}

vk::Pipeline VulkanContext::createGraphicsPipeline(vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo, vk::PipelineShaderStageCreateInfo ShaderStages[], 
	                                               vk::PipelineVertexInputStateCreateInfo vertexInputInfo,         vk::PipelineInputAssemblyStateCreateInfo inputAssembleInfo,
	                                               vk::PipelineViewportStateCreateInfo viewportState,              vk::PipelineRasterizationStateCreateInfo rasterizerinfo,
	                                               vk::PipelineMultisampleStateCreateInfo multisampling,           vk::PipelineDepthStencilStateCreateInfo depthStencilState,   
	                                               vk::PipelineColorBlendStateCreateInfo colorBlend,               vk::PipelineDynamicStateCreateInfo DynamicState, 
	                                               vk::PipelineLayout&  pipelineLayout, int numOfShaderStages)
{

	vk::GraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.pNext = &pipelineRenderingCreateInfo; // Add this line
	pipelineInfo.stageCount = numOfShaderStages;
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


	vk::Pipeline graphicsPipeline;
	vk::Result result = LogicalDevice.createGraphicsPipelines(nullptr, 1, &pipelineInfo, nullptr, &graphicsPipeline);

	if (result != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	return graphicsPipeline;
}

vk::Format VulkanContext::FindCompatableDepthFormat()
{
	const std::vector<vk::Format> candidates = {
	   vk::Format::eD32SfloatS8Uint,  
	   vk::Format::eD24UnormS8Uint,   
	   vk::Format::eD32Sfloat,        
	   vk::Format::eD16Unorm,         
	   vk::Format::eD16UnormS8Uint   
	};

	for (const auto& format : candidates)
	{
		vk::FormatProperties props = PhysicalDevice.getFormatProperties(format);

		if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
		{
			return format;
		}
	}
}


void VulkanContext::destroy_swapchain()
{

	for (size_t i = 0; i <swapchainImageViews.size(); i++)
	{
		LogicalDevice.destroyImageView(swapchainImageViews[i], nullptr);
	}
   
	LogicalDevice.destroySwapchainKHR(swapChain, nullptr);

	swapchainImageViews.clear();

}
