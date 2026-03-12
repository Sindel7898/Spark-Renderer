#pragma once
#include "NvdiaDLSS_Intergration.h"
#include "Camera.h" 
#include "structs.h"
#include"BufferManager.h"
#include"VulkanContext.h"
#include <iostream> 
#include <glm/gtc/type_ptr.hpp>

static void NVSDK_CONV LogCallback(const char* message, NVSDK_NGX_Logging_Level loggingLevel, NVSDK_NGX_Feature sourceComponent)
{
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
	appId.v.ApplicationId = 0x1234567890ABCDEF;

    NVSDK_NGX_FeatureCommonInfo commonInfo = {};
    commonInfo.LoggingInfo.LoggingCallback = LogCallback;
    commonInfo.LoggingInfo.MinimumLoggingLevel = NVSDK_NGX_LOGGING_LEVEL_ON;
    commonInfo.LoggingInfo.DisableOtherLoggingSinks = false;

    NVSDK_NGX_Result initResult = NVSDK_NGX_VULKAN_Init(
        appId.v.ApplicationId,
        L".",
        m_instance,
        m_physicalDevice,
        m_device,
        vkGetInstanceProcAddr,
        vkGetDeviceProcAddr,
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

    if (dlssFeatureHandle_ != nullptr) {
        NVSDK_NGX_VULKAN_ReleaseFeature(dlssFeatureHandle_);
        dlssFeatureHandle_ = nullptr;
    }


    uint32_t displayWidth = m_vulkanContext->swapchainExtent.width;
    uint32_t displayHeight = m_vulkanContext->swapchainExtent.height;

    NVSDK_NGX_PerfQuality_Value dlssQuality = NVSDK_NGX_PerfQuality_Value_Balanced;

    paramsDLSS_->Set(NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_Quality, NVSDK_NGX_DLSS_Hint_Render_Preset_L);
    paramsDLSS_->Set(NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_UltraQuality, NVSDK_NGX_DLSS_Hint_Render_Preset_L);
    paramsDLSS_->Set(NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_Balanced, NVSDK_NGX_DLSS_Hint_Render_Preset_L);
    paramsDLSS_->Set(NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_Performance, NVSDK_NGX_DLSS_Hint_Render_Preset_L);

    paramsDLSS_->Set(NVSDK_NGX_Parameter_RTXValue, NVSDK_NGX_RTX_Value_On);


    int dlssCreateFeatureFlags = NVSDK_NGX_DLSS_Feature_Flags_IsHDR | NVSDK_NGX_DLSS_Feature_Flags_MVLowRes;

    NVSDK_NGX_DLSSD_Create_Params dlssdCreateParams = {};
    dlssdCreateParams.InDenoiseMode = NVSDK_NGX_DLSS_Denoise_Mode_DLUnified; 
    dlssdCreateParams.InRoughnessMode = NVSDK_NGX_DLSS_Roughness_Mode_Unpacked; 
    dlssdCreateParams.InUseHWDepth = NVSDK_NGX_DLSS_Depth_Type_HW;
    dlssdCreateParams.InWidth = displayWidth;
    dlssdCreateParams.InHeight = displayHeight;
    dlssdCreateParams.InTargetWidth = displayWidth;
    dlssdCreateParams.InTargetHeight = displayHeight;
    dlssdCreateParams.InPerfQualityValue = dlssQuality;
    dlssdCreateParams.InFeatureCreateFlags = dlssCreateFeatureFlags;

    auto commmandBuffer = m_bufferManager->CreateSingleUseCommandBuffer(commandPool);

    NVSDK_NGX_Result createDlssResult = NGX_VULKAN_CREATE_DLSSD_EXT1(
        m_device, commmandBuffer, 1, 1, &dlssFeatureHandle_, paramsDLSS_, &dlssdCreateParams
    );

    if (NVSDK_NGX_FAILED(createDlssResult)) {
        throw std::runtime_error("Failed to create DLAA Feature");
    }

    m_bufferManager->SubmitAndDestoyCommandBuffer(commandPool, commmandBuffer, m_vulkanContext->graphicsQueue);
}

void NvdiaDLSS_Intergration::render(VkCommandBuffer commandBuffer, ImageData InImage,
    GBuffer inColorTexture, ImageData inDepthTexture,
    ImageData OutImage, VkFormat depthFormat,float deltaTime)
{

    NVSDK_NGX_Resource_VK inColorResource = NVSDK_NGX_Create_ImageView_Resource_VK(
        InImage.imageView, InImage.image,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, VK_FORMAT_R16G16B16A16_SFLOAT,
        m_vulkanContext->swapchainExtent.width, m_vulkanContext->swapchainExtent.height, false);

    NVSDK_NGX_Resource_VK outColorResource = NVSDK_NGX_Create_ImageView_Resource_VK(
        OutImage.imageView, OutImage.image,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, VK_FORMAT_R16G16B16A16_SFLOAT,
        m_vulkanContext->swapchainExtent.width, m_vulkanContext->swapchainExtent.height, true);

    NVSDK_NGX_Resource_VK depthResource = NVSDK_NGX_Create_ImageView_Resource_VK(
        inDepthTexture.imageView, inDepthTexture.image,
        { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 }, depthFormat,
        m_vulkanContext->swapchainExtent.width, m_vulkanContext->swapchainExtent.height, false);

    NVSDK_NGX_Resource_VK motionVectorResource = NVSDK_NGX_Create_ImageView_Resource_VK(
        inColorTexture.MotionVector.imageView, inColorTexture.MotionVector.image,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, VK_FORMAT_R16G16B16A16_SFLOAT,
        m_vulkanContext->swapchainExtent.width, m_vulkanContext->swapchainExtent.height, false);

    NVSDK_NGX_Resource_VK diffuseAlbedoResource = NVSDK_NGX_Create_ImageView_Resource_VK(
        inColorTexture.Albedo.imageView, inColorTexture.Albedo.image,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, VK_FORMAT_R16G16B16A16_SFLOAT,
        m_vulkanContext->swapchainExtent.width, m_vulkanContext->swapchainExtent.height, false);

    NVSDK_NGX_Resource_VK normalsResource = NVSDK_NGX_Create_ImageView_Resource_VK(
        inColorTexture.Normal.imageView, inColorTexture.Normal.image,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, VK_FORMAT_R16G16B16A16_SFLOAT,
        m_vulkanContext->swapchainExtent.width, m_vulkanContext->swapchainExtent.height, false);

    NVSDK_NGX_Resource_VK roughnessResource = NVSDK_NGX_Create_ImageView_Resource_VK(
        inColorTexture.Materials.imageView, inColorTexture.Materials.image,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, VK_FORMAT_R8G8B8A8_UNORM, 
        m_vulkanContext->swapchainExtent.width, m_vulkanContext->swapchainExtent.height, false);

    NVSDK_NGX_Resource_VK specularAlbedoResource = NVSDK_NGX_Create_ImageView_Resource_VK(
        inColorTexture.SpecularAlbedo.imageView, inColorTexture.SpecularAlbedo.image,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
        m_vulkanContext->swapchainExtent.width, m_vulkanContext->swapchainExtent.height, false);

    float width  = static_cast<float>(m_vulkanContext->swapchainExtent.width);
    float height = static_cast<float>(m_vulkanContext->swapchainExtent.height);

    NVSDK_NGX_VK_DLSSD_Eval_Params evalParams = {};
    evalParams.pInColor = &inColorResource;     
    evalParams.pInOutput = &outColorResource;   
    evalParams.pInDepth = &depthResource;
    evalParams.pInMotionVectors = &motionVectorResource;
    evalParams.pInDiffuseAlbedo = &diffuseAlbedoResource;
    evalParams.pInNormals = &normalsResource;
    evalParams.pInRoughness = &roughnessResource;
    evalParams.pInSpecularAlbedo = &specularAlbedoResource;
    //evalParams.InJitterOffsetX = -m_camera->Jitter.x;
    //evalParams.InJitterOffsetY = -m_camera->Jitter.y;
    evalParams.InRenderSubrectDimensions = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    evalParams.InMVScaleX = 1;
    evalParams.InMVScaleY = 1;
    evalParams.InReset = SceneChangeNotifer;
    evalParams.InFrameTimeDeltaInMsec = deltaTime * 1000.0f;
    evalParams.InPreExposure = 1.0f;



    evalParams.pInWorldToViewMatrix = const_cast<float*>(glm::value_ptr(m_camera->GetViewMatrix()));
    evalParams.pInViewToClipMatrix = const_cast<float*>(glm::value_ptr(m_camera->GetProjectionMatrix()));

	SceneChangeNotifer = 0;

    NVSDK_NGX_Result result = NGX_VULKAN_EVALUATE_DLSSD_EXT(
        commandBuffer,
        dlssFeatureHandle_,
        paramsDLSS_,
        &evalParams
    );

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

    for (unsigned int i = 0; i < instanceExtCount; ++i) {
        bool found = false;
        for (const auto& existing : instanceExtensions) {
            if (strcmp(existing, instanceExt[i]) == 0) found = true;
        }
        if (!found) instanceExtensions.push_back(instanceExt[i]);
    }

    // Add Device Extensions
    for (unsigned int i = 0; i < deviceExtCount; ++i) {
        std::string extName = deviceExt[i];

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