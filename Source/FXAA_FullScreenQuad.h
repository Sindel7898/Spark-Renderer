#pragma once

#include <memory>
#include <string>
#include <vector>
#include "VulkanContext.h"
#include "Drawable.h"

class FXAA_FullScreenQuad : public Drawable
{
public:
    FXAA_FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext, Camera* cameraref, vk::CommandPool commandpool);
    ~FXAA_FullScreenQuad();

    void CreateImage(vk::Extent3D ImageEXtent);
    void DestroyImage();
    void createDescriptorSetLayout() override;
    void createDescriptorSets(vk::DescriptorPool descriptorpool, ImageData LightingPass);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;

    ImageData FxaaImage;

    glm::vec4 bFXAA_Padding;
    int bFXAA = 1;

private:
    Camera* camera = nullptr;
};