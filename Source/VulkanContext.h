#pragma once
#include <vulkan/vulkan.hpp>
#include "VkBootstrap.h"
#include <iostream>
#include <GLFW/glfw3.h>
#include "../Source/Window.h"
#include "BufferManager.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

class VulkanContext
{
public:

	VulkanContext(Window& Window);

	void InitVulkan();
	void SelectGPU_CreateDevice();
	void createSurface();
	void create_swapchain();
	vk::Pipeline createGraphicsPipeline(vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo, vk::PipelineShaderStageCreateInfo ShaderStages[],          vk::PipelineVertexInputStateCreateInfo* vertexInputInfo, 
		                                vk::PipelineInputAssemblyStateCreateInfo* inputAssembleInfo,     vk::PipelineViewportStateCreateInfo viewportState,         vk::PipelineRasterizationStateCreateInfo rasterizerinfo, 
		                                vk::PipelineMultisampleStateCreateInfo multisampling,           vk::PipelineDepthStencilStateCreateInfo depthStencilState, vk::PipelineColorBlendStateCreateInfo colorBlend, 
		                                vk::PipelineDynamicStateCreateInfo DynamicState, vk::PipelineLayout& pipelineLayout, int numOfShaderStages = 2);

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
	///std::vector<vk::Image>       swapchainImages = {};
	//std::vector<vk::ImageView>   swapchainImageViews = {};

	std::vector<ImageData>   swapchainImageData = {};


	std::vector<vk::SurfaceFormatKHR> SurfaceFormat;

	Window window; 


	///////EXTENSIONS 
	PFN_vkCmdSetPolygonModeEXT                         vkCmdSetPolygonModeEXT            = nullptr;
	PFN_vkCreateAccelerationStructureKHR               vkCreateAccelerationStructureKHR  = nullptr;
	PFN_vkDestroyAccelerationStructureKHR              vkDestroyAccelerationStructureKHR = nullptr;
	PFN_vkCmdBuildAccelerationStructuresKHR            vkCmdBuildAccelerationStructuresKHR = nullptr;
	PFN_vkGetAccelerationStructureBuildSizesKHR        vkGetAccelerationStructureBuildSizesKHR = nullptr;
	PFN_vkGetAccelerationStructureDeviceAddressKHR     vkGetAccelerationStructureDeviceAddressKHR = nullptr;
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