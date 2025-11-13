#pragma once
#include <vulkan/vulkan.hpp>


class  Camera;
class  VulkanContext;
class  BufferManager;
class  Lighting_FullScreenQuad;


class ReSTIR_DI
{

    ReSTIR_DI(VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger, Lighting_FullScreenQuad* rLightingPass);
    void createDescriptorSetLayout();

    void createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool);
    void UpdateDescrptorSets();
    void DestroyAtlasImages();
    void CreateImage();
    void DispatchResevoirCandidateCalcCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex);
private:

    VulkanContext* vulkanContext = nullptr;
    BufferManager* bufferManager = nullptr;
    Camera* camera = nullptr;
    vk::CommandPool commandPool = nullptr;

    Lighting_FullScreenQuad* LightingPass;

    vk::DescriptorSetLayout RservoirSamplingDescriptorSetLayout;
    std::vector<vk::DescriptorSet>  RservoirSamplingProbeDescriptorSets;

    ImageData ResevoirImage;
};

