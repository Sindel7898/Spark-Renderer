#pragma once

#include <memory>
#include <string>
#include <vector>
#include "VulkanContext.h"
#include "Drawable.h"

class Lighting_RTX;

struct PostProcessSettings {
    glm::vec4 Brightness_Saturation_Concentration_GIboost;
    glm::vec4 MaxGamma_MinGamma_Padding;
};

class CombinedResult_FullScreenQuad : public Drawable
{
public:

    CombinedResult_FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext, Camera* cameraref, vk::CommandPool commandpool, Lighting_RTX* lighting);
    ~CombinedResult_FullScreenQuad();

    void createDescriptorSetLayout() override;
    void UpdataeUniformBufferData();
    void createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, ImageData LightingResultImage, ImageData SSGIImage, ImageData SSAOIImage, ImageData MaterialImage, ImageData AlbedoImage, ImageData DDGIImaGE);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;
    void DrawGammaCorrection(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);

    void CreateImage(vk::Extent3D imageExtent);
    void DestroyImage();

    ImageData IMGUI_PRESENT_IMAGE_GAMMA_CORRECTED;
    ImageData IMGUI_PRESENT_IMAGE;
    ImageData Combined_Lighting_Image;
    ImageData Final_Denoised_Image;

    float Brightness = 1.0;
    float Saturation = 1.0;
    float Concentration = 1.0;
    float MaxGamma = 0.98;
    float MinGamma = 0.92;
    float GIBoost = 1;

    vk::DescriptorSetLayout  Gamma_Correction_descriptorSetLayout;
    std::vector<vk::DescriptorSet>  Gamma_Correction_DescriptorSets;

    Lighting_RTX* lightingref;

private:
};