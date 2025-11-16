#pragma once

#include <memory>
#include <string>
#include "VulkanContext.h"
#include "Drawable.h"
#include "SkyBox.h"
class RT_Shadows;

class Lighting_FullScreenQuad : public Drawable
{
public:

    Lighting_FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext, Camera* cameraref, vk::CommandPool commandpool, SkyBox* skyboxref, RT_Shadows* raytracingref);
    void CreateUniformBuffer() override;
    void createDescriptorSetLayout() override;
    void createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, GBuffer* Gbuffer, ImageData* RT_Reflection);
    void UpdateDescrptorSets();
    void UpdateUniformBuffer(uint32_t currentImage, std::vector<std::shared_ptr<Light>>& lightref);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;

    void CleanUp() ;

    GBuffer*   GbufferRef = nullptr;
    RT_Shadows* raytracingRef = nullptr;
    SkyBox* SkyBoxRef = nullptr;
    ImageData* RT_ReflectionRef = nullptr;
    int LightCount = 0;

   private:
   Camera* camera = nullptr;
};

static inline void Lighting_FullScreenQuadDeleter(Lighting_FullScreenQuad* fullScreenQuad) {
  
    if (fullScreenQuad)
    {
        fullScreenQuad->CleanUp();
        delete fullScreenQuad;
    }
}

