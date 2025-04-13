#pragma once

#include <memory>
#include <string>
#include "BufferManager.h"
#include "VulkanContext.h"
#include "Camera.h"
#include "AssetManager.h"
#include "Light.h"

struct VertexUniformBufferObject1 {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct FragmentUniformBufferObject1 {
    alignas(16) glm::vec3  LightLocation;
    alignas(16) glm::vec4  BaseColor;
    alignas(16) float      AmbientStrength;
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
    void LoadTextures();
    void CreateVertexAndIndexBuffer();
    void CreateUniformBuffer();
    void UpdateUniformBuffer(uint32_t currentImage, Light* light);
    void BreakDownModelMatrix();
    glm::mat4 GetModelMatrix();
    void createDescriptorSetLayout();

    void createDescriptorSets(vk::DescriptorPool descriptorpool);

    void SetModelMatrix(glm::mat4 newModelMatrix);

    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex);
    void Clean();


    vk::CommandPool            commandPool;
    vk::DescriptorSetLayout descriptorSetLayout;
    
    std::vector<vk::DescriptorSet> DescriptorSets;

    std::vector<BufferData> VertexuniformBuffers;
    std::vector<BufferData> FragmentuniformBuffers;

    BufferData VertexBufferData;
    BufferData IndexBufferData;
    ImageData AlbedoTextureData;
    ImageData NormalTextureData;

    std::unique_ptr<BufferManager> bufferManger = nullptr;
    std::shared_ptr<VulkanContext> vulkanContext = nullptr;
    

    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale   ;

    VertexUniformBufferObject1 ModelData;
    FragmentUniformBufferObject1 LightData;

private:

   // std::unique_ptr<MeshLoader>    meshLoader = nullptr;
    std::shared_ptr<Camera>        camera = nullptr;


    std::vector<void*> VertexUniformBuffersMappedMem;
    std::vector<void*> FragmentUniformBuffersMappedMem;

    std::vector<InstanceData> instances;
    const std::string FilePath;


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