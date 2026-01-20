#pragma once
#include "NvdiaDLSS_Intergration.h"
#include "Camera.h" 
#include "structs.h"
#include"BufferManager.h"
#include"VulkanContext.h"
#include <iostream> 

static void NVSDK_CONV LogCallback(const char* message, NVSDK_NGX_Logging_Level loggingLevel, NVSDK_NGX_Feature sourceComponent)
{
    // You can filter by level here if needed (e.g., only show errors)
    std::cout << "[DLSS Output] " << message << std::endl;
}

void NvdiaDLSS_Intergration::initializePointers(BufferManager* bufferManager, VulkanContext* vulkanContext, Camera* camera)
{
    m_bufferManager = bufferManager;
    m_vulkanContext = vulkanContext;
    m_camera = camera;

    m_instance = m_vulkanContext->VulkanInstance;
    m_physicalDevice = m_vulkanContext->PhysicalDevice;
    m_device = m_vulkanContext->LogicalDevice;

    NVSDK_NGX_Application_Identifier appId{};
    appId.IdentifierType = NVSDK_NGX_Application_Identifier_Type_Application_Id;
    appId.v.ProjectDesc.EngineType = NVSDK_NGX_ENGINE_TYPE_CUSTOM;
    appId.v.ProjectDesc.ProjectId = "DLSSIntegration";
    appId.v.ProjectDesc.EngineVersion = "1.0.0";

    NVSDK_NGX_FeatureCommonInfo commonInfo = {};
    commonInfo.LoggingInfo.LoggingCallback = LogCallback;
    commonInfo.LoggingInfo.MinimumLoggingLevel = NVSDK_NGX_LOGGING_LEVEL_VERBOSE;
    commonInfo.LoggingInfo.DisableOtherLoggingSinks = false;

    NVSDK_NGX_Result initResult = NVSDK_NGX_VULKAN_Init(
        appId.v.ApplicationId,
        L".",
        m_instance,
        m_physicalDevice,
        m_device,
        nullptr,
        nullptr,
        &commonInfo
    );

    if (NVSDK_NGX_FAILED(initResult)) {
        throw std::runtime_error("Failed to initialize NVSDK NGX");
    }

    NVSDK_NGX_Result capResult = NVSDK_NGX_VULKAN_GetCapabilityParameters(&paramsDLSS_);
    if (NVSDK_NGX_FAILED(capResult)) {
        throw std::runtime_error("Failed to get NGX capability parameters");
    }
}

void NvdiaDLSS_Intergration::init(vk::CommandPool commandPool) {

    uint32_t displayWidth = m_vulkanContext->swapchainExtent.width;
    uint32_t displayHeight = m_vulkanContext->swapchainExtent.height;

    uint32_t renderWidth = displayWidth;
    uint32_t renderHeight = displayHeight;

    NVSDK_NGX_PerfQuality_Value dlssQuality = NVSDK_NGX_PerfQuality_Value_DLAA;

    int dlssAvailable = 0;
    paramsDLSS_->Get(NVSDK_NGX_Parameter_SuperSampling_Available, &dlssAvailable);
    if (!dlssAvailable) {
        throw std::runtime_error("DLSS not available on this hardware/driver.");
    }

    int dlssCreateFeatureFlags = NVSDK_NGX_DLSS_Feature_Flags_None;
    dlssCreateFeatureFlags |= NVSDK_NGX_DLSS_Feature_Flags_MVLowRes;
    dlssCreateFeatureFlags |= NVSDK_NGX_DLSS_Feature_Flags_DoSharpening;
    dlssCreateFeatureFlags |= NVSDK_NGX_DLSS_Feature_Flags_IsHDR;

    NVSDK_NGX_DLSS_Create_Params dlssCreateParams{
        .Feature = {
            .InWidth = renderWidth,
            .InHeight = renderHeight,
            .InTargetWidth = displayWidth,  
            .InTargetHeight = displayHeight,
            .InPerfQualityValue = dlssQuality,
        },
        .InFeatureCreateFlags = dlssCreateFeatureFlags,
    };

    auto commmandBuffer = m_bufferManager->CreateSingleUseCommandBuffer(commandPool);

    constexpr unsigned int creationNodeMask = 1;
    constexpr unsigned int visibilityNodeMask = 1;

    NVSDK_NGX_Result createDlssResult =
        NGX_VULKAN_CREATE_DLSS_EXT(commmandBuffer, creationNodeMask, visibilityNodeMask,
            &dlssFeatureHandle_, paramsDLSS_, &dlssCreateParams);

    if (createDlssResult != NVSDK_NGX_Result_Success)
    {
        throw std::runtime_error("Failed to create NVSDK NGX DLSS feature");
    }

    m_bufferManager->SubmitAndDestoyCommandBuffer(commandPool, commmandBuffer, m_vulkanContext->graphicsQueue);
}

