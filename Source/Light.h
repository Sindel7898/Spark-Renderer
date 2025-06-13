#pragma once

#include <memory>
#include <string>
#include "VertexInputLayouts.h"
#include "Drawable.h"

class Camera;


class Light : public Drawable
{
public:

    Light(VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* camera, BufferManager* buffermanger);

    void CreateVertexAndIndexBuffer() override;
    void CreateUniformBuffer() override;
    void createDescriptorSets(vk::DescriptorPool descriptorpool) override;
    void UpdateUniformBuffer(uint32_t currentImage) override;
    void createDescriptorSetLayout() override;
 

    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;

    void CleanUp();

    glm::vec3  color;
    float      ambientStrength;
    float      lightIntensity;

    int        lightType;

private:

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


static inline void LightDeleter(Light* light) {

        if (light) {
            light->CleanUp();
            delete light;
        }
};