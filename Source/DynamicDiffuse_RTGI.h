#pragma once

#include <memory>
#include <string>
#include "structs.h"
#include <vulkan/vulkan.hpp>
#include "MeshLoader.h"

class  Camera;
class  VulkanContext;
class  BufferManager;
class  Model;
class  Light;


class DynamicDiffuse_RTGI
{
public:

    DynamicDiffuse_RTGI(const std::string filepath,VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger);
    void CreateStorageImage();
    void DestroyStorageImage();
    void GenerateGrid();
    void CreateUniformBuffer();
    void createRayTracingDescriptorSetLayout();
    void createRaytracedDescriptorSets(vk::DescriptorPool descriptorpool);
    void UpdateUniformBuffer(uint32_t currentImage, std::vector<std::shared_ptr<Light>>& lightref, std::vector<std::shared_ptr<Model>>& Modelref);
    void CreateVertexAndIndexBuffer();
    uint32_t alignedSize(uint32_t value, uint32_t alignment);
    void Draw(BufferData RayGenBuffer, BufferData RayHitBuffer, BufferData RayMisBuffer, vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);

    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);

    void CleanUp() ;

    vk::DescriptorSetLayout        ProbeDescriptorSetLayout;
    std::vector<vk::DescriptorSet> ProbeDescriptorSets;


    ImageData IrradianceImageAtlasImage;
    ImageData ProbeVisibilityAtlasImage;

    vk::Extent3D swapchainextent;
    vk::Extent3D Blurextent;

    std::vector<glm::mat4> ProbeLocations;

    int NumOfProbes       = 5;
    glm::vec3 ProbeOffset = glm::vec3(10.0f, 10.0f, 10.0f);


private:

    VulkanContext*   vulkanContext = nullptr;
    BufferManager*   bufferManager = nullptr;
    Camera*          camera        = nullptr;
    vk::CommandPool commandPool = nullptr;

    BufferData vertexBufferData;
    BufferData indexBufferData;

    std::string FilePath;

    const StoredModelData* storedModelData = nullptr;

};


static inline void DynamicDiffuse_RTGIDeleter(DynamicDiffuse_RTGI* rayTracing) {

        if (rayTracing) {
            rayTracing->CleanUp();
            delete rayTracing;
        }
   
};