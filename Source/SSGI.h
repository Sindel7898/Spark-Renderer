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
    void GenerateMipMaps(vk::CommandBuffer commandbuffer);
    void createDescriptorSetLayout() override;
    void createDescriptorSets(vk::DescriptorPool descriptorpool, GBuffer gbuffer, ImageData LightingPass, ImageData DepthImage);
    void UpdateUniformBuffer(uint32_t currentImage, std::vector<std::shared_ptr<Light>>& lightref,float DeltaTime);
    void CreateUniformBuffer();
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;

    void DrawTA(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);

    void DrawDownSampleHalfResFirstPass(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);
    void DrawDownSampleHalfResSecondPass(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);
    void DrawDownSampleQuaterfResFirstPass(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);
    void DrawDownSampleQuaterfResSecondPass(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);
    void DrawUPSampleHalfResFirstPass(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);
    void DrawUPSampleHalfResSecondPass(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);
    void DrawUPSampleFullResFirstPass(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);
    void DrawUPSampleFullResSecondPass(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);

 
    void CleanUp() ;

    vk::DescriptorSetLayout TemporalAccumilationDescriptorSetLayout;
    std::vector<vk::DescriptorSet> TemporalAccumilationFullDescriptorSets;

    vk::DescriptorSetLayout Blured_TemporalAccumilationDescriptorSetLayout;

    std::vector<vk::DescriptorSet> DownSampleHalfRes_PING_SampleDescriptorSets;
    std::vector<vk::DescriptorSet> DownSampleHalfRes_PONG_SampleDescriptorSets;

    std::vector<vk::DescriptorSet> DownSampleQuaterRes_PING_SampleDescriptorSets;
    std::vector<vk::DescriptorSet> DownSampleQuaterRes_PONG_SampleDescriptorSets;

    std::vector<vk::DescriptorSet> UPSampleHalfRes_PING_SampleDescriptorSets;
    std::vector<vk::DescriptorSet> UPSampleHalfRes_PONG_SampleDescriptorSets;

    std::vector<vk::DescriptorSet> UPSampleFullRes_PING_SampleDescriptorSets;
    std::vector<vk::DescriptorSet> UPSampleFullRes_PONG_SampleDescriptorSets;

    std::vector<ImageData> BlueNoiseTextures;

    ImageData SSGIPassImage;
    ImageData SSGIPassLastFrameImage;
    ImageData SSGIAccumilationImage;

    ImageData BlurPing_DownSampleHalfRes;
    ImageData BlurPong_DownSampleHalfRes;

    ImageData BlurPing_DownSampleQuaterRes;
    ImageData BlurPong_DownSampleQuaterRes;

    ImageData BlurPing_UPSampleHalfRes;
    ImageData BlurPong_UPSampleHalfRes;

    ImageData BlurPing_UPSampleFullRes;
    ImageData BlurPong_UPSampleFullRes;

    vk::Extent3D SSGI_ImageFullResolution;
    vk::Extent3D SSGI_ImageHalfResolution;
    vk::Extent3D SSGI_ImageQuaterResolution;

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
};


static inline void SSGIDeleter(SSGI* ssgi) {

        if (ssgi) {
            ssgi->CleanUp();
            delete ssgi;
        }
   
};