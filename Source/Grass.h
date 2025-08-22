#pragma once

#include <memory>
#include <string>
#include "VulkanContext.h"
#include "Camera.h"
#include "AssetManager.h"
#include "Light.h"
#include "Drawable.h"
#include "structs.h"
#include <meshoptimizer.h>

struct GrassData
{
    glm::mat4 ModelMatrix;
};

class Grass : public Drawable
{
public:

    Grass(const std::string filepath, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger);
    void LoadTextures();
    void GeneratePositionalData();
    void CreateVertexAndIndexBuffer() override;
    void createDescriptorSetLayout() override;
    void createDescriptorSets(vk::DescriptorPool descriptorpool) override;
    void CreateUniformBuffer() override;
    void UpdateUniformBuffer(uint32_t currentImage);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;
    void CleanUp() ;


    std::vector<GrassData> InsanceData;

    ImageData  albedoTextureData;
    ImageData  normalTextureData;
    ImageData  MetallicRoughnessTextureData;
    ImageData  HeighMapTextureData;

    const StoredModelData* storedModelData = nullptr;

    VertexUniformData vertexdata{};


    VkDeviceSize vertexBufferSize;
    VkDeviceSize IndexBufferSize;

    std::vector<BufferData> GrassDataStorageBuffers;
    std::vector<void*>      GrassDataStorageBuffersMappedMem;
    VkDeviceSize  GrassDatavertexBufferSize;

private:

    std::string FilePath;
    float DeltaTime;
};


static inline void GrassDeleter(Grass* model) {

        if (model) {
            model->CleanUp();
            delete model;
        }
   
};