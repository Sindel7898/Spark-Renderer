#pragma once

#include <memory>
#include <string>
#include "VulkanContext.h"
#include "Drawable.h"

struct Vertex {
    glm::vec2 position;
    glm::vec2 uv;
};

struct GBuffer {
    ImageData Position;
    ImageData Normal;
    ImageData Albedo;
};

class FullScreenQuad : public Drawable
{
public:

    FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext, vk::CommandPool commandpool);
    void CreateVertexAndIndexBuffer() override;
    void createDescriptorSetLayout() override;
    void createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, GBuffer Gbuffer) ;
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;
    ////////////////Unneeded

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

};


