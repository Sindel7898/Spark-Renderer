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
class  Lighting_RTX;


struct CameraConstantBuffer
{
    glm::mat4 ViewMatrix;
    glm::mat4 ProjectionMatrix;
    glm::mat4 ModelMatrix;
    GeneralAtlasInfo generalAtlasInfo;
    int ShowDebugStatus;
};

//Probe States
enum ProbeState {
    ACTIVE,    
    SLEEP,     
    DISSABLED  
};

struct ProbeInformation
{
    glm::vec4 probeLocationsXYZ_ProbeState;
};

class DynamicDiffuse_RTGI
{
public:

    DynamicDiffuse_RTGI(const std::string filepath, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger, SkyBox* skybox, Lighting_RTX* LRTX);
    ~DynamicDiffuse_RTGI();

    void CreateStorageBuffer();
    void createRayTracingDescriptorSetLayout();
    void createRaytracedDescriptorSets(vk::DescriptorPool descriptorpool, vk::AccelerationStructureKHR TLAS, std::vector<BufferData>& fragmentUniformBuffers);
    void createDescriptorSets(vk::DescriptorPool descriptorpool, GBuffer gbuffer);
    void CreateVertexAndIndexBuffer();
    uint32_t alignedSize(uint32_t value, uint32_t alignment);

    void Draw(BufferData RayGenBuffer, BufferData RayHitBuffer, BufferData RayMisBuffer, vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);
    bool UpdateUniformBuffer(vk::DescriptorPool descriptorpool, vk::AccelerationStructureKHR TLAS, std::vector<BufferData>& fragmentUniformBuffers, GBuffer gbuffer, bool ForceUpdate, int lightcount);
    void DrawNode(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex, const std::vector<std::shared_ptr<Node>>& nodes);

    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex);

    // Computes probe world positions and writes them into the probe storage buffer
    void DispatchGridCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex);

    // Generates a new set of randomised Fibonacci sphere ray directions each frame for temporal variation
    void DispatchDirectionsCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex, float deltaTime);

    // Accumulates raw radiance hits into the irradiance/visibility atlas textures
    void DispatchCalcProbeDataCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex);

    // Screen-space pass that samples the irradiance/visibility atlas to produce the final GI image
    void DispatchSampleGIFromProbeDataCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex);

    // Classifies each probe as ACTIVE/SLEEP/DISABLED based on what its rays hit
    void DispatchProbeStatus(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex);

    void DestroyAtlasImages();
    void DestroySampledGIImage();
    void CreateAtlasImages();
    void CreateSampledGIImage();

    void CleanUp();


    vk::DescriptorSetLayout        ProbeDescriptorSetLayout;           // Debug probe rendering
    vk::DescriptorSetLayout        GridDescriptorSetLayout;            // Grid init + direction generation passes
    vk::DescriptorSetLayout        RaytracingDescriptorSetLayout;      // Full RT pass 
    vk::DescriptorSetLayout        ConstructProbeDataDescriptorSetLayout; // Irradiance/visibility accumulation
    vk::DescriptorSetLayout        DDGISamplingDescriptorSetLayout;    // Final GI sampling from probes
    vk::DescriptorSetLayout        ProbeStatusDescriptorSetLayout;     // Probe classification pass

    std::vector<vk::DescriptorSet>  ProbeDescriptorSets;
    std::vector<vk::DescriptorSet>  GridDescriptorSets;
    std::vector<vk::DescriptorSet>  RaytracingDescriptorSets;
    std::vector<vk::DescriptorSet>  ConstructProbeDataDescriptorSets;
    std::vector<vk::DescriptorSet>  DDGISamplingDescriptorSets;
    std::vector<vk::DescriptorSet>  ProbeStatusDescriptorSets;

    // --- Atlas Images ---
    // Radiance: raw ray hit colour/distance, SIZE(RaysPerProbe x TotalProbes)
    // Irradiance/Visibility: oct-encoded
    // Prev_ variants for temporal blending
    ImageData RadianceImageAtlasImage;
    ImageData IradianceImageAtlasImage;
    ImageData VisibilityImageAtlasImage;
    ImageData Prev_IradianceImageAtlasImage;
    ImageData Prev_VisibilityImageAtlasImage;

    // GI sampling compute pass Image
    ImageData Probe_Sampled_GI_Image;

    vk::Extent3D RadianceImageExtent;
    vk::Extent3D IradianceImageExtent;

    std::vector<ProbeInformation> ProbeData;

    // Probe grid dimensions 
    int NumOfProbesX = 10;
    int NumOfProbesY = 10;
    int NumOfProbesZ = 10;

    int RaysPerProbe = 128; 

    glm::vec3 ProbeOffset  = glm::vec3(3.000, 3.000, 3.00);  //spacing between probes
    glm::vec3 GridLocation = glm::vec3(-13.000, -4.000, -15); //origin of the probe grid

    int ProbeSideLength = (6 * 6);//probe sample size
    int GutterSize = 2; // Texel border around each probe 

    int Last_NumOfProbesX;
    int Last_NumOfProbesY;
    int Last_NumOfProbesZ;
    int Last_RaysPerProbe;
    glm::vec3 Last_ProbeOffset;
    glm::vec3 Last_GridLocation;

    BufferData ProbeDataStorageBuffers;                // Per-probe position
    BufferData ProbeFibonacciDirectionsStorageBuffers; //Fibonacci sphere directions

    bool DrawDEBUG_Probes = false; 
    bool ShowDEBUG_Status = false; 

    float infiniteBounceMultiplyer = 0.75f;
    int UseinfiniteBounce = 1;
    int LightCount;
    int  DDGIVertex = 0;

    Lighting_RTX* lighting_RTX = nullptr;

    int RESET_PROBE_STATUS = 0; 

private:

    VulkanContext* vulkanContext = nullptr;
    BufferManager* bufferManager = nullptr;
    Camera* camera = nullptr;
    vk::CommandPool commandPool = nullptr;

    BufferData vertexBufferData;
    BufferData indexBufferData;

    std::string FilePath;

    const StoredModelData* storedModelData = nullptr;

    SkyBox* skyboxRef = nullptr;

    int UpdateGrid;
};