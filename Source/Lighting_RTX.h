#pragma once

#include <memory>
#include <string>
#include "VulkanContext.h"
#include "SkyBox.h"
class RT_Shadows;

struct Lightin_RTX_PC {
	glm::vec2 ScreenSize;
	int       LightCount;
	int       FrameIndex;
    glm::vec4 GI_Solution_Index_Padding;
};

enum GI_Solution
{
    _None = 0,
    _DDGI = 1,
	_SSGI = 2,
	_DDGI_AND_SSGI = 3,
	_PathTracing = 4
};

class Lighting_RTX
{
public:

    Lighting_RTX(BufferManager* buffermanager, VulkanContext* vulkancontext, Camera* cameraref, vk::CommandPool commandpool, SkyBox* skyboxref);
    void CreateUniformBuffer();
    void CreateStorageImage();
    void DestroyStorageImage();
    void createDescriptorSetLayout();
    void createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, GBuffer* Gbuffer, vk::AccelerationStructureKHR* TLAS);
    void UpdateDescrptorSets();
    void UpdateUniformBuffer(uint32_t currentImage, std::vector<std::shared_ptr<Light>>& lightref);
    uint32_t alignedSize(uint32_t value, uint32_t alignment);

    void Draw(BufferData RayGenBuffer, BufferData RayHitBuffer, BufferData RayMisBuffer, vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);

    void CleanUp();
    
  

    std::vector<BufferData> UniformBuffers;
    std::vector<void*>      UniformBuffersMappedMem;

    ImageData ResultingStorageImage;

    GBuffer* GbufferRef = nullptr;

    vk::DescriptorSetLayout  descriptorSetLayout;

    int LightCount = 0;
    int GISolutionIndex = 0;

   private:

   BufferManager* bufferManager = nullptr;
   VulkanContext* vulkanContext = nullptr;
   Camera* camera = nullptr;

   vk::CommandPool                commandPool;
   std::vector<vk::DescriptorSet> DescriptorSets;

   vk::Extent3D swapchainextent;
   vk::AccelerationStructureKHR* TLASr;

   SkyBox* SkyBoxRef = nullptr;
   int frameIndex = 0;
};

static inline void Lighting_RTXDeleter(Lighting_RTX* ref) {
  
    if (ref)
    {
        ref->CleanUp();
        delete ref;
    }
}

