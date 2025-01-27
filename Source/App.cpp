#include "App.h"
#include <set>


void App::Run()
{
	MainLoop();

}

void App::Initialisation()
{
	InitVulkan();
	createSurface();
	create_swapchain(800, 800);
}

void App::MainLoop()
{
	while (!window.shouldClose())
	{
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

	VKB_Instance = inst_ret.value();

	//grab the instance 
	Instance = VKB_Instance.instance;
	Debug_Messenger = VKB_Instance.debug_messenger;
}

void App::createSurface()
{
	
	glfwCreateWindowSurface(Instance, window.GetWindow(), nullptr, &surface);

	VkPhysicalDeviceVulkan13Features Features1_3{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	Features1_3.dynamicRendering = true;
	Features1_3.synchronization2 = true;

	VkPhysicalDeviceVulkan12Features Features1_2{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	Features1_2.bufferDeviceAddress = true;
	Features1_2.descriptorIndexing = true;

	vkb::PhysicalDeviceSelector selector{ VKB_Instance };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(Features1_3)
		.set_required_features_12(Features1_2)
		.set_surface(surface)
		.select()
		.value();

	vkb::DeviceBuilder deviceBuilder{ physicalDevice };
	VKB_Device = deviceBuilder.build().value();

	device = VKB_Device.device;
	PhysicalDevice = physicalDevice.physical_device;

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(physicalDevice.physical_device, &properties);
	

	std::cout << properties.deviceName << std::endl;
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


void App::CleanUp()
{
	vkDestroySurfaceKHR(Instance, surface, nullptr);
	vkb::destroy_instance(VKB_Instance);
	vkDestroyDevice(device, nullptr);

}