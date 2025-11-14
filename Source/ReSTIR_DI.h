#pragma once
#include <vulkan/vulkan.hpp>
#include "structs.h"


class  Camera;
class  VulkanContext;
class  BufferManager;
class  Lighting_FullScreenQuad;
class  SSGI;

struct PushConstant {
    glm::vec4 CameraPosition;
    glm::vec4 ScreenSize;
};

class ReSTIR_DI
{
public:
    ReSTIR_DI(VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger, Lighting_FullScreenQuad* rLightingPass, SSGI* rssgi);
    void createDescriptorSetLayout();

    void createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool);
    void UpdateDescrptorSets();
    void DestroyImage();
    void CreateImage();
    void CleanUp();
    void DispatchResevoirCandidateCalcCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex);
    vk::DescriptorSetLayout RservoirSamplingDescriptorSetLayout;
    ImageData ResevoirImage;

private:

    VulkanContext* vulkanContext = nullptr;
    BufferManager* bufferManager = nullptr;
    Camera* camera = nullptr;
    vk::CommandPool commandPool = nullptr;

    Lighting_FullScreenQuad* LightingPass;
    SSGI* ssgi;

    std::vector<vk::DescriptorSet>  RservoirSamplingProbeDescriptorSets;

};

static inline void ReSTIR_DI_Deleter(ReSTIR_DI* restir) {

    if (restir) {
        restir->CleanUp();
        delete restir;
    }

};