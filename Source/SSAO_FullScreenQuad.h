#pragma once

#include <memory>
#include <string>
#include <vector>
#include "VulkanContext.h"
#include "Drawable.h"

struct SSAOUniformBuffer
{
    glm::mat4  CameraProjMatrix;
    glm::mat4  CameraViewMatrix;
    glm::vec4  KernelSizeRadiusBiasAndBool;
    glm::vec4  ssaoKernel[32];
};

class SSA0_FullScreenQuad : public Drawable
{
public:
    SSA0_FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext, Camera* cameraref, vk::CommandPool commandpool);

    ~SSA0_FullScreenQuad();

    // Setup methods
    void CreateImage();
    void DestroyImage();
    void createDescriptorSetLayout() override;
    void CreateKernel();
    void UpdataeUniformBufferData();
    void CreateUniformBuffer() override;

    // Helper
    float lerp(float a, float b, float f);

    // Rendering
    void createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, GBuffer Gbuffer);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;
    void DrawSSAOBlurVertical(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);
    void DrawSSAOBlurHorizontal(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);

    ImageData NoiseTexture;

    int KernelSize = 23;
    int bShouldSSAO = 1;
    float Radius = 1.5f;
    float Bias = 0.900;

    vk::DescriptorSetLayout SSAOBlurDescriptorSetLayout;
    std::vector<vk::DescriptorSet> SSAOBlurDescriptorSet;

    vk::Extent3D SSAOImageSize;
    vk::Extent3D BluredSSAOImageSize;

    ImageData SSAOImage;
    ImageData BluredSSAOImage;

private:
    Camera* camera = nullptr;
    std::vector<glm::vec4> ssaoNoise;
    SSAOUniformBuffer SSAOuniformbuffer;
};