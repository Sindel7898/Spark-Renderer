#pragma once

#include <memory>
#include <string>
#include "structs.h"
#include "Drawable.h"

class  Camera;
class  VulkanContext;
class  BufferManager;

struct SSGI_UniformBufferData {
    glm::mat4 ProjectionMatrix;
    glm::vec4 BlueNoiseImageIndex_WithPadding;

};


class SSGI : public Drawable
{
public:

    SSGI(BufferManager* buffermanager, VulkanContext* vulkancontext, Camera* cameraref, vk::CommandPool commandpool);
    void CreateVertexAndIndexBuffer() override;
    void CreateGIImage();
    void DestroyImage();
    void DownSampleToQuaterResolution(vk::CommandBuffer commandbuffer);
    void GenerateMipMaps(vk::CommandBuffer commandbuffer);
    void createDescriptorSetLayout() override;
    void createDescriptorSets(vk::DescriptorPool descriptorpool, GBuffer gbuffer, ImageData LightingPass, ImageData DepthImage);
    void UpdateUniformBuffer(uint32_t currentImage, std::vector<std::shared_ptr<Light>>& lightref,float DeltaTime);
    void CreateUniformBuffer();
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;

    void DrawTA(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);

    void CleanUp() ;
    std::vector<BufferData> SSGI_UniformBuffers;

    std::vector<void*> SSGI_UniformBuffersMappedMem;
    std::vector<vk::DescriptorSet> TemporalAccumilationFullDescriptorSets;

    vk::DescriptorSetLayout  BilateralFilterDescriptorSetLayout;

    ImageData SSGIPassImage;
    ImageData SSGIPassLastFrameImage;
    ImageData SSGIAccumilationImage;


    std::vector<ImageData> BlueNoiseTextures;
    int NoiseIndex;

    glm::mat4 LastCameraMatrix;
private:
    std::vector<Vertex> quad = {
     {{-1.0f, -1.0f}, {0.0f, 0.0f}}, // Bottom-left
     {{ 1.0f, -1.0f}, {1.0f, 0.0f}}, // Bottom-right
     {{-1.0f,  1.0f}, {0.0f, 1.0f}}, // Top-left
     {{ 1.0f,  1.0f}, {1.0f, 1.0f}}  // Top-right
    };

    const std::vector<uint16_t> quadIndices = {
           0, 1, 2,
           2, 1, 3
    };

    VulkanContext*   vulkanContext = nullptr;
    BufferManager*   bufferManager = nullptr;
    Camera*          camera        = nullptr;
    vk::CommandPool commandPool = nullptr;
};


static inline void SSGIDeleter(SSGI* ssgi) {

        if (ssgi) {
            ssgi->CleanUp();
            delete ssgi;
        }
   
};