// 1. Guard Macros to prevent redefinition warnings
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h> // Include C Header FIRST

#ifndef VK_VERSION_1_0
#define VK_VERSION_1_0 1
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

// 2. Streamline Includes
#ifdef constant
#undef constant
#endif
#include <sl.h>
#include <sl_dlss.h>
#include <sl_dlss_d.h>
#include <sl_helpers_vk.h> // REQUIRED for sl::VulkanInfo

#include "NvdiaDLSS_Intergration.h"
#include "Camera.h" 
#include "structs.h"

void LogCallback(sl::LogType type, const char* msg) {
    if (type == sl::LogType::eError) {
        std::cerr << "[Streamline Error] " << msg << std::endl;
    }
    else if (type == sl::LogType::eWarn) {
        std::cerr << "[Streamline Warn] " << msg << std::endl;
    }
    else {
        std::cout << "[Streamline Info] " << msg << std::endl;
    }
}

void NvdiaDLSS_Intergration::InitDLSS()
{
    HMODULE mod = LoadLibraryA("sl.interposer.dll");
    if (!mod) {
        std::cerr << "Failed to load sl.interposer.dll.\n";
        return;
    }

    sl_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(mod, "vkGetInstanceProcAddr");

    sl::Preferences pref{};
    pref.flags |= sl::PreferenceFlags::eUseManualHooking;
    pref.flags |= sl::PreferenceFlags::eUseFrameBasedResourceTagging; 
    pref.applicationId = 231313132;
    pref.engine = sl::EngineType::eCustom;
    pref.engineVersion = "1.0.0";
	pref.showConsole = true;
	pref.logLevel = sl::LogLevel::eOff;
    pref.logMessageCallback = LogCallback;
    sl::Feature features[] = { sl::kFeatureDLSS, sl::kFeatureDLSS_RR };
    pref.featuresToLoad = features;
    pref.numFeaturesToLoad = std::size(features);
    pref.renderAPI = sl::RenderAPI::eVulkan;

    if (slInit(pref) != sl::Result::eOk) {
        std::cerr << "Streamline Init failed.\n";
    }
}

float Halton(uint32_t index, uint32_t base) {
    float f = 1.0f;
    float r = 0.0f;
    while (index > 0) {
        f = f / (float)base;
        r = r + f * (float)(index % base);
        index = index / base;
    }
    return r - 0.5f;
}

void NvdiaDLSS_Intergration::PrepareDLSS(vk::CommandBuffer cmd, uint32_t frameIndex, Camera& cam, vk::Extent3D swapchainExtent)
{
    uint32_t jitterIndex = frameIndex % 16;
    float jitterX = Halton(jitterIndex + 1, 2);
    float jitterY = Halton(jitterIndex + 1, 3);

    sl::FrameToken* frameToken = nullptr;
    if (slGetNewFrameToken(frameToken, &frameIndex) != sl::Result::eOk) return;

    m_currentFrameToken = (void*)frameToken;

    sl::Constants consts = {};
    glm::mat4 proj = cam.GetProjectionMatrix();
    glm::mat4 view = cam.GetViewMatrix();
    glm::mat4 viewProj = proj * view;
    glm::mat4 prevViewProj = cam.GetPrevProjectionMatrix() * cam.GetPrevViewMatrix();
    glm::mat4 clipToPrevClip = prevViewProj * glm::inverse(viewProj);

    glm::mat4 prevClipToClip = glm::inverse(clipToPrevClip);

    consts.cameraViewToClip = *(sl::float4x4*)glm::value_ptr(glm::transpose(proj));
    consts.clipToCameraView = *(sl::float4x4*)glm::value_ptr(glm::transpose(glm::inverse(proj)));
    consts.clipToPrevClip = *(sl::float4x4*)glm::value_ptr(glm::transpose(clipToPrevClip));
    consts.prevClipToClip = *(sl::float4x4*)glm::value_ptr(glm::transpose(prevClipToClip));
    consts.cameraPinholeOffset = { 0.0f, 0.0f };


    glm::vec3 pos = cam.GetPosition();
    consts.cameraPos = { pos.x, pos.y, pos.z };

    consts.cameraNear = cam.GetNearClip(); // e.g., 0.1f
    consts.cameraFar = cam.GetFarClip();   // e.g., 1000.0f
    consts.cameraFOV = glm::radians(cam.GetFOV()); // FOV in radians
    consts.cameraAspectRatio = (float)swapchainExtent.width / (float)swapchainExtent.height;

    // Extract vectors from View Matrix if Camera doesn't have getters
    // View Matrix: Row 0=Right, Row 1=Up, Row 2=Forward (depending on implementation)
    // Safer to take Inverse View (Camera World) column vectors
    glm::mat4 invView = glm::inverse(view);
    glm::vec3 forward = -glm::vec3(invView[2]); // -Z is forward in OpenGL/Vulkan
    glm::vec3 up = glm::vec3(invView[1]);
    glm::vec3 right = glm::vec3(invView[0]);

    consts.cameraFwd = { forward.x, forward.y, forward.z };
    consts.cameraUp = { up.x, up.y, up.z };
    consts.cameraRight = { right.x, right.y, right.z };

    consts.jitterOffset = { jitterX, jitterY };
    consts.mvecScale = { 1.0f / swapchainExtent.width, 1.0f / swapchainExtent.height };
    consts.depthInverted = sl::Boolean::eTrue;
    consts.cameraMotionIncluded = sl::Boolean::eTrue;
    consts.motionVectors3D = sl::Boolean::eFalse;
    consts.reset = sl::Boolean::eFalse;

    sl::ViewportHandle viewport = { 0 };
    slSetConstants(consts, *frameToken, viewport);

    sl::DLSSOptions dlssOpts = {};
    dlssOpts.mode = sl::DLSSMode::eBalanced;
    dlssOpts.outputWidth = swapchainExtent.width;
    dlssOpts.outputHeight = swapchainExtent.height;
    dlssOpts.dlaaPreset = sl::DLSSPreset::ePresetK;
    slDLSSSetOptions(viewport, dlssOpts);

    cam.UpdateJitter(jitterX, jitterY);
}

