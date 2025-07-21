#pragma once

#include <memory>
#include <string>
#include "VulkanContext.h"
#include "Drawable.h"
#include "Structs.h"


class SSR_FullScreenQuad : public Drawable
{
public:

    SSR_FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext, Camera* cameraref, vk::CommandPool commandpool);
    void CreateVertexAndIndexBuffer() override;
    void CreateImage(vk::Extent3D ImageEXtent);
    void DestroyImage();
    void createDescriptorSetLayout() override;
    void createDescriptorSets(vk::DescriptorPool descriptorpool, ImageData LightingPass, ImageData NormalPass, ImageData ViewSpacePositionPass, ImageData DepthPass, ImageData ReflectionMask);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;
    void CleanUp();

    ImageData SSRImage;

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
};

static inline void SSR_FullScreenQuadDeleter(SSR_FullScreenQuad* ssr_FullScreenQuad) {

    if (ssr_FullScreenQuad)
    {
        ssr_FullScreenQuad->CleanUp();
        delete ssr_FullScreenQuad;
    }
}

