#pragma once

#include <memory>
#include <string>
#include "VulkanContext.h"
#include "Drawable.h"
#include "Structs.h"


class FXAA_FullScreenQuad : public Drawable
{
public:

    FXAA_FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext, Camera* cameraref, vk::CommandPool commandpool);
    void CreateVertexAndIndexBuffer() override;
    void CreateImage(vk::Extent3D ImageEXtent);
    void DestroyImage();
    void createDescriptorSetLayout() override;
    void createDescriptorSets(vk::DescriptorPool descriptorpool, ImageData LightingPass);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;
    void CleanUp();

    ImageData FxaaImage;

    glm::vec4 bFXAA_Padding;
    int bFXAA = 0;
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

static inline void FXAA_FullScreenQuadDeleter(FXAA_FullScreenQuad* fxaa_FullScreenQuad) {

    if (fxaa_FullScreenQuad)
    {
        fxaa_FullScreenQuad->CleanUp();
        delete fxaa_FullScreenQuad;
    }
}

