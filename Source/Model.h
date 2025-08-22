#pragma once

#include <memory>
#include <string>
#include "VulkanContext.h"
#include "Camera.h"
#include "AssetManager.h"
#include "Light.h"
#include "Drawable.h"
#include "structs.h"
#include "Instances.h"


class Model : public Drawable
{
public:

    Model(const std::string filepath, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger);
    void LoadTextures();
    void CreateVertexAndIndexBuffer() override;
    void createDescriptorSetLayout() override;
    void createDescriptorSets(vk::DescriptorPool descriptorpool) override;
    void CreateUniformBuffer() override;
    void Instantiate();
    void Destroy(int instanceIndex);
    void UpdateUniformBuffer(uint32_t currentImage);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;
    void CreateBLAS();

    void CleanUp() ;

    vk::DescriptorSetLayout  RayTracingDescriptorSetLayout;

    ImageData  albedoTextureData;
    ImageData  normalTextureData;
    ImageData  MetallicRoughnessTextureData;

    vk::AccelerationStructureKHR BLAS;

    std::vector<Model_InstanceData*>     Instances;
    std::vector<std::shared_ptr<Model_GPU_InstanceData>> GPU_InstancesData;

    std::vector<BufferData> Model_GPU_DataUniformBuffers;
    std::vector<void*>      Model_GPU_DataUniformBuffersMappedMem;

private:


    std::string FilePath;
    const StoredModelData* storedModelData = nullptr;
    BufferData BLAS_Buffer;
    BufferData BLAS_ScratchBuffer;

};


static inline void ModelDeleter(Model* model) {

        if (model) {
            model->CleanUp();
            delete model;
        }
   
};