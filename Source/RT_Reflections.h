#pragma once

#include <memory>
#include <string>
#include "structs.h"
#include <vulkan/vulkan.hpp>

class  Camera;
class  VulkanContext;
class  BufferManager;

class RT_Reflections
{
public:

    RT_Reflections(VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger);
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

    ImageData RT_ReflectionPassImage;


    vk::Extent3D swapchainextent;
private:

    VulkanContext*   vulkanContext = nullptr;
    BufferManager*   bufferManager = nullptr;
    Camera*          camera        = nullptr;
    vk::CommandPool commandPool = nullptr;
};


static inline void RT_ReflectionsDeleter(RT_Reflections* rayTracing) {

        if (rayTracing) {
            rayTracing->CleanUp();
            delete rayTracing;
        }
   
};