#pragma once

#include <memory>
#include <string>
#include "structs.h"
#include <vulkan/vulkan.hpp>

class  Camera;
class  VulkanContext;
class  BufferManager;

struct RayGen_UniformBufferData {
    glm::mat4 ViewMatrix;
    glm::mat4 ProjectionMatrix;
};

struct RayClosesetHit_UniformBufferData {
    glm::vec4 LightPosition_Padding;
};


class RayTracing
{
public:

    RayTracing(VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger);
    void CreateStorageImage();
    void DestroyStorageImage();
    void createRayTracingDescriptorSetLayout();
    void createRaytracedDescriptorSets(vk::DescriptorPool descriptorpool, vk::AccelerationStructureKHR TLAS, GBuffer gbuffer);
    void UpdateUniformBuffer(uint32_t currentImage, std::vector<std::shared_ptr<Light>>& lightref);
    void CreateUniformBuffer();
    uint32_t alignedSize(uint32_t value, uint32_t alignment);
    void Draw(BufferData RayGenBuffer, BufferData RayHitBuffer, BufferData RayMisBuffer, vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);

    void CleanUp() ;
    std::vector<BufferData> RayGen_UniformBuffers;
    std::vector<BufferData> RayClosestHit_UniformBuffers;

    std::vector<void*> RayGen_UniformBuffersMappedMem;
    std::vector<void*> RayClosestHit_UniformBuffersMappedMem;

    vk::DescriptorSetLayout  RayTracingDescriptorSetLayout;
    std::vector<vk::DescriptorSet> RayTracingDescriptorSets;

    ImageData ShadowPassImage;
private:

    VulkanContext*   vulkanContext = nullptr;
    BufferManager*   bufferManager = nullptr;
    Camera*          camera        = nullptr;
    vk::CommandPool commandPool = nullptr;
};


static inline void RayTracingDeleter(RayTracing* rayTracing) {

        if (rayTracing) {
            rayTracing->CleanUp();
            delete rayTracing;
        }
   
};