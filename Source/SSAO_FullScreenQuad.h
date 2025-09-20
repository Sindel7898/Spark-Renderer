#pragma once

#include <memory>
#include <string>
#include "VulkanContext.h"
#include "Drawable.h"
#include "Structs.h"

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
    void CreateVertexAndIndexBuffer() override;
    void CreateImage();
    void DestroyImage();
    void createDescriptorSetLayout() override;
    void CreateKernel();
    void UpdataeUniformBufferData();
    void CreateUniformBuffer() override;
    float lerp(float a, float b, float f);
    void createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, GBuffer Gbuffer);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;

    void DrawSSAOBlurVertical(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);
    void DrawSSAOBlurHorizontal(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);

    void CleanUp();

    ImageData NoiseTexture;

    int KernelSize = 24;
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


    Camera* camera = nullptr;
    std::vector<glm::vec4> ssaoNoise;
    SSAOUniformBuffer SSAOuniformbuffer;

};

static inline void SSA0_FullScreenQuadDeleter(SSA0_FullScreenQuad* SSA0_fullScreenQuad) {

    if (SSA0_fullScreenQuad)
    {
        SSA0_fullScreenQuad->CleanUp();
        delete SSA0_fullScreenQuad;
    }
}

