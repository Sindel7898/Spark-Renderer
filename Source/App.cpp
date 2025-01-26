#include "App.h"
#include <set>


void App::Run()
{
	MainLoop();

}

void App::InitVulkan()
{
	CreateInstance();
	createSurface();
	SelectPhysicalDevice();
	createLogicalDevice();
}

void App::MainLoop()
{
	while (!window.shouldClose())
	{
		glfwPollEvents();
	}
}

void App::CleanUp()
{
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
	vkDestroyDevice(device, nullptr);

}

void App::CreateInstance()
{
	//Applicaiton info
	VkApplicationInfo Appinfo{};
	Appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	Appinfo.pApplicationName = "Hello Triangle";
	Appinfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	Appinfo.pEngineName = "No Engine";
	Appinfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	Appinfo.apiVersion = VK_API_VERSION_1_0;
    ///////////////////////////////////////////////////////////////
	
	//get Extensios from glfw
	uint32_t ExtensionCount = 0;
	const char** GlfW_ExtensionName;
	GlfW_ExtensionName = glfwGetRequiredInstanceExtensions(&ExtensionCount);
   
	// Instance Creation Info
	VkInstanceCreateInfo Createinfo{};
	Createinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	Createinfo.pApplicationInfo = &Appinfo;
	Createinfo.enabledExtensionCount = ExtensionCount;
	Createinfo.ppEnabledExtensionNames = GlfW_ExtensionName;
	
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	if (enableValidationLayers && checkValidationLayerSupport()) {
		Createinfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayerToUse.size());
		Createinfo.ppEnabledLayerNames = ValidationLayerToUse.data();
	}
	else {
		Createinfo.enabledLayerCount = 0;
	}

	//////////////////////////////////////////////////////////////
	VkResult result = vkCreateInstance(&Createinfo, nullptr, &instance);

	if (result!= VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
     }

	//////////////////////////////////////////////////////////////
}

bool App::checkValidationLayerSupport()
{
	uint32_t LayerCount;
	vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(LayerCount);

	vkEnumerateInstanceLayerProperties(&LayerCount, availableLayers.data());

	for (const char* LayerName : ValidationLayerToUse)
	{
		  bool layerFound = false;

		  for (const auto& layerProperties : availableLayers) {

			  if (strcmp(LayerName, layerProperties.layerName) == 0) {

				  layerFound = true;
				  break;
			  }
		  }

		  if (!layerFound) {
			  return false;
		  }
	}

	return true;
}

void App::SelectPhysicalDevice()
{
	uint32_t DeviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &DeviceCount, nullptr);

	if (DeviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice>PhysicalDeviceList(DeviceCount);
	vkEnumeratePhysicalDevices(instance, &DeviceCount, PhysicalDeviceList.data());

	for (auto device : PhysicalDeviceList) {

			VkPhysicalDeviceProperties PhysicalDeviceProperties;
			vkGetPhysicalDeviceProperties(device, &PhysicalDeviceProperties);

				if (PhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && IsDeviceSuitable(device)) {
				PhysicalDevice = device;
				VkPhysicalDeviceProperties deviceProperties;
				vkGetPhysicalDeviceProperties(PhysicalDevice, &deviceProperties);
				std::cout << "Selected GPU: " << deviceProperties.deviceName << std::endl;
				break;
			}
			
		
	}

	if (PhysicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}
void App::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(PhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> CreatedQueuesInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
	float queuePriority = 1.0f;

	for (uint32_t Family : uniqueQueueFamilies) {

		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		CreatedQueuesInfos.push_back(queueCreateInfo);
	}
	
	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = CreatedQueuesInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(CreatedQueuesInfos.size());
	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = 0;

	if (enableValidationLayers && checkValidationLayerSupport()) {

		createInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayerToUse.size());
		createInfo.ppEnabledLayerNames = ValidationLayerToUse.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(PhysicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {

		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily .value(), 0, &presentQueue);

}
void App::createSurface()
{
	//manual creation of a surface 
	//VkWin32SurfaceCreateInfoKHR createinfo{};
	//createinfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	//createinfo.hwnd = glfwGetWin32Window(window.GetWindow());
	//createinfo.hinstance = GetModuleHandle(nullptr);
	//vkCreateWin32SurfaceKHR(instance, &createinfo, nullptr, &surface);

	if (glfwCreateWindowSurface(instance, window.GetWindow(), nullptr, &surface) !=VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");

	}
}
bool App::IsDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = findQueueFamilies(device);
	return indices.isComplete();

}

QueueFamilyIndices App::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;


	for (auto& QueueFamily : queueFamilies) {

		if (QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}
	return indices;
}

