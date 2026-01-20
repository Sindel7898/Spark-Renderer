#pragma once
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <nvsdk_ngx.h>
#include <nvsdk_ngx_defs.h>
#include <nvsdk_ngx_params.h>
#include <nvsdk_ngx_vk.h>

#include <nvsdk_ngx_defs_dlssd.h>
#include <nvsdk_ngx_params_dlssd.h>

#include <nvsdk_ngx_helpers.h>
#include <nvsdk_ngx_helpers_vk.h>
#include <nvsdk_ngx_helpers_dlssd.h>
#include <nvsdk_ngx_helpers_dlssd_vk.h>

class  Camera;
class  BufferManager;
struct ImageData;
class  VulkanContext;

class NvdiaDLSS_Intergration
{
public:
    // Initialize the generic NGX Context
    void InitNGX(vk::Instance instance, vk::PhysicalDevice physicalDevice, vk::Device device);

    // Initialize the specific Ray Reconstruction feature
    // IMPORTANT: This requires a command buffer to perform initial setup
    void InitDLSS_RR(vk::CommandBuffer cmd, vk::Extent2D inputSize, vk::Extent2D outputSize);


    void init(int currentWidth, int currentHeight, float upScaleFactor);

    void render(VkCommandBuffer commandBuffer, VulkanCore::Texture& inColorTexture, VulkanCore::Texture& inDepthTexture, VulkanCore::Texture& inMotionVectorTexture, VulkanCore::Texture& outColorTexture, glm::vec2 cameraJitter);

    void requiredExtensions(std::vector<std::string>& instanceExtensions, std::vector<std::string>& deviceExtensions);

    void CleanUp();

    // The main denoising function matching DlssRR::denoise logic
    void Denoise(vk::CommandBuffer cmd,
        uint32_t frameIndex,
        Camera& cam,
        const ImageData& colorIn,
        const ImageData& colorOut,
        const ImageData& depth,
        const ImageData& mvec,
        const ImageData& normalRoughness,
        const ImageData& diffuseAlbedo,
        const ImageData& specularAlbedo,
        const ImageData& specularHitDist);

    float UpScaleFactor;
private:
	BufferManager* m_bufferManager = nullptr;
	VulkanContext* m_vulkanContext = nullptr;

	vk::CommandPool m_commandPool = nullptr;
    vk::Device m_device = nullptr;
    vk::PhysicalDevice m_physicalDevice = nullptr;
    vk::Instance m_instance = nullptr;

    NVSDK_NGX_Handle* dlssFeatureHandle_ = nullptr;
    NVSDK_NGX_Parameter* paramsDLSS_ = nullptr;

    // Helper to wrap Vulkan images for NGX
    NVSDK_NGX_Resource_VK CreateNGXResource(const ImageData& data, bool isOutput = false);
};

static inline void NvdiaDLSS_Deleter(NvdiaDLSS_Intergration* integration)
{
    if (integration)
    {
        integration->CleanUp();
        delete integration;
    }
}