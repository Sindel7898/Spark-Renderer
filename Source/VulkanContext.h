#pragma once
#include "VkBootstrap.h"
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <GLFW/glfw3.h>
#include "../Source/Window.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

class VulkanContext
{
public:

	VulkanContext(Window& Window);

	void InitVulkan();
	void SelectGPU_CreateDevice();
	void createSurface();
	void create_swapchain();
	vk::Pipeline createGraphicsPipeline(vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo, vk::PipelineShaderStageCreateInfo ShaderStages[],          vk::PipelineVertexInputStateCreateInfo vertexInputInfo, 
		                                vk::PipelineInputAssemblyStateCreateInfo inputAssembleInfo,     vk::PipelineViewportStateCreateInfo viewportState,         vk::PipelineRasterizationStateCreateInfo rasterizerinfo, 
		                                vk::PipelineMultisampleStateCreateInfo multisampling,           vk::PipelineDepthStencilStateCreateInfo depthStencilState, vk::PipelineColorBlendStateCreateInfo colorBlend, 
		                                vk::PipelineDynamicStateCreateInfo DynamicState, vk::PipelineLayout& pipelineLayout);

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
	std::vector<vk::Image>       swapchainImages = {};
	std::vector<vk::ImageView>   swapchainImageViews = {};
	std::vector<vk::SurfaceFormatKHR> SurfaceFormat;

	Window window; 

	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
	PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
};

static inline void VulkanContextDeleter(VulkanContext* vulkanContext)
{
	if (vulkanContext)
	{
		for (auto& imageView : vulkanContext->swapchainImageViews) {
			vulkanContext->LogicalDevice.destroyImageView(imageView);
		}

		vulkanContext->swapchainImageViews.clear();

	/*	for (auto& Image : vulkanContext->swapchainImages)
		{
			vulkanContext->LogicalDevice.destroyImage(Image);
		}

		vulkanContext->swapchainImages.clear();*/

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
		vulkanContext->swapchainImages.clear();
		vulkanContext->SurfaceFormat.clear();

		delete vulkanContext;
	}
}