#pragma once

#include <memory>
#include <string>
#include <vector> 
#include "VulkanContext.h"
#include "SkyBox.h"

constexpr int MAX_LIGHT_COUNT = 1000;
constexpr int MAX_OBJECT_COUNT = 10000;

class RT_Shadows;

struct Lightin_RTX_PC {
    glm::vec2 ScreenSize;
    int       LightCount;
    int       FrameIndex;
    glm::vec4 GI_Solution_Index_Padding;
};

class Lighting_RTX
{
public:
    Lighting_RTX(BufferManager* buffermanager, VulkanContext* vulkancontext, Camera* cameraref, vk::CommandPool commandpool, SkyBox* skyboxref);
    ~Lighting_RTX(); 

    void CreateUniformBuffer();
    void CreateStorageImage();
    void DestroyStorageImage();
    void createDescriptorSetLayout();
    void createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, GBuffer* Gbuffer, vk::AccelerationStructureKHR* TLAS);
    void UpdateDescrptorSets();
    void UpdateUniformBuffer(uint32_t currentImage, const std::vector<std::shared_ptr<Light>>& lightref);
    uint32_t alignedSize(uint32_t value, uint32_t alignment);

    void Draw(BufferData RayGenBuffer, BufferData RayHitBuffer, BufferData RayMisBuffer, vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);

    void CleanUp();

    std::vector<BufferData> UniformBuffers;
    std::vector<void*>      UniformBuffersMappedMem;

    ImageData ResultingStorageImage;

    GBuffer* GbufferRef = nullptr;

    vk::DescriptorSetLayout  descriptorSetLayout;

    int LightCount = 0;
    int GISolutionIndex = 0;

private:
    BufferManager* bufferManager = nullptr;
    VulkanContext* vulkanContext = nullptr;
    Camera* camera = nullptr;

    vk::CommandPool                commandPool;
    std::vector<vk::DescriptorSet> DescriptorSets;

    vk::Extent3D swapchainextent;
    vk::AccelerationStructureKHR* TLASr = nullptr;

    SkyBox* SkyBoxRef = nullptr;
    int frameIndex = 0;
};