#pragma once

#include <memory>
#include <string>
#include "VulkanContext.h"
#include "Camera.h"
#include "AssetManager.h"
#include "Light.h"
#include "Drawable.h"
#include "structs.h"
#include "SkyBox.h"

struct ModelData {
    TransformMatrices  transformMatrices;
	glm::vec4 IsReflectiveWithPadding;

};

class Model : public Drawable
{
public:

    Model(const std::string filepath, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger, SkyBox* skyboxref);
    void LoadTextures();
    void CreateVertexAndIndexBuffer() override;
    void createDescriptorSetLayout() override;
    void createDescriptorSets(vk::DescriptorPool descriptorpool) override;
    void CreateUniformBuffer() override;
    void UpdateUniformBuffer(uint32_t currentImage);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;
    void ReflectiveSwitch(bool breflective);
    void CleanUp() ;


    ImageData  albedoTextureData;
    ImageData  normalTextureData;
    ImageData  MetallicRoughnessTextureData;

    VertexUniformData vertexdata{};

private:

	int IsReflective = 0;

	ImageData* ReflectiveCubeMapData;
    std::string FilePath;
    const StoredModelData* storedModelData = nullptr;
    BufferData bottomLevelASBuffer;
    BufferData scratchBuffer;
};


static inline void ModelDeleter(Model* model) {

        if (model) {
            model->CleanUp();
            delete model;
        }
   
};