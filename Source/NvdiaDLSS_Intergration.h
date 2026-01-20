#pragma once
#include <vulkan/vulkan.hpp>

class Camera;
struct ImageData;

struct DLSSRequirements {
    std::vector<const char*> instanceExtensions;
    std::vector<const char*> deviceExtensions;
    uint32_t extraGraphicsQueues = 0;
    uint32_t extraComputeQueues = 0;

};
class NvdiaDLSS_Intergration
{
public:
    void InitDLSS();
    void PrepareDLSS(vk::CommandBuffer cmd, uint32_t frameIndex, Camera& cam, vk::Extent3D swapchainExtent);
    void CleanUp();

    void RegisterVulkanDevice(vk::Instance instance, vk::PhysicalDevice physicalDevice, vk::Device device, uint32_t graphicsQueueIndex, uint32_t computeQueueIndex);

    void* m_currentFrameToken = nullptr;
    PFN_vkGetInstanceProcAddr sl_vkGetInstanceProcAddr = nullptr;

    void TagAndEvaluate(vk::CommandBuffer cmd, const ImageData& depth, const ImageData& mvec, const ImageData& diffuseNoisy, const ImageData& specularNoisy, const ImageData& outputColor, vk::Extent3D renderSize, vk::Extent3D displaySize);

    DLSSRequirements GetDLSSVulkanRequirements();
    PFN_vkQueuePresentKHR GetPresentProxy(vk::Device device);
    vk::Extent3D m_lastSwapchainExtent = { 0, 0, 0 };
};

static inline void NvdiaDLSS_Deleter(NvdiaDLSS_Intergration* integration)
{
    if (integration)
    {
        integration->CleanUp();
        delete integration;
    }
}