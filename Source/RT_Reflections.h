#pragma once

#include <memory>
#include <string>
#include "structs.h"
#include <vulkan/vulkan.hpp>
#include "Structs.h"

class  Camera;
class  VulkanContext;
class  BufferManager;
class  Model;

struct Reflection_RayGen_UniformBufferData {
    glm::mat4 ViewMatrix;
    glm::mat4 ProjectionMatrix;
};

class RT_Reflections
{
public:

    RT_Reflections(VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger);
    void CreateStorageImage();
    void DestroyStorageImage();
    void createRayTracingDescriptorSetLayout();
    void createRaytracedDescriptorSets(vk::DescriptorPool descriptorpool, vk::AccelerationStructureKHR TLAS, GBuffer gbuffer, std::vector<BufferData>& fragmentUniformBuffers);
    void UpdateUniformBuffer(uint32_t currentImage, std::vector<std::shared_ptr<Light>>& lightref, std::vector<std::shared_ptr<Model>>& Modelref);
    void CreateUniformBuffer();
    uint32_t alignedSize(uint32_t value, uint32_t alignment);
    void Draw(BufferData RayGenBuffer, BufferData RayHitBuffer, BufferData RayMisBuffer, vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);

    void DrawHorizontalBlurPass(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);

    void DrawVerticalBlurPass(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);

    void CleanUp() ;
    std::vector<BufferData> RayGen_UniformBuffers;
    std::vector<void*> RayGen_UniformBuffersMappedMem;

    std::vector<BufferData> IndexStorageBuffers;
    std::vector<void*>      IndexStorageBuffersMappedMem;

    std::vector<BufferData> VertexStorageBuffers;
    std::vector<void*>      VertexStorageBuffersMappedMem;

    std::vector<BufferData> OffsetStorageBuffers;
    std::vector<void*>      OffsetStorageBuffersMappedMem;

    std::vector<BufferData> TransformationUniformBuffers;
    std::vector<void*>       TransformationUniformMappedMem;

    vk::DescriptorSetLayout  RayTracingDescriptorSetLayout;
    vk::DescriptorSetLayout  BlurDescriptorSetLayout;

    std::vector<vk::DescriptorSet> RayTracingDescriptorSets;
    std::vector<vk::DescriptorSet> HorizontalDescriptorSets;
    std::vector<vk::DescriptorSet> FullBlurDescriptorSets;


    ImageData ReflectionPassImage;
    ImageData HorizontalBlurReflectionPassImage;
    ImageData FullBlurReflectionPassImage;

    int NumOfShadowCasters;

    vk::Extent3D swapchainextent;
    vk::Extent3D Blurextent;

    BufferData vertexBufferData;
    BufferData indexBufferData;

private:

    VulkanContext*   vulkanContext = nullptr;
    BufferManager*   bufferManager = nullptr;
    Camera*          camera        = nullptr;
    vk::CommandPool commandPool = nullptr;


    std::vector<Vertex> quad = {
     {{-1.0f, -1.0f}, {0.0f, 0.0f}}, // Bottom-left
     {{ 1.0f, -1.0f}, {1.0f, 0.0f}}, // Bottom-right
     {{-1.0f,  1.0f}, {0.0f, 1.0f}}, // Top-left
     {{ 1.0f,  1.0f}, {1.0f, 1.0f}}  // Top-right
    };

    const std::vector<uint16_t> quadIndices = {
           0, 1, 2,
           2, 1, 3
    };

};


static inline void RT_ReflectionsDeleter(RT_Reflections* rayTracing) {

        if (rayTracing) {
            rayTracing->CleanUp();
            delete rayTracing;
        }
   
};