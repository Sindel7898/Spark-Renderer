#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>

#ifndef VK_VERSION_1_0
#define VK_VERSION_1_0 1
#endif

#ifdef constant
#undef constant
#endif

#include "VulkanContext.h"
#include <vector>
#include <string>


uint32_t FindQueueFamily(const std::vector<VkQueueFamilyProperties>& families, VkQueueFlagBits requiredFlag) {

    for (uint32_t i = 0; i < static_cast<uint32_t>(families.size()); i++) {

        if (families[i].queueFlags & requiredFlag) {
            return i;
        }
    }
    throw std::runtime_error("Failed to find required queue family!");
}

VulkanContext::VulkanContext(Window& Window, NvdiaDLSS_Intergration& _NvdiaDLSS_Intergration) : window(Window) {
    DLSS_IntergrationRef = &_NvdiaDLSS_Intergration;
    InitVulkan();
    createSurface();
    SelectGPU_CreateDevice();
    create_swapchain();
}

void VulkanContext::InitVulkan()
{
    std::unique_ptr<vkb::InstanceBuilder> builderPtr = std::make_unique<vkb::InstanceBuilder>();

    auto inst_ret = builderPtr->set_app_name("Vulkan Application")
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
        VK_KHR_RAY_QUERY_EXTENSION_NAME,
        VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME
    };

    std::vector<const char*> dInstanceExtensions;

	DLSS_IntergrationRef->requiredExtensions(dInstanceExtensions, deviceExtensions);

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
    PhysicalDevice = physicalDevice.physical_device;

    std::cout << "GPU: " << std::string_view(PhysicalDevice.getProperties().deviceName) << std::endl;

    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
    rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    rayQueryFeatures.rayQuery = VK_TRUE;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationFeatures{};
    accelerationFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    accelerationFeatures.accelerationStructure = VK_TRUE;
    accelerationFeatures.pNext = &rayQueryFeatures;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipeLineFeatures{};
    rtPipeLineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rtPipeLineFeatures.rayTracingPipeline = VK_TRUE;
    rtPipeLineFeatures.pNext = &accelerationFeatures;

    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3Features{};
    extendedDynamicState3Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
    extendedDynamicState3Features.extendedDynamicState3PolygonMode = VK_TRUE;
    extendedDynamicState3Features.pNext = &rtPipeLineFeatures;

    VkPhysicalDeviceVulkan12Features features_1_2{};
    features_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features_1_2.bufferDeviceAddress = VK_TRUE;
    features_1_2.descriptorIndexing = VK_TRUE;
    features_1_2.descriptorBindingPartiallyBound = VK_TRUE;
    features_1_2.runtimeDescriptorArray = VK_TRUE;
    features_1_2.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    features_1_2.pNext = &extendedDynamicState3Features;

    VkPhysicalDeviceVulkan13Features features_1_3{};
    features_1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features_1_3.dynamicRendering = VK_TRUE;
    features_1_3.synchronization2 = VK_TRUE;
    features_1_3.pNext = &features_1_2;


    std::vector<VkQueueFamilyProperties> queueFamilies = physicalDevice.get_queue_families();

    graphicsQueueFamilyIndex = FindQueueFamily(queueFamilies, VK_QUEUE_GRAPHICS_BIT);
    computeQueueFamilyIndex = FindQueueFamily(queueFamilies, VK_QUEUE_COMPUTE_BIT);

    float queuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    VkDeviceQueueCreateInfo graphicsQueueInfo{};
    graphicsQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphicsQueueInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    graphicsQueueInfo.queueCount = 1;
    graphicsQueueInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(graphicsQueueInfo);

    if (computeQueueFamilyIndex != graphicsQueueFamilyIndex) {

        VkDeviceQueueCreateInfo computeQueueInfo{};
        computeQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        computeQueueInfo.queueFamilyIndex = computeQueueFamilyIndex;
        computeQueueInfo.queueCount = 1;
        computeQueueInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(computeQueueInfo);
    }

    std::vector<std::string> extensionStrings = physicalDevice.get_extensions();
    std::vector<const char*> extensionCStrings;
    for (const auto& ext : extensionStrings) {
        extensionCStrings.push_back(ext.c_str());
    }

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = &features_1_3;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensionCStrings.size());
    deviceCreateInfo.ppEnabledExtensionNames = extensionCStrings.data();

    VkPhysicalDeviceFeatures enabledFeatures = physicalDevice.features;
    deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

    if (vkCreateDevice(PhysicalDevice, &deviceCreateInfo, nullptr, (VkDevice*)&LogicalDevice) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create logical device!");
    }

    vkGetDeviceQueue(LogicalDevice, graphicsQueueFamilyIndex, 0, (VkQueue*)&graphicsQueue);
    vkGetDeviceQueue(LogicalDevice, computeQueueFamilyIndex, 0, (VkQueue*)&presentQueue);

    vkCmdSetPolygonModeEXT = (PFN_vkCmdSetPolygonModeEXT)vkGetDeviceProcAddr(LogicalDevice, "vkCmdSetPolygonModeEXT");
    vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(LogicalDevice, "vkCreateAccelerationStructureKHR");
    vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(LogicalDevice, "vkDestroyAccelerationStructureKHR");
    vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(LogicalDevice, "vkCmdBuildAccelerationStructuresKHR");
    vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(LogicalDevice, "vkGetAccelerationStructureBuildSizesKHR");
    vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(LogicalDevice, "vkGetAccelerationStructureDeviceAddressKHR");
    vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(LogicalDevice, "vkCreateRayTracingPipelinesKHR");
    vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(LogicalDevice, "vkGetRayTracingShaderGroupHandlesKHR");
    vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(LogicalDevice, "vkCmdTraceRaysKHR");
    vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(LogicalDevice, "vkSetDebugUtilsObjectNameEXT");
    vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(LogicalDevice, "vkCmdBeginDebugUtilsLabelEXT");
    vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(LogicalDevice, "vkCmdEndDebugUtilsLabelEXT");


    vk::PhysicalDeviceAccelerationStructurePropertiesKHR AccelerationStructureProperties{};
    AccelerationStructureProperties.pNext = nullptr;
    RayTracingPipelineProperties.pNext = &AccelerationStructureProperties;

    vk::PhysicalDeviceProperties2 prop2{};
    prop2.pNext = &RayTracingPipelineProperties;

    PhysicalDevice.getProperties2(&prop2);

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

    int Width = 0;
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
    return vk::Format::eD32Sfloat; // Default fallback
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