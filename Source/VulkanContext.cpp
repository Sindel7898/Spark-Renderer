#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#ifndef VK_VERSION_1_0
#define VK_VERSION_1_0 1
#endif

#ifdef constant
#undef constant
#endif

#include "VulkanContext.h"

VulkanContext::VulkanContext(Window& Window, NvdiaDLSS_Intergration& _NvdiaDLSS_Intergration) : window(Window){
	DLSS_IntergrationRef = &_NvdiaDLSS_Intergration;
	DLSS_IntergrationRef->InitDLSS();

	InitVulkan();
	createSurface();
	SelectGPU_CreateDevice();
	create_swapchain();
}

void VulkanContext::InitVulkan()
{
	std::unique_ptr<vkb::InstanceBuilder> builderPtr;

	if (DLSS_IntergrationRef->sl_vkGetInstanceProcAddr != nullptr) {
		builderPtr = std::make_unique<vkb::InstanceBuilder>(DLSS_IntergrationRef->sl_vkGetInstanceProcAddr);
	}
	else {
		builderPtr = std::make_unique<vkb::InstanceBuilder>();
	}


	auto inst_ret = builderPtr->set_app_name(" Vulkan Application")
		.request_validation_layers(enableValidationLayers)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();

	if (!inst_ret)
	{
		throw std::runtime_error("Failed to create Vulkan instance: " + inst_ret.error().message());
	}

	VKB_Instance = inst_ret.value();
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
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
		VK_KHR_RAY_QUERY_EXTENSION_NAME,
	};

	vkb::PhysicalDeviceSelector selector{ VKB_Instance };

	   auto physicalDeviceResult = selector
		.set_minimum_version(1, 4)
		.set_required_features(deviceFeatures)
		.add_required_extensions(deviceExtensions)
		.set_surface(surface)
		.select();


	if (!physicalDeviceResult)
	{
		throw std::runtime_error("Failed to select a suitable physical device!" + physicalDeviceResult.error().message());
	}

	vkb::PhysicalDevice physicalDevice = physicalDeviceResult.value();

	VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
	rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
	rayQueryFeatures.rayQuery = VK_TRUE;

	vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationFeatures{};
	accelerationFeatures.accelerationStructure = vk::True;
	accelerationFeatures.pNext = &rayQueryFeatures;

	vk::PhysicalDeviceRayTracingPipelineFeaturesKHR   rtPipeLineFeatures{};
	rtPipeLineFeatures.rayTracingPipeline = vk::True;
	rtPipeLineFeatures.pNext = &accelerationFeatures;

	vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3Features{};
	extendedDynamicState3Features.extendedDynamicState3PolygonMode = vk::True;
	extendedDynamicState3Features.pNext = &rtPipeLineFeatures;

	vk::PhysicalDeviceVulkan12Features features_1_2{};
	features_1_2.sType = vk::StructureType::ePhysicalDeviceVulkan12Features;
	features_1_2.bufferDeviceAddress = vk::True;
	features_1_2.descriptorIndexing = vk::True;
	features_1_2.bufferDeviceAddress = vk::True;
	features_1_2.descriptorBindingPartiallyBound = vk::True;
	features_1_2.runtimeDescriptorArray = vk::True;
	features_1_2.shaderSampledImageArrayNonUniformIndexing = vk::True;
	features_1_2.pNext = &extendedDynamicState3Features;

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
	
	///

	vk::PhysicalDeviceAccelerationStructurePropertiesKHR AccelerationStructureProperties{};
	AccelerationStructureProperties.pNext = nullptr;

	RayTracingPipelineProperties.pNext = &AccelerationStructureProperties;

	vk::PhysicalDeviceProperties2 prop2{};
	prop2.pNext = &RayTracingPipelineProperties;

	PhysicalDevice.getProperties2(&prop2);

	if (!PhysicalDevice.getFeatures().samplerAnisotropy) {
		throw std::runtime_error("Anisotropic filtering is not supported on this device!");
	}

	//Information about queues
	graphicsQueue = VKB_Device.get_queue(vkb::QueueType::graphics).value();
	presentQueue = VKB_Device.get_queue(vkb::QueueType::present).value();
	graphicsQueueFamilyIndex = VKB_Device.get_queue_index(vkb::QueueType::graphics).value();

	if (DLSS_IntergrationRef) {
		DLSS_IntergrationRef->RegisterVulkanDevice(
			VulkanInstance,
			PhysicalDevice,
			LogicalDevice,
			graphicsQueueFamilyIndex,
			graphicsQueueFamilyIndex
		);
	}


	vkCmdSetPolygonModeEXT                      = (PFN_vkCmdSetPolygonModeEXT) vkGetDeviceProcAddr(LogicalDevice, "vkCmdSetPolygonModeEXT");
	vkCreateAccelerationStructureKHR            = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(LogicalDevice, "vkCreateAccelerationStructureKHR");
	vkDestroyAccelerationStructureKHR           = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(LogicalDevice, "vkDestroyAccelerationStructureKHR");
	vkCmdBuildAccelerationStructuresKHR         = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(LogicalDevice, "vkCmdBuildAccelerationStructuresKHR");
	vkGetAccelerationStructureBuildSizesKHR     = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(LogicalDevice, "vkGetAccelerationStructureBuildSizesKHR");
	vkGetAccelerationStructureDeviceAddressKHR  = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(LogicalDevice, "vkGetAccelerationStructureDeviceAddressKHR");
	vkCreateRayTracingPipelinesKHR              = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(LogicalDevice, "vkCreateRayTracingPipelinesKHR");
	vkGetRayTracingShaderGroupHandlesKHR        = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(LogicalDevice, "vkGetRayTracingShaderGroupHandlesKHR");
	vkCmdTraceRaysKHR                           = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(LogicalDevice, "vkCmdTraceRaysKHR");
	vkSetDebugUtilsObjectNameEXT                = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(LogicalDevice, "vkSetDebugUtilsObjectNameEXT");
	vkCmdBeginDebugUtilsLabelEXT                = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(LogicalDevice, "vkCmdBeginDebugUtilsLabelEXT");
	vkCmdEndDebugUtilsLabelEXT                  = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(LogicalDevice, "vkCmdEndDebugUtilsLabelEXT");


	HMODULE slModule = GetModuleHandleA("sl.interposer.dll");

	if (slModule)
	{
		// 1. Get the Interposer's GetDeviceProcAddr
		PFN_vkGetDeviceProcAddr slGetDeviceProcAddr =
			(PFN_vkGetDeviceProcAddr)GetProcAddress(slModule, "vkGetDeviceProcAddr");

		if (slGetDeviceProcAddr)
		{
			// 2. Use it to fetch the Hooked Present/Acquire functions
			slQueuePresent = (PFN_vkQueuePresentKHR)slGetDeviceProcAddr(LogicalDevice, "vkQueuePresentKHR");
			slAcquireNextImage = (PFN_vkAcquireNextImageKHR)slGetDeviceProcAddr(LogicalDevice, "vkAcquireNextImageKHR");

			std::cout << "Streamline Hooks Loaded Successfully." << std::endl;
		}
		else
		{
			std::cerr << "[Error] Could not find vkGetDeviceProcAddr in sl.interposer.dll" << std::endl;
		}
	}
	else
	{
		std::cerr << "[Error] sl.interposer.dll is not loaded!" << std::endl;
	}

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


	swapchainformat = vk::Format::eB8G8R8A8Unorm;
	                    

	vk::SurfaceFormatKHR format;
        format.format = swapchainformat;
	    format.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
	
		int Width  = 0;
		int Height = 0;

		glfwGetFramebufferSize(window.GetWindow(), &Width, &Height);


	vkb::Swapchain vkbswapChain = swapChainBuilder
		.set_desired_format(format)
		.set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
		.set_desired_extent(Width, Height)
		.add_image_usage_flags(
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			VK_IMAGE_USAGE_STORAGE_BIT |
			VK_IMAGE_USAGE_SAMPLED_BIT
		)
		.build()
		.value();


	
	swapchainExtent = vkbswapChain.extent;
	swapChain = vkbswapChain.swapchain;
	
	auto imageVector = vkbswapChain.get_images().value(); 

    for (auto& image : imageVector)
    {
	   vk::Image CastedImage = static_cast<vk::Image>(image);

	   ImageData NewImageData;
	   NewImageData.image = CastedImage;

	   swapchainImageData.push_back(NewImageData);
    }
	

	auto imageViews = vkbswapChain.get_image_views().value();

	for (int i = 0; i < swapchainImageData.size(); i++)
	{
		swapchainImageData[i].imageView = imageViews[i];
	}
}


vk::Format VulkanContext::FindCompatableDepthFormat()
{
	const std::vector<vk::Format> candidates = {
	   vk::Format::eD32Sfloat,
	   vk::Format::eD32SfloatS8Uint,
	   vk::Format::eD16Unorm,
	   vk::Format::eD24UnormS8Uint,   
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

void VulkanContext::ResetTemporalAccumilation()
{
	AccumilationCount = 0;
}

void VulkanContext::destroy_swapchain()
{

	for (size_t i = 0; i < swapchainImageData.size(); i++)
	{
		LogicalDevice.destroyImageView(swapchainImageData[i].imageView, nullptr);
	}
   
	LogicalDevice.destroySwapchainKHR(swapChain, nullptr);

	swapchainImageData.clear();

}

void VulkanContext::CleanUp()
{

	for (auto& imagedata : swapchainImageData) {
		LogicalDevice.destroyImageView(imagedata.imageView);
					 
	}				

	if (DLSS_IntergrationRef) {
		DLSS_IntergrationRef->CleanUp();
	}

	if (swapChain) {
		LogicalDevice.destroySwapchainKHR(swapChain);
		swapChain = nullptr;
	}				 
					 
	if (surface) {
		VulkanInstance.destroySurfaceKHR(surface);
		surface = nullptr;
	}	
		
	if (LogicalDevice)
	{	
		LogicalDevice.destroy();
	}	
		
	if (VulkanInstance)
	{	
		VulkanInstance.destroy();
	}

	PhysicalDevice = nullptr;
	graphicsQueue = nullptr;
	presentQueue = nullptr;
	swapchainImageData.clear();
	SurfaceFormat.clear();

}

VulkanContext::~VulkanContext()
{
	CleanUp();
}