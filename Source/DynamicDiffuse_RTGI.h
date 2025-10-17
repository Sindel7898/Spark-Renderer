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

struct CameraConstantBuffer
{
    glm::mat4 ViewMatrix;
    glm::mat4 ProjectionMatrix;

};

struct GridData
{
    glm::vec4 probeCount;
    glm::vec4 probeOffset;
    glm::vec4 probeBaseLocation;

};

class DynamicDiffuse_RTGI
{
public:

    DynamicDiffuse_RTGI(const std::string filepath,VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger);
    void CreateStorageImage();
    void DestroyStorageImage();
    void UpdateProbeCount(glm::vec3 NewProbeCount);
    void UpdateProbsOffset(glm::vec3 NewProbeOffset);
    void UpdateGridLocation(glm::vec3 NewGridLocation);
    void CreateStorageBuffer();
    void createRayTracingDescriptorSetLayout();
    void createRaytracedDescriptorSets(vk::DescriptorPool descriptorpool);
    void CreateVertexAndIndexBuffer();
    uint32_t alignedSize(uint32_t value, uint32_t alignment);
    void Draw(BufferData RayGenBuffer, BufferData RayHitBuffer, BufferData RayMisBuffer, vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);
    void DrawNode(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex, const std::vector<std::shared_ptr<Node>>& nodes);

    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);

    void DispatchGridCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex);

    void CleanUp() ;

    vk::DescriptorSetLayout        ProbeDescriptorSetLayout;
    vk::DescriptorSetLayout        GridDescriptorSetLayout;
    vk::DescriptorSetLayout        RaytracingDescriptorSetLayout;

    std::vector<vk::DescriptorSet> ProbeDescriptorSets;
    std::vector<vk::DescriptorSet> GridDescriptorSets;


    ImageData IrradianceImageAtlasImage;
    ImageData ProbeVisibilityAtlasImage;

    vk::Extent3D swapchainextent;
    vk::Extent3D Blurextent;

    std::vector<glm::mat4> ProbeLocations;

    int NumOfProbesX = 10;
    int NumOfProbesY = 10;
    int NumOfProbesZ = 10;

    int LastNumOfProbesX = 0;
    int LastNumOfProbesY = 0;
    int LastNumOfProbesZ = 0;

    glm::vec3 ProbeOffset     = glm::vec3(20, 6.4, 10.8);
    glm::vec3 LastProbeOffset = glm::vec3(0, 0, 0);

    glm::vec3 GridLocation     = glm::vec3(-90.000, 0, -50.000);
    glm::vec3 LastGridLocation = glm::vec3(0, 0, 0);


    std::vector<BufferData> ProbePositionsStorageBuffers;
    std::vector<BufferData> ProbeFibonacciDirectionsStorageBuffers;

private:

    VulkanContext*   vulkanContext = nullptr;
    BufferManager*   bufferManager = nullptr;
    Camera*          camera        = nullptr;
    vk::CommandPool commandPool = nullptr;

    BufferData vertexBufferData;
    BufferData indexBufferData;

    std::string FilePath;

    const StoredModelData* storedModelData = nullptr;

    bool UpdateGrid = false;
};


static inline void DynamicDiffuse_RTGIDeleter(DynamicDiffuse_RTGI* rayTracing) {

        if (rayTracing) {
            rayTracing->CleanUp();
            delete rayTracing;
        }
   
};