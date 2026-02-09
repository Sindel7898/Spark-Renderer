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
struct GBuffer;
class  VulkanContext;

class NvdiaDLSS_Intergration
{
public:
    NvdiaDLSS_Intergration() = default;

    void initializePointers(BufferManager* bufferManager, VulkanContext* vulkanContext, Camera* camera);

    void init(vk::CommandPool commandPool);

    void render(VkCommandBuffer commandBuffer, ImageData InImage, GBuffer inColorTexture, ImageData inDepthTexture, ImageData OutImage, VkFormat depthFormat, float deltaTime);
    void requiredExtensions(std::vector<const char*>& instanceExtensions, std::vector<const char*>& deviceExtensions);

    void CleanUp();

	int SceneChangeNotifer = 1;
private:
    float UpScaleFactor = 1;
	BufferManager* m_bufferManager = nullptr;
	VulkanContext* m_vulkanContext = nullptr;
	Camera*        m_camera        = nullptr;

    vk::Device m_device = nullptr;
    vk::PhysicalDevice m_physicalDevice = nullptr;
    vk::Instance m_instance = nullptr;

    NVSDK_NGX_Handle* dlssFeatureHandle_ = nullptr;
    NVSDK_NGX_Parameter* paramsDLSS_ = nullptr;
};

static inline void NvdiaDLSS_Deleter(NvdiaDLSS_Intergration* integration)
{
    if (integration)
    {
        integration->CleanUp();
        delete integration;
    }
}