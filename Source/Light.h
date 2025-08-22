#pragma once

#include <memory>
#include <string>
#include "VertexInputLayouts.h"
#include "Drawable.h"
#include "VulkanContext.h"
#include "Instances.h"

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
    void Instantiate();
    void Destroy(int instanceIndex);

    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;

    void CleanUp();

    std::vector<Light_InstanceData*>     Instances;
    std::vector<std::shared_ptr<Light_GPU_InstanceData>> GPU_InstancesData;

    std::vector<BufferData> Light_GPU_DataUniformBuffers;
    std::vector<void*>      Light_GPU_DataUniformBuffersMappedMem;

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