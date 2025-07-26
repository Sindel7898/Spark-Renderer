#pragma once

#include <memory>
#include <string>
#include "structs.h"
#include <vulkan/vulkan.hpp>

class  Camera;
class  VulkanContext;
class  BufferManager;

class RayTracing
{
public:

    RayTracing(VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger);
    void CreateStorageImage();
    void createRayTracingDescriptorSetLayout();
    void createRaytracedDescriptorSets(vk::DescriptorPool descriptorpool, vk::AccelerationStructureKHR TLAS, GBuffer gbuffer);
    void CreateUniformBuffer();
    void UpdateUniformBuffer(uint32_t currentImage);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex);

    void CleanUp() ;

    vk::DescriptorSetLayout  RayTracingDescriptorSetLayout;
    std::vector<vk::DescriptorSet> DescriptorSets;

    VertexUniformData vertexdata{};

    int bCubeMapReflection = 0;
    int bScreenSpaceReflection = 0;

    ImageData ShadowPassImage;
private:

    VulkanContext*   vulkanContext = nullptr;
    BufferManager*   bufferManager = nullptr;
    Camera*          camera        = nullptr;
    vk::CommandPool* commandPool = nullptr;
};


static inline void RayTracingDeleter(RayTracing* rayTracing) {

        if (rayTracing) {
            rayTracing->CleanUp();
            delete rayTracing;
        }
   
};