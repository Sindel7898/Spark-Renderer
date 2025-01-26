#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include "Window.h"
#include "vector"
#include <stdexcept>
#include <iostream>
#include <optional>

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

class App
{
public:

	void Run();
	void InitVulkan();
	void MainLoop();
	void SelectPhysicalDevice();
	void createLogicalDevice();
	void createSurface();

	void CleanUp();

	// Vulkan Init Tasks
	void CreateInstance();
	bool checkValidationLayerSupport();
	bool IsDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

 private:

	 Window window{ 800,800,"Vulkan Window" };

	 const std::vector<const char*> ValidationLayerToUse = {
	    "VK_LAYER_KHRONOS_validation"
	 };

#ifdef NDEBUG
	 const bool enableValidationLayers = false;
#else 
	 const bool enableValidationLayers = true;
#endif
	 VkDevice device;
	 VkInstance instance;
	 VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
	 VkQueue graphicsQueue;
	 VkSurfaceKHR surface;
	 VkQueue presentQueue;

};

