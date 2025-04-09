#pragma once

#include <memory>
#include <string>
#include "BufferManager.h"
#include "VulkanContext.h"
#include "Camera.h"
#include "AssetManager.h"

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
    ~Model();
    void LoadTextures(const std::string& filepath);
    void CreateVertexAndIndexBuffer();
    void CreateUniformBuffer();
    void UpdateUniformBuffer(uint32_t currentImage);
    glm::mat4 GetModelMatrix();
    void createDescriptorSetLayout();

    void createDescriptorSets(vk::DescriptorPool descriptorpool);

    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex);
    void Clean();


    vk::CommandPool            commandPool;
    vk::DescriptorSetLayout descriptorSetLayout;
    
    std::vector<vk::DescriptorSet> DescriptorSets;

    std::vector<BufferData> uniformBuffers;
    BufferData VertexBufferData;
    BufferData IndexBufferData;
    ImageData MeshTextureData;

    std::unique_ptr<BufferManager> bufferManger = nullptr;
    std::shared_ptr<VulkanContext> vulkanContext = nullptr;
    

    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale    = glm::vec3(1.0f);

    UniformBufferObject1 ModelData;

private:

   // std::unique_ptr<MeshLoader>    meshLoader = nullptr;
    std::shared_ptr<Camera>        camera = nullptr;


    std::vector<void*> uniformBuffersMappedMem;
    std::vector<InstanceData> instances;
    const std::string filePath;


    StoredModelData storedModelData;
};

struct ModelDeleter {

    void operator()(Model* model) const {
        if (!model) return;

        if (model) {

            model->Clean();
          //  delete model;
        }
    }
};