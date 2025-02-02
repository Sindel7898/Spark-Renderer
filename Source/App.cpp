#include "App.h"

void App::Initialisation()
{
	window = new Window(800, 800, "Vulkan Window"); // For pointer

	InitVulkan();
	createSurface();
	create_swapchain(800, 800);
}

void App::MainLoop()
{
	while (!window->shouldClose())
	{
		std::cout << "Running Main Loop..." << std::endl;
		glfwPollEvents();
	}

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
	Instance = VKB_Instance.instance;
	Debug_Messenger = VKB_Instance.debug_messenger;
}

void App::createSurface()
{
	
	if (glfwCreateWindowSurface(Instance, window->GetWindow(), nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface!");
	}

	VkPhysicalDeviceVulkan13Features features_1_3{};
	features_1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features_1_3.dynamicRendering = VK_TRUE;
	features_1_3.synchronization2 = VK_TRUE;

	VkPhysicalDeviceVulkan12Features features_1_2{};
	features_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
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

	vkb::Device VKB_Device = deviceBuilder.build().value();

	device = VKB_Device.device;

	if (device == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Failed to select a suitable device!");
	}

	PhysicalDevice = physicalDevice.physical_device;

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(PhysicalDevice, &properties);
	
	std::cout << "GPU: " << std::string(properties.deviceName) << std::endl;
}


void App::create_swapchain(uint32_t width, uint32_t height)
{
	vkb::SwapchainBuilder swapChainBuilder(PhysicalDevice, device, surface);
	swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	VkSurfaceFormatKHR format;
	format.format = swapchainImageFormat;
	format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

	vkb::Swapchain vkbswapChain = swapChainBuilder
		.set_desired_format(format)
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	swapchainExtent = vkbswapChain.extent;

	swapChain = vkbswapChain.swapchain;
	swapchainImages = vkbswapChain.get_images().value();
	swapchainImageViews = vkbswapChain.get_image_views().value();


}

void App::Run()
{
	MainLoop();

}



void App::destroy_swapchain()
{
	vkDestroySwapchainKHR(device, swapChain, nullptr);

	for (size_t i = 0; i < swapchainImageViews.size(); i++)
	{
		vkDestroyImageView(device, swapchainImageViews[i], nullptr);
	}

	swapchainImageViews.clear();
	swapchainImages.clear();
}


void App::CleanUp()
{
	destroy_swapchain();
	vkDestroySurfaceKHR(Instance, surface, nullptr);
	vkDestroyDevice(device, nullptr);
	vkb::destroy_debug_utils_messenger(Instance, Debug_Messenger);
	vkDestroyInstance(Instance, nullptr);
	delete window; 
}