#pragma once

#include <memory>
#include <string>
#include "VulkanContext.h"
#include "Drawable.h"

struct PostProcessSettings {
    glm::vec4 Brightness_Saturation_Concentration_GIboost;
    glm::vec4 MaxGamma_MinGamma_Padding;
};
class CombinedResult_FullScreenQuad : public Drawable
{
public:

    CombinedResult_FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext, Camera* cameraref, vk::CommandPool commandpool);
    void createDescriptorSetLayout() override;
    void UpdataeUniformBufferData();
    void createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, ImageData LightingResultImage, ImageData SSGIImage, ImageData SSAOIImage, ImageData MaterialImage, ImageData AlbedoImage, ImageData DDGIImaGE);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;
    void CreateImage(vk::Extent3D imageExtent);
    void DestroyImage();

    void CleanUp();

    ImageData FinalResultImage;

    float Brightness = 1.0;
    float Saturation = 1.0;
    float Concentration = 1.0;
    float MaxGamma = 0.98;
    float MinGamma = 0.92;
    float GIBoost = 1;
private:
};

static inline void CombinedResult_FullScreenQuadDeleter(CombinedResult_FullScreenQuad* CombinedResult_fullScreenQuad) {

    if (CombinedResult_fullScreenQuad)
    {
        CombinedResult_fullScreenQuad->CleanUp();
        delete CombinedResult_fullScreenQuad;
    }
}

