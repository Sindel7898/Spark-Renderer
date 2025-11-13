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

struct RTpcInfo
{
    SampleGridInfo  sampleGridInfo;
    glm::vec4 UseInfiniteBounce_infinite_bounces_multiplier_Padding;

};


struct CameraConstantBuffer
{
    glm::mat4 ViewMatrix;
    glm::mat4 ProjectionMatrix;
    glm::mat4 ModelMatrix;
    GeneralAtlasInfo generalAtlasInfo;
    int ShowDebugStatus;
};

enum ProbeState {

    ACTIVE,    // this is for when a probe is activly collecting new scene data
    SLEEP,     // this is for when a probe is always hitting the skybox
    DISSABLED //  this is for when a probe is inside an object and is not contributing meaningfully.
   //A probe that is disabled can be moved so it is active
};

struct ProbeInformation
{
    glm::vec4 probeLocations;
    ProbeState probeState;
    glm::vec3 Padding;
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
    void DispatchDirectionsCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex, float deltaTime);
    void DispatchCalcProbeDataCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex);
    void DispatchSampleGIFromProbeDataCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex);

    void DispatchProbeStatus(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex);

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
    vk::DescriptorSetLayout        ProbeStatusDescriptorSetLayout;


    std::vector<vk::DescriptorSet>  ProbeDescriptorSets;
    std::vector<vk::DescriptorSet>  GridDescriptorSets;
    std::vector<vk::DescriptorSet>  RaytracingDescriptorSets;
    std::vector<vk::DescriptorSet>  ConstructProbeDataDescriptorSets;
    std::vector<vk::DescriptorSet>  DDGISamplingDescriptorSets;
    std::vector<vk::DescriptorSet>  ProbeStatusDescriptorSets;

    ImageData RadianceImageAtlasImage;
    ImageData IradianceImageAtlasImage;
    ImageData VisibilityImageAtlasImage;
    ImageData Prev_IradianceImageAtlasImage;
    ImageData Prev_VisibilityImageAtlasImage;
    ImageData Probe_Sampled_GI_Image;

    vk::Extent3D RadianceImageExtent;
    vk::Extent3D IradianceImageExtent;

    std::vector<ProbeInformation> ProbeData;

    int NumOfProbesX = 9;
    int NumOfProbesY = 9;
    int NumOfProbesZ = 9;

    int RaysPerProbe = 128;

    glm::vec3 ProbeOffset     = glm::vec3(11.42, 7.82, 10.35);

    glm::vec3 GridLocation     = glm::vec3(-42.85, -9.26, -45.61);
    float RayRotationRadians = 0;;
    float RotationSpeed = 1;


    int ProbeSideLength = (6 * 6);
    int GutterSize = 2;

    int Last_NumOfProbesX;
    int Last_NumOfProbesY;
    int Last_NumOfProbesZ;
    int Last_RaysPerProbe;
    glm::vec3 Last_ProbeOffset;
    glm::vec3 Last_GridLocation;
    std::vector<BufferData> ProbeDataStorageBuffers;
    std::vector<BufferData> ProbeFibonacciDirectionsStorageBuffers;

	bool DrawDEBUG_Probes = false;
    bool ShowDEBUG_Status = false;

    float infiniteBounceMultiplyer = 0.75f;
    int UseinfiniteBounce = 1;
    int SampleCount = 2;
    int FrameCount = 0;
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

    int UpdateGrid;
};


static inline void DynamicDiffuse_RTGIDeleter(DynamicDiffuse_RTGI* rayTracing) {

        if (rayTracing) {
            rayTracing->CleanUp();
            delete rayTracing;
        }
   
};