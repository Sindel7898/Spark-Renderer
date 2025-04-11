#pragma once

#include <memory>
#include <string>
#include "BufferManager.h"
#include "VertexInputLayouts.h"
#include "VulkanContext.h"

class Camera;

struct LightVertexUniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec4 BaseColor;
};

class Light
{
public:

    Light(VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* camera, BufferManager* buffermanger);

    void CreateVertex();
    void CreateUniformBuffer();
    void createDescriptorSets(vk::DescriptorPool descriptorpool);
    void UpdateUniformBuffer(uint32_t currentImage);
    void createDescriptorSetLayout();

    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex);

    glm::mat4 GetModelMatrix();


    vk::CommandPool            commandPool;
    std::vector<BufferData> VertexuniformBuffers;

    BufferData VertexBufferData;

    std::unique_ptr<BufferManager> bufferManger = nullptr;
    std::shared_ptr<VulkanContext> vulkanContext = nullptr;
    vk::DescriptorSetLayout descriptorSetLayout;
    std::vector<vk::DescriptorSet> DescriptorSets;

    glm::vec3  LightLocation;
    glm::vec3  LightScale;
    glm::vec3  LightRotation;

    glm::vec4  BaseColor;
    float      AmbientStrength = 0.0f;
    
    LightVertexUniformBufferObject ModelData;
private:

    std::shared_ptr<Camera>    camera = nullptr;


    std::vector<void*> VertexUniformBuffersMappedMem;


    const std::vector<VertexOnly> vertices = {
        // Front face (Z = -1.0f)
        {{-1.0f, -1.0f, -1.0f}}, {{ 1.0f, -1.0f, -1.0f}}, {{ 1.0f,  1.0f, -1.0f}},
        {{ 1.0f,  1.0f, -1.0f}}, {{-1.0f,  1.0f, -1.0f}}, {{-1.0f, -1.0f, -1.0f}},

        // Back face (Z = 1.0f)
        {{-1.0f, -1.0f,  1.0f}}, {{ 1.0f, -1.0f,  1.0f}}, {{ 1.0f,  1.0f,  1.0f}},
        {{ 1.0f,  1.0f,  1.0f}}, {{-1.0f,  1.0f,  1.0f}}, {{-1.0f, -1.0f,  1.0f}},

        // Left face (X = -1.0f)
        {{-1.0f, -1.0f, -1.0f}}, {{-1.0f,  1.0f, -1.0f}}, {{-1.0f,  1.0f,  1.0f}},
        {{-1.0f,  1.0f,  1.0f}}, {{-1.0f, -1.0f,  1.0f}}, {{-1.0f, -1.0f, -1.0f}},

        // Right face (X = 1.0f)
        {{ 1.0f, -1.0f, -1.0f}}, {{ 1.0f,  1.0f, -1.0f}}, {{ 1.0f,  1.0f,  1.0f}},
        {{ 1.0f,  1.0f,  1.0f}}, {{ 1.0f, -1.0f,  1.0f}}, {{ 1.0f, -1.0f, -1.0f}},

        // Top face (Y = 1.0f)
        {{-1.0f,  1.0f, -1.0f}}, {{ 1.0f,  1.0f, -1.0f}}, {{ 1.0f,  1.0f,  1.0f}},
        {{ 1.0f,  1.0f,  1.0f}}, {{-1.0f,  1.0f,  1.0f}}, {{-1.0f,  1.0f, -1.0f}},

        // Bottom face (Y = -1.0f)
        {{-1.0f, -1.0f, -1.0f}}, {{ 1.0f, -1.0f, -1.0f}}, {{ 1.0f, -1.0f,  1.0f}},
        {{ 1.0f, -1.0f,  1.0f}}, {{-1.0f, -1.0f,  1.0f}}, {{-1.0f, -1.0f, -1.0f}}
    };

};