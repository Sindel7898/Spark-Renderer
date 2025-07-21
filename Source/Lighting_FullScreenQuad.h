#pragma once

#include <memory>
#include <string>
#include "VulkanContext.h"
#include "Drawable.h"
#include "Structs.h"
#include "SkyBox.h"


struct LightUniformData {
     glm::vec4  lightPositionAndLightType;
     glm::vec4  colorAndAmbientStrength;
     glm::vec4  CameraPositionAndLightIntensity;
     glm::mat4  LightViewProjMatrix;
};

class Lighting_FullScreenQuad : public Drawable
{
public:

    Lighting_FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext,Camera* cameraref, vk::CommandPool commandpool, SkyBox* skyboxref);
    void CreateVertexAndIndexBuffer() override;
    void CreateUniformBuffer() override;
    void createDescriptorSetLayout() override;
    void createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, GBuffer Gbuffer, ImageData ReflectionMask);
    void UpdateUniformBuffer(uint32_t currentImage, std::vector<std::shared_ptr<Light>>& lightref);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;

    void CleanUp() ;

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


        ImageData* ReflectiveCubeMapData;
        Camera* camera = nullptr;
};

static inline void Lighting_FullScreenQuadDeleter(Lighting_FullScreenQuad* fullScreenQuad) {
  
    if (fullScreenQuad)
    {
        fullScreenQuad->CleanUp();
        delete fullScreenQuad;
    }
}

