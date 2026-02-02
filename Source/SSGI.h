#pragma once

#include <memory>
#include <string>
#include "structs.h"

class  Camera;
class  VulkanContext;
class  BufferManager;
class  Lighting_RTX;

struct SSGI_UniformBufferData {
    glm::mat4 ProjectionMatrix;
    glm::mat4 ViewMatrix;
    glm::vec4 BlueNoiseImageIndex_WithPadding;
};


class SSGI
{
public:

    SSGI(BufferManager* buffermanager, VulkanContext* vulkancontext, Camera* cameraref, vk::CommandPool commandpool, Lighting_RTX* lighting);
    ~SSGI();

    void CreateNoiseTextures();
    void CreateGIImage();
    void DestroyImage();
    void createDescriptorSetLayout();
    void createDescriptorSets(vk::DescriptorPool descriptorpool, GBuffer gbuffer, ImageData LightingPass, ImageData DepthImage);
    void UpdateUniformBuffer(uint32_t currentImage, float DeltaTime);
    void ComputeSSGI(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);
    void CreateUniformBuffer();    

    std::vector<ImageData> BlueNoiseTextures;

    ImageData SSGIPassImage;
    vk::Extent3D SSGI_ImageFullResolution;

    int NoiseIndex;
    vk::DescriptorSetLayout  descriptorSetLayout;

private:
   
    glm::mat4 LastCameraMatrix;

    std::vector<BufferData> ComputeUniformBuffers;
    std::vector<void*>      ComputeUniformBuffersMappedMem;
    std::vector<vk::DescriptorSet> DescriptorSets;


    BufferManager* bufferManager = nullptr;
    VulkanContext* vulkanContext = nullptr;
    Camera* camera = nullptr;
    vk::CommandPool commandPool;
    Lighting_RTX* lightingref;

};

