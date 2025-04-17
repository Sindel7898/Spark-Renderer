#pragma once

#include <memory>
#include <string>
#include "VulkanContext.h"
#include "Camera.h"
#include "AssetManager.h"
#include "Light.h"
#include "Drawable.h"



class Model : public Drawable
{
public:

    Model(const std::string  filepath, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger);
    void ModelDeleter(Model* model);
    void LoadTextures();
    void CreateVertexAndIndexBuffer() override;
    void createDescriptorSetLayout() override;
    void createDescriptorSets(vk::DescriptorPool descriptorpool) override;
    void CreateUniformBuffer() override;
    void UpdateUniformBuffer(uint32_t currentImage, Light* lightref);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;
    void CleanUp() ;


    ImageData  albedoTextureData;
    ImageData  normalTextureData;
    

   // LightUniformData lightData;
private:

    std::string FilePath;
    StoredModelData storedModelData;
};


struct ModelDeleter {

    void operator()(Model* model) const {

        if (model) {
            model->CleanUp();
            delete model;
        }
    }
};