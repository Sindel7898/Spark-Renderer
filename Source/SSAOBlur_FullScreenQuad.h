#pragma once

#include <memory>
#include <string>
#include "VulkanContext.h"
#include "Drawable.h"
#include "Structs.h"

class SSAOBlur_FullScreenQuad : public Drawable
{
public:

    SSAOBlur_FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext, Camera* cameraref, vk::CommandPool commandpool);
    void CreateVertexAndIndexBuffer() override;
    void createDescriptorSetLayout() override;
    void UpdataeUniformBufferData();
    void CreateUniformBuffer() override;
    void createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, GBuffer Gbuffer);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;

    void CleanUp();

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
};

static inline void SSAOBlur_FullScreenQuadDeleter(SSAOBlur_FullScreenQuad* SSA0_fullScreenQuad) {

    if (SSA0_fullScreenQuad)
    {
        SSA0_fullScreenQuad->CleanUp();
        delete SSA0_fullScreenQuad;
    }
}