void NvdiaDLSS_Intergration::render(VkCommandBuffer commandBuffer, ImageData InImage,
                                                                   GBuffer   inColorTexture,
                                                                   ImageData inDepthTexture,
                                                                   ImageData OutImage) 
{

    NVSDK_NGX_Resource_VK inColorResource = NVSDK_NGX_Create_ImageView_Resource_VK(
        InImage.imageView, InImage.image,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, VK_FORMAT_UNDEFINED,
        m_vulkanContext->swapchainExtent.width, m_vulkanContext->swapchainExtent.height, true);

    NVSDK_NGX_Resource_VK outColorResource = NVSDK_NGX_Create_ImageView_Resource_VK(
        OutImage.imageView, OutImage.image,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, VK_FORMAT_UNDEFINED,
        m_vulkanContext->swapchainExtent.width, m_vulkanContext->swapchainExtent.height, true);

    NVSDK_NGX_Resource_VK depthResource = NVSDK_NGX_Create_ImageView_Resource_VK(
        inDepthTexture.imageView, inDepthTexture.image,
        { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 }, VK_FORMAT_UNDEFINED,
        m_vulkanContext->swapchainExtent.width, m_vulkanContext->swapchainExtent.height, true);


    NVSDK_NGX_Resource_VK motionVectorResource = NVSDK_NGX_Create_ImageView_Resource_VK(
        inColorTexture.MotionVector.imageView, inColorTexture.MotionVector.image,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, VK_FORMAT_UNDEFINED,
        m_vulkanContext->swapchainExtent.width, m_vulkanContext->swapchainExtent.height, true);

    NVSDK_NGX_VK_DLSS_Eval_Params evalParams = {
        .Feature =
            {
                .pInColor = &inColorResource,
                .pInOutput = &outColorResource,
                .InSharpness = 1.0,
            },
        .pInDepth = &depthResource,
        .pInMotionVectors = &motionVectorResource,
        .InJitterOffsetX = m_camera->GetjitterInPixelSpace().x,
        .InJitterOffsetY = m_camera->GetjitterInPixelSpace().y,
        .InRenderSubrectDimensions =
            {
                .Width  = static_cast<unsigned int>(m_vulkanContext->swapchainExtent.width),
                .Height = static_cast<unsigned int>(m_vulkanContext->swapchainExtent.height),
            },
        .InReset = 0,
        .InMVScaleX = -1.0f * inColorResource.Resource.ImageViewInfo.Width,
        .InMVScaleY = -1.0f * inColorResource.Resource.ImageViewInfo.Height,
        .pInExposureTexture = nullptr,
    };

    NVSDK_NGX_Result result = NGX_VULKAN_EVALUATE_DLSS_EXT(
        commandBuffer, dlssFeatureHandle_, paramsDLSS_, &evalParams);


   if (result != NVSDK_NGX_Result_Success) {
       auto store = GetNGXResultAsString(result);
       throw std::runtime_error("DLSS Evaluation Failed");
   }
}



void NvdiaDLSS_Intergration::requiredExtensions(std::vector<const char*>& instanceExtensions, std::vector<const char*>& deviceExtensions)
{
    unsigned int instanceExtCount;
    const char** instanceExt;
    unsigned int deviceExtCount;
    const char** deviceExt;

    NVSDK_NGX_Result result = NVSDK_NGX_VULKAN_RequiredExtensions(
        &instanceExtCount, &instanceExt, &deviceExtCount, &deviceExt);

    if (NVSDK_NGX_FAILED(result)) {
        throw std::runtime_error("Failed to Find Required Extensions for DLSS");
    }

    // Add Instance Extensions
    for (unsigned int i = 0; i < instanceExtCount; ++i) {
        // Check for duplicates before adding
        bool found = false;
        for (const auto& existing : instanceExtensions) {
            if (strcmp(existing, instanceExt[i]) == 0) found = true;
        }
        if (!found) instanceExtensions.push_back(instanceExt[i]);
    }

    // Add Device Extensions
    for (unsigned int i = 0; i < deviceExtCount; ++i) {
        std::string extName = deviceExt[i];

        // Vulkan 1.2+ promotes BufferDeviceAddress to core. 
        // Requesting the EXT extension on a 1.3 instance can cause failure if the driver 
        // doesn't explicitly list the EXT string (even if the feature works).
        if (extName == "VK_EXT_buffer_device_address") {
            continue;
        }

        // Check for duplicates
        bool found = false;
        for (const auto& existing : deviceExtensions) {
            if (strcmp(existing, deviceExt[i]) == 0) found = true;
        }

        if (!found) {
            deviceExtensions.push_back(deviceExt[i]);
        }
    }
}


void NvdiaDLSS_Intergration::CleanUp()
{
    NVSDK_NGX_VULKAN_DestroyParameters(paramsDLSS_);
    NVSDK_NGX_VULKAN_Shutdown1(nullptr);
}