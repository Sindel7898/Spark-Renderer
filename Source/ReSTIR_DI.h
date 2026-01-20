#pragma once
#include <vulkan/vulkan.hpp>
#include "structs.h"


class  Camera;
class  VulkanContext;
class  BufferManager;
class  Lighting_RTX;
class  SSGI;
class  DynamicDiffuse_RTGI;

struct PushConstant {
    glm::vec4 CameraPosition;
    glm::vec4 ScreenSize;
    SampleGridInfo sampleGridInfo;
	glm::vec4 Temporal_Spatial_Reuse_EnableDDGI_DDGI_Vertex_Flags;
    glm::mat4  ProjectionViewMatrix;
};

class ReSTIR_DI
{
public:
    ReSTIR_DI(VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger, Lighting_RTX* rLightingPass, SSGI* rssgi, DynamicDiffuse_RTGI* DDGIr);
    void createDescriptorSetLayout();

    void createDescriptorDDGIATLAS(vk::DescriptorPool descriptorpool);

    void createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, vk::AccelerationStructureKHR* TLAS);
    void UpdateDescrptorSets();
    void DestroyImage();
    void CreateImage();
    void CleanUp();
    void DispatchResevoirCandidateCalcCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex);
    void Draw(BufferData RayGenBuffer, BufferData RayHitBuffer, BufferData RayMisBuffer, vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);
    uint32_t alignedSize(uint32_t value, uint32_t alignment);
    //vk::DescriptorSetLayout RservoirSamplingDescriptorSetLayout;
    vk::DescriptorSetLayout RayTracingDescriptorSetLayout;
    vk::DescriptorSetLayout DDGIATLASDescriptorSetLayout;

    ImageData ResevoirImage;
    ImageData PrevResevoirImage;
    ImageData ReSTIRDI_Results;

    bool bTemporalReuse = true;
	bool bSpatialReuse = true;
    bool bDDGI = true;
	int  DDGIVertex = 0;

private:

    VulkanContext* vulkanContext = nullptr;
    BufferManager* bufferManager = nullptr;
    Camera* camera = nullptr;
    vk::CommandPool commandPool = nullptr;

    Lighting_RTX* LightingPass;
    SSGI* ssgi;
    DynamicDiffuse_RTGI* DDGIRef;
   // std::vector<vk::DescriptorSet>  RservoirSamplingProbeDescriptorSets;
    std::vector<vk::DescriptorSet>  RaytracingDescriptorSets;
    std::vector<vk::DescriptorSet>  RaytracingDDGIDescriptorSets;

    vk::AccelerationStructureKHR*   TLASr;

};

static inline void ReSTIR_DI_Deleter(ReSTIR_DI* restir) {

    if (restir) {
        restir->CleanUp();
        delete restir;
    }

};