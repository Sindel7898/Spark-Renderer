#pragma once

#include <memory>
#include <string>
#include "BufferManager.h"
#include "MeshLoader.h"
#include "VulkanContext.h"
#include "Camera.h"


struct SkyBoxUniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};


class SkyBox
{
public:
    SkyBox(std::array<const char*, 6> filePaths, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* camera, BufferManager* buffermanger);

    void CreateVertex();
    void CreateUniformBuffer();
    void createDescriptorSets(vk::DescriptorPool descriptorpool);
    void UpdateUniformBuffer(uint32_t currentImage);
    void createDescriptorSetLayout();

    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex);


    vk::CommandPool            commandPool;
    std::vector<BufferData> uniformBuffers;

    BufferData VertexBufferData;
    ImageData MeshTextureData;

    std::unique_ptr<BufferManager> bufferManger = nullptr;
    std::shared_ptr<VulkanContext> vulkanContext = nullptr;
    vk::DescriptorSetLayout descriptorSetLayout;
    std::vector<vk::DescriptorSet> DescriptorSets;

private:

    std::shared_ptr<Camera>        camera = nullptr;


    std::vector<void*> uniformBuffersMappedMem;


    const std::vector<SkyBoxVertex> vertices = {

        {{-1.0f,  1.0f, -1.0f}}, {{-1.0f, -1.0f, -1.0f}}, {{ 1.0f, -1.0f, -1.0f}},
        {{ 1.0f, -1.0f, -1.0f}}, {{ 1.0f,  1.0f, -1.0f}}, {{-1.0f,  1.0f, -1.0f}},

        {{-1.0f, -1.0f,  1.0f}}, {{-1.0f, -1.0f, -1.0f}}, {{-1.0f,  1.0f, -1.0f}},
        {{-1.0f,  1.0f, -1.0f}}, {{-1.0f,  1.0f,  1.0f}}, {{-1.0f, -1.0f,  1.0f}},

        {{ 1.0f, -1.0f, -1.0f}}, {{ 1.0f, -1.0f,  1.0f}}, {{ 1.0f,  1.0f,  1.0f}},
        {{ 1.0f,  1.0f,  1.0f}}, {{ 1.0f,  1.0f, -1.0f}}, {{ 1.0f, -1.0f, -1.0f}},

        {{-1.0f, -1.0f,  1.0f}}, {{-1.0f,  1.0f,  1.0f}}, {{ 1.0f,  1.0f,  1.0f}},
        {{ 1.0f,  1.0f,  1.0f}}, {{ 1.0f, -1.0f,  1.0f}}, {{-1.0f, -1.0f,  1.0f}},

        {{-1.0f,  1.0f, -1.0f}}, {{ 1.0f,  1.0f, -1.0f}}, {{ 1.0f,  1.0f,  1.0f}},
        {{ 1.0f,  1.0f,  1.0f}}, {{-1.0f,  1.0f,  1.0f}}, {{-1.0f,  1.0f, -1.0f}},

        {{-1.0f, -1.0f, -1.0f}}, {{-1.0f, -1.0f,  1.0f}}, {{ 1.0f, -1.0f, -1.0f}},
        {{ 1.0f, -1.0f, -1.0f}}, {{-1.0f, -1.0f,  1.0f}}, {{ 1.0f, -1.0f,  1.0f}}
    };

};

struct SkyBoxDeleter {

    void operator()(SkyBox* skybox) const {

        if (skybox) {

            skybox->vulkanContext->LogicalDevice.destroyDescriptorSetLayout(skybox->descriptorSetLayout);

            skybox->bufferManger->DestroyImage(skybox->MeshTextureData);
            skybox->bufferManger->DestroyBuffer(skybox->VertexBufferData);
            
            for (auto& uniformBuffer : skybox->uniformBuffers) {

                skybox->bufferManger->UnmapMemory(uniformBuffer);
                skybox->bufferManger->DestroyBuffer(uniformBuffer);
            }

            //delete skybox;
        }
    }
};