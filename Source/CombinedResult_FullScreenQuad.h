#pragma once

#include <memory>
#include <string>
#include "VulkanContext.h"
#include "Drawable.h"
#include "Structs.h"

class CombinedResult_FullScreenQuad : public Drawable
{
public:

    CombinedResult_FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext, Camera* cameraref, vk::CommandPool commandpool);
    void CreateVertexAndIndexBuffer() override;
    void createDescriptorSetLayout() override;
    void UpdataeUniformBufferData();
    void createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, ImageData LightingResultImage, ImageData SSGIImage, ImageData SSAOIImage, ImageData MaterialImage, ImageData AlbedoImage);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;
    void CreateImage(vk::Extent3D imageExtent);
    void DestroyImage();

    void CleanUp();

    ImageData FinalResultImage;
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

static inline void CombinedResult_FullScreenQuadDeleter(CombinedResult_FullScreenQuad* CombinedResult_fullScreenQuad) {

    if (CombinedResult_fullScreenQuad)
    {
        CombinedResult_fullScreenQuad->CleanUp();
        delete CombinedResult_fullScreenQuad;
    }
}