sl::Resource CreateSLResource(const ImageData& Image, vk::ImageLayout layout)
{
    sl::Resource res = {};
    res.type = sl::ResourceType::eTex2d;
    res.native = (void*)(VkImage)Image.image;
    res.memory = nullptr;
    res.view = (void*)(VkImageView)Image.imageView;
    res.width = Image.extent.width;
    res.height = Image.extent.height;
    res.nativeFormat = (uint32_t)Image.format;
    res.mipLevels = 1;
    res.arrayLayers = 1;
    res.flags = 0;
    res.state = (uint32_t)layout;

    if (layout == vk::ImageLayout::eGeneral)
        res.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    else
        res.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

    return res;
}

void NvdiaDLSS_Intergration::TagAndEvaluate(vk::CommandBuffer cmd, const ImageData& depth, const ImageData& mvec, const ImageData& diffuseNoisy, const ImageData& specularNoisy, const ImageData& outputColor, vk::Extent3D renderSize, vk::Extent3D displaySize)
{
    if (!m_currentFrameToken) return;
    sl::FrameToken* frameToken = (sl::FrameToken*)m_currentFrameToken;

    sl::ViewportHandle viewport = { 0 };
    sl::Extent renderExt = { 0, 0, renderSize.width, renderSize.height };
    sl::Extent displayExt = { 0, 0, displaySize.width, displaySize.height };

    auto depthRes = CreateSLResource(depth, vk::ImageLayout::eDepthAttachmentOptimal);
    auto mvecRes = CreateSLResource(mvec, vk::ImageLayout::eGeneral);
    auto diffRes = CreateSLResource(diffuseNoisy, vk::ImageLayout::eGeneral);
    auto specRes = CreateSLResource(specularNoisy, vk::ImageLayout::eGeneral);
    auto outRes = CreateSLResource(outputColor, vk::ImageLayout::eGeneral);

    std::vector<sl::ResourceTag> tags;
    tags.push_back({ &depthRes, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent, &renderExt });
    tags.push_back({ &mvecRes, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent, &renderExt });
    tags.push_back({ &diffRes, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilPresent, &renderExt });
    tags.push_back({ &specRes, sl::kBufferTypeSpecularHitNoisy, sl::ResourceLifecycle::eValidUntilPresent, &renderExt });
    tags.push_back({ &outRes , sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilPresent, &displayExt });

    if (slSetTagForFrame(*frameToken, viewport, tags.data(), (uint32_t)tags.size(), (void*)(VkCommandBuffer)cmd) != sl::Result::eOk) {
        std::cerr << "SL Tagging failed\n";
        return;
    }

    const sl::BaseStructure* inputs[] = { &viewport };
    if (slEvaluateFeature(sl::kFeatureDLSS, *frameToken, inputs, 1, (void*)(VkCommandBuffer)cmd) != sl::Result::eOk) {
        std::cerr << "SL Evaluate failed\n";
    }
}

DLSSRequirements NvdiaDLSS_Intergration::GetDLSSVulkanRequirements() {
    DLSSRequirements reqsOut;
    sl::FeatureRequirements reqs{};

    if (slGetFeatureRequirements(sl::kFeatureDLSS, reqs) == sl::Result::eOk) {
        for (uint32_t i = 0; i < reqs.vkNumInstanceExtensions; i++)
            reqsOut.instanceExtensions.push_back(reqs.vkInstanceExtensions[i]);
        for (uint32_t i = 0; i < reqs.vkNumDeviceExtensions; i++)
            reqsOut.deviceExtensions.push_back(reqs.vkDeviceExtensions[i]);
        reqsOut.extraGraphicsQueues = reqs.vkNumGraphicsQueuesRequired;
        reqsOut.extraComputeQueues = reqs.vkNumComputeQueuesRequired;
    }
    return reqsOut;
}


void NvdiaDLSS_Intergration::RegisterVulkanDevice(vk::Instance instance, vk::PhysicalDevice physicalDevice, vk::Device device, uint32_t graphicsQueueIndex, uint32_t computeQueueIndex)
{
    sl::VulkanInfo vkInfo = {};
    vkInfo.instance = (VkInstance)instance;
    vkInfo.physicalDevice = (VkPhysicalDevice)physicalDevice;
    vkInfo.device = (VkDevice)device;
    vkInfo.graphicsQueueIndex = graphicsQueueIndex;
    vkInfo.computeQueueIndex = computeQueueIndex;

    if (slSetVulkanInfo(vkInfo) != sl::Result::eOk) {
        std::cerr << "Failed to set Streamline Vulkan Info\n";
    }
}

void NvdiaDLSS_Intergration::CleanUp()
{
    slShutdown();
}