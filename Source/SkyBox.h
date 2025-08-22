#pragma once

#include <memory>
#include <string>
#include "BufferManager.h"
#include "VulkanContext.h"
#include "Camera.h"
#include "VertexInputLayouts.h"
#include "Drawable.h"

struct SkyBoxVertexData
{
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
};

class SkyBox : public Drawable
{
public:
    SkyBox(VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* camera, BufferManager* buffermanger);

    void CreateVertexAndIndexBuffer() override;
    void CreateUniformBuffer() override;
    void createDescriptorSets(vk::DescriptorPool descriptorpool) override;
    void UpdateDescriptorSets();
    void UpdateUniformBuffer(uint32_t currentImage) override;
    void createDescriptorSetLayout() override;
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;

    void CleanUp();


    std::vector<ImageData> SkyBoxImages;
    int SkyBoxIndex;
    int LastSkyBoxIndex;
    bool bSkyBoxUpdate;

    SkyBoxVertexData transformMatrices;
private:

    const std::vector<VertexOnly> vertices = {

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

static inline void SkyBoxDeleter(SkyBox* skyBox){

        if (skyBox) {
            skyBox->CleanUp();
            delete skyBox;
        }
};