#pragma once
#include <vulkan/vulkan.hpp>
#include "VkBootstrap.h"
#include <iostream>
#include <GLFW/glfw3.h>
#include "../Source/Window.h"
#include "BufferManager.h"

class BufferManager;
const int MAX_FRAMES_IN_FLIGHT = 2;

class VulkanContext
{
public:

	VulkanContext(Window& Window);

	void InitVulkan();
	void SelectGPU_CreateDevice();
	void createSurface();
	void create_swapchain();
	
	vk::Format FindCompatableDepthFormat();

	void destroy_swapchain();

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else 
	const bool enableValidationLayers = true;
#endif

	vkb::Instance VKB_Instance;

	vk::Device                 LogicalDevice = nullptr;
	vk::PhysicalDevice         PhysicalDevice = nullptr;
	vk::Instance               VulkanInstance = nullptr;
	vk::DebugUtilsMessengerEXT Debug_Messenger = nullptr;
	vk::SurfaceKHR             surface = nullptr;

	vk::Queue                  graphicsQueue;
	vk::Queue                  presentQueue;
	uint32_t                   graphicsQueueFamilyIndex;

	vk::Format                 swapchainformat;
	vk::Extent2D               swapchainExtent = { 0, 0 };
	vk::SwapchainKHR           swapChain = nullptr;

	void ResetTemporalAccumilation();
	int AccumilationCount = 1;

	std::vector<ImageData>   swapchainImageData = {};


	std::vector<vk::SurfaceFormatKHR> SurfaceFormat;

	Window window; 
	vk::PhysicalDeviceRayTracingPipelinePropertiesKHR RayTracingPipelineProperties{};


	///////EXTENSIONS 
	PFN_vkCmdSetPolygonModeEXT                         vkCmdSetPolygonModeEXT            = nullptr;
	PFN_vkCreateAccelerationStructureKHR               vkCreateAccelerationStructureKHR  = nullptr;
	PFN_vkDestroyAccelerationStructureKHR              vkDestroyAccelerationStructureKHR = nullptr;
	PFN_vkCmdBuildAccelerationStructuresKHR            vkCmdBuildAccelerationStructuresKHR = nullptr;
	PFN_vkGetAccelerationStructureBuildSizesKHR        vkGetAccelerationStructureBuildSizesKHR = nullptr;
	PFN_vkGetAccelerationStructureDeviceAddressKHR     vkGetAccelerationStructureDeviceAddressKHR = nullptr;
	PFN_vkCreateRayTracingPipelinesKHR                 vkCreateRayTracingPipelinesKHR = nullptr;
	PFN_vkGetRayTracingShaderGroupHandlesKHR           vkGetRayTracingShaderGroupHandlesKHR = nullptr;
	PFN_vkCmdTraceRaysKHR                              vkCmdTraceRaysKHR = nullptr;
	PFN_vkSetDebugUtilsObjectNameEXT                   vkSetDebugUtilsObjectNameEXT = nullptr;
	PFN_vkCmdBeginDebugUtilsLabelEXT                   vkCmdBeginDebugUtilsLabelEXT = nullptr;
	PFN_vkCmdEndDebugUtilsLabelEXT                     vkCmdEndDebugUtilsLabelEXT = nullptr;


};

static inline void VulkanContextDeleter(VulkanContext* vulkanContext)
{
	if (vulkanContext)
	{
		for (auto& imagedata : vulkanContext->swapchainImageData) {
			vulkanContext->LogicalDevice.destroyImageView(imagedata.imageView);

		}

		if (vulkanContext->swapChain) {
			vulkanContext->LogicalDevice.destroySwapchainKHR(vulkanContext->swapChain);
			vulkanContext->swapChain = nullptr;
		}

		if (vulkanContext->surface) {
			vulkanContext->VulkanInstance.destroySurfaceKHR(vulkanContext->surface);
			vulkanContext->surface = nullptr;
		}

		if (vulkanContext->LogicalDevice)
		{
			vulkanContext->LogicalDevice.destroy();
		}

		if (vulkanContext->VulkanInstance)
		{
			vulkanContext->VulkanInstance.destroy();
		}

		vulkanContext->PhysicalDevice = nullptr;
		vulkanContext->graphicsQueue = nullptr;
		vulkanContext->presentQueue = nullptr;
		vulkanContext->swapchainImageData.clear();
		vulkanContext->SurfaceFormat.clear();

		delete vulkanContext;
	}
}