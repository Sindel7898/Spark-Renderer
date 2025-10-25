#pragma once

#include <memory>
#include <string>
#include "structs.h"
#include <vulkan/vulkan.hpp>
#include "MeshLoader.h"

class  Camera;
class  VulkanContext;
class  BufferManager;
class  Model;
class  Light;
class  SkyBox;

struct GridData
{
    glm::vec4 probeCount;
    glm::vec4 probeOffset;
    glm::vec4 probeBaseLocation;
	glm::mat4 RotationMatrix;
};

struct GeneralAtlasInfo
{
    int  AtlasWidthSize;
    int  ProbeSideLength;
    int  GutterSize;
    int  RaysPerProbe;
};

struct SampleGridInfo
{
    glm::vec4 GridBaseLocation_ScreenSizeWidth;
    glm::vec4 ProbeSpacing_ScreenSizeHeight;
    glm::vec4 ProbeCount;
    GeneralAtlasInfo generalAtlasInfo;
};


struct CameraConstantBuffer
{
    glm::mat4 ViewMatrix;
    glm::mat4 ProjectionMatrix;
    GeneralAtlasInfo generalAtlasInfo;
};


class DynamicDiffuse_RTGI
{
public:

    DynamicDiffuse_RTGI(const std::string filepath,VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger, SkyBox* skybox);
    void CreateStorageBuffer();
    void createRayTracingDescriptorSetLayout();
    void createRaytracedDescriptorSets(vk::DescriptorPool descriptorpool, vk::AccelerationStructureKHR TLAS, std::vector<BufferData>& fragmentUniformBuffers);
    void createDescriptorSets(vk::DescriptorPool descriptorpool, GBuffer gbuffer);
    void CreateVertexAndIndexBuffer();
    uint32_t alignedSize(uint32_t value, uint32_t alignment);
    void Draw(BufferData RayGenBuffer, BufferData RayHitBuffer, BufferData RayMisBuffer, vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);
    bool UpdateUniformBuffer(vk::DescriptorPool descriptorpool, vk::AccelerationStructureKHR TLAS, std::vector<BufferData>& fragmentUniformBuffers, GBuffer gbuffer);
    void DrawNode(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex, const std::vector<std::shared_ptr<Node>>& nodes);

    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);

    void DispatchGridCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex);
    void DispatchDirectionsCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex);
    void DispatchCalcProbeDataCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex);
    void DispatchSampleGIFromProbeDataCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex);

    void DestroyAtlasImages();
    void DestroySampledGIImage();
    void CreateAtlasImages();
    void CreateSampledGIImage();

    void CleanUp() ;

    vk::DescriptorSetLayout        ProbeDescriptorSetLayout;
    vk::DescriptorSetLayout        GridDescriptorSetLayout;
    vk::DescriptorSetLayout        RaytracingDescriptorSetLayout;
    vk::DescriptorSetLayout        ConstructProbeDataDescriptorSetLayout;
    vk::DescriptorSetLayout        DDGISamplingDescriptorSetLayout;


    std::vector<vk::DescriptorSet>  ProbeDescriptorSets;
    std::vector<vk::DescriptorSet>  GridDescriptorSets;
    std::vector<vk::DescriptorSet>  RaytracingDescriptorSets;
    std::vector<vk::DescriptorSet>  ConstructProbeDataDescriptorSets;
    std::vector<vk::DescriptorSet>  DDGISamplingDescriptorSets;

    //ImageData DDGI_AlbedoImageAtlasImage;
    //ImageData DDGI_NormalImageAtlasImage;
    //ImageData DDGI_MaterialImageAtlasImage;

    ImageData RadianceImageAtlasImage;
    ImageData IradianceImageAtlasImage;
    ImageData Probe_Sampled_GI_Image;

    vk::Extent3D RadianceImageExtent;
    vk::Extent3D IradianceImageExtent;

    std::vector<glm::mat4> ProbeLocations;

    int NumOfProbesX = 6;
    int NumOfProbesY = 6;
    int NumOfProbesZ = 6;

    int RaysPerProbe = 128;

    glm::vec3 ProbeOffset     = glm::vec3(20, 9.48, 9.74);

    glm::vec3 GridLocation     = glm::vec3(-52.63, 1.0, -28.21);
    float RayRotationRadians = 0;;
    float RotationSpeed = 0.0001;


    int ProbeSideLength = (8 * 8);
    int GutterSize = 2;

    int Last_NumOfProbesX;
    int Last_NumOfProbesY;
    int Last_NumOfProbesZ;
    int Last_RaysPerProbe;
    glm::vec3 Last_ProbeOffset;
    glm::vec3 Last_GridLocation;
    std::vector<BufferData> ProbePositionsStorageBuffers;
    std::vector<BufferData> ProbeFibonacciDirectionsStorageBuffers;

	bool DrawDEBUG_Probes = false;
private:

    VulkanContext*   vulkanContext = nullptr;
    BufferManager*   bufferManager = nullptr;
    Camera*          camera        = nullptr;
    vk::CommandPool commandPool = nullptr;

    BufferData vertexBufferData;
    BufferData indexBufferData;

    std::string FilePath;

    const StoredModelData* storedModelData = nullptr;


    SkyBox* skyboxRef = nullptr;
};


static inline void DynamicDiffuse_RTGIDeleter(DynamicDiffuse_RTGI* rayTracing) {

        if (rayTracing) {
            rayTracing->CleanUp();
            delete rayTracing;
        }
   
};