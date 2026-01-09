#pragma once

#include <memory>
#include <string>
#include "VulkanContext.h"
#include "SkyBox.h"
class RT_Shadows;

class Lighting_RTX
{
public:

    Lighting_RTX(BufferManager* buffermanager, VulkanContext* vulkancontext, Camera* cameraref, vk::CommandPool commandpool, SkyBox* skyboxref);
    void CreateUniformBuffer();
    void CreateStorageImage();
    void DestroyStorageImage();
    void createDescriptorSetLayout();
    void createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, GBuffer* Gbuffer, vk::AccelerationStructureKHR* TLAS);
    void UpdateDescrptorSets();
    void UpdateUniformBuffer(uint32_t currentImage, std::vector<std::shared_ptr<Light>>& lightref);
    uint32_t alignedSize(uint32_t value, uint32_t alignment);

    void Draw(BufferData RayGenBuffer, BufferData RayHitBuffer, BufferData RayMisBuffer, vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);

    void CleanUp();
    
    BufferManager* bufferManager = nullptr;
    VulkanContext* vulkanContext = nullptr;
    Camera* camera = nullptr;


    vk::CommandPool                commandPool;
    std::vector<vk::DescriptorSet> DescriptorSets;

    std::vector<BufferData> UniformBuffers;
    std::vector<void*>      UniformBuffersMappedMem;

    GBuffer*   GbufferRef = nullptr;
    SkyBox* SkyBoxRef = nullptr;
    int LightCount = 0;
	ImageData ResultingStorageImage;
    vk::Extent3D swapchainextent;
    vk::DescriptorSetLayout  descriptorSetLayout;
    vk::AccelerationStructureKHR* TLASr;

   private:


};

static inline void Lighting_RTXDeleter(Lighting_RTX* ref) {
  
    if (ref)
    {
        ref->CleanUp();
        delete ref;
    }
}

