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

    Model(const std::string filepath, std::string modelindex, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger);
    void LoadTextures();
    void CreateVertexAndIndexBuffer() override;
    void createDescriptorSetLayout() override;
    void createDescriptorSets(vk::DescriptorPool descriptorpool) override;
    void CreateBottomLevelAccelerationStructure();
    void CreateUniformBuffer() override;
    void UpdateUniformBuffer(uint32_t currentImage, Light* lightref);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;
    void CleanUp() ;


    ImageData  albedoTextureData;
    ImageData  normalTextureData;
    

   // vk::AccelerationStructureKHR GetBottomLevelAS() const { return bottomLevelAS; }
    uint64_t  GetBLASAddressInfo();
    
private:

    std::string FilePath;
    std::string ModelIndex;;

    StoredModelData storedModelData;

    //vk::AccelerationStructureKHR bottomLevelAS;

    BufferData bottomLevelASBuffer;
    BufferData scratchBuffer;
};


static inline void ModelDeleter(Model* model) {

        if (model) {
            model->CleanUp();
            delete model;
        }
   
};