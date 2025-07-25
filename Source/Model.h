#pragma once

#include <memory>
#include <string>
#include "VulkanContext.h"
#include "Camera.h"
#include "AssetManager.h"
#include "Light.h"
#include "Drawable.h"
#include "structs.h"

struct ModelData {
    TransformMatrices  transformMatrices;
	glm::vec4  bCubeMapReflection_bScreenSpaceReflectionWithPadding;

};

class Model : public Drawable
{
public:

    Model(const std::string filepath, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger);
    void LoadTextures();
    void CreateVertexAndIndexBuffer() override;
    void createDescriptorSetLayout() override;
    void createDescriptorSets(vk::DescriptorPool descriptorpool) override;
    void CreateUniformBuffer() override;
    void UpdateUniformBuffer(uint32_t currentImage);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;
    void CubeMapReflectiveSwitch(bool breflective);
    void ScreenSpaceReflectiveSwitch(bool breflective);
    void CreateBLAS();

    void CleanUp() ;


    ImageData  albedoTextureData;
    ImageData  normalTextureData;
    ImageData  MetallicRoughnessTextureData;

    VertexUniformData vertexdata{};

    int bCubeMapReflection = 0;
    int bScreenSpaceReflection = 0;

    vk::AccelerationStructureKHR BLAS;

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