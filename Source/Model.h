#pragma once

#include <memory>
#include <string>
#include "BufferManager.h"
#include "MeshLoader.h"
#include "VulkanContext.h"
#include "Camera.h"


struct UniformBufferObject1 {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct InstanceData {
    BufferData uniformBuffer;
    void* mappedMemory;
    vk::DescriptorSet descriptorSet;
    uint32_t instanceID; // Unique identifier
};

class Model
{
public:

    Model(const std::string& filepath, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* camera, BufferManager* buffermanger);

    void LoadTextures(const char* FilePath);
    void CreateVertexAndIndexBuffer();
    void CreateUniformBuffer();
    void UpdateUniformBuffer(uint32_t currentImage, float Location);
    void createDescriptorSetLayout();

    void createDescriptorSets(vk::DescriptorPool descriptorpool);

    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex);


    vk::CommandPool            commandPool;
    vk::DescriptorSetLayout descriptorSetLayout;
    
    std::vector<vk::DescriptorSet> DescriptorSets;

    std::vector<BufferData> uniformBuffers;
    BufferData VertexBufferData;
    BufferData IndexBufferData;
    ImageData MeshTextureData;

    std::unique_ptr<BufferManager> bufferManger = nullptr;
    std::shared_ptr<VulkanContext> vulkanContext = nullptr;

private:

    std::unique_ptr<MeshLoader>    meshLoader = nullptr;
    std::shared_ptr<Camera>        camera = nullptr;


    std::vector<void*> uniformBuffersMappedMem;
    std::vector<InstanceData> instances;

};

struct ModelDeleter {

    void operator()(Model* model) const {

        if (model) {

            model->bufferManger->DestroyImage(model->MeshTextureData);
            model->bufferManger->DestroyBuffer(model->VertexBufferData);
            model->bufferManger->DestroyBuffer(model->IndexBufferData);

            model->vulkanContext->LogicalDevice.destroyImageView(model->MeshTextureData.imageView);
            model->vulkanContext->LogicalDevice.destroySampler(model->MeshTextureData.imageSampler);

            delete model;
        }
    }
};