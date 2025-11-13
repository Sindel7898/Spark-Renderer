#pragma once

#include <vulkan/vulkan.hpp>
#include "stb_image.h"
#include <unordered_map>
#include <iostream>
#include <algorithm> 
#include <cmath> 
#include <cstdint>
#include "VertexInputLayouts.h"          
#include "structs.h"


class VulkanContext;


class BufferManager
{
public:

    BufferManager(VulkanContext* VulkcanContext);
    void CreateBuffer(BufferData* BufferData, VkDeviceSize BufferSize, vk::BufferUsageFlags BufferUse, vk::CommandPool commandpool, vk::Queue queue);
    void CreateGPU_Only_Buffer(BufferData* bufferData, VkDeviceSize BufferSize, vk::BufferUsageFlags BufferUse, vk::CommandPool commandpool, vk::Queue queue);
    void CreateDeviceBuffer(BufferData* bufferData, VkDeviceSize BufferSize, vk::BufferUsageFlags BufferUse, vk::CommandPool commandpool, vk::Queue queue);
    void AddBufferLog(BufferData* bufferdata);
    void RemoveBufferLog(BufferData bufferdata);

    void CreateImage(ImageData* imageData, vk::Extent3D imageExtent, vk::Format imageFormat, vk::ImageUsageFlags UsageFlag, bool bMipMaps = false);

    void AddImageLog(ImageData* imageData);
    void RemoveImageLog(ImageData imageData);

    void CreateSharedBuffers(vk::CommandPool& commandPool);

    void DestroySharedBuffers();


    ~BufferManager();


    void CreateCubeMap(ImageData* imageData, std::array<const char*, 6> FilePaths, vk::CommandPool commandpool, vk::Queue Queue);

    void GenerateMipMaps(ImageData* imageData, vk::CommandBuffer* cmdBuffer, float width, float height, vk::Queue graphicsqueue, int layerCount);



    vk::ImageView CreateImageView(ImageData* imageData, vk::Format ImageFormat, vk::ImageAspectFlags ImageAspectBits);
    vk::Sampler CreateImageSampler(vk::SamplerAddressMode addressMode = vk::SamplerAddressMode::eRepeat, bool LinearFiltering = true);

    void TransitionImage(vk::CommandBuffer CommandBuffer, ImageData* imageData, ImageTransitionData& imagetransinotdata);

    vk::CommandBuffer CreateSingleUseCommandBuffer(vk::CommandPool commandpool);
    void SubmitAndDestoyCommandBuffer(vk::CommandPool commandpool, vk::CommandBuffer CommandBuffer, vk::Queue Queue);



    void CreateGPUOptimisedBuffer(BufferData* bufferData, const void* Data, VkDeviceSize BufferSize, vk::BufferUsageFlags BufferUse, vk::CommandPool commandpool, vk::Queue queue);

    void CreateTextureImage(ImageData* Image, const void* pixeldata, vk::DeviceSize imagesize, int texWidth, int textHeight, vk::Format ImageFormat, vk::CommandPool commandpool, vk::Queue Queue);

    ImageData LoadTextureImage(std::string FilePath, vk::Format ImageFormat, vk::CommandPool commandpool, vk::Queue Queue);


    void DestroyBuffer(BufferData& buffer);

    void DestroyImage(const ImageData& buffer);

    void CopyDataToBuffer(const void* data, BufferData Buffer);
    void CopyBufferToAnotherBuffer(vk::CommandPool commandpool, BufferData Buffer1, BufferData Buffer2, vk::Queue Queue);

    void CopyImageToAnotherImage(vk::CommandBuffer commandbuffer, ImageData SrcImage, vk::ImageLayout SrcImageLayout, vk::ImageSubresourceLayers SrcSubresourceLayers, ImageData DstImage, vk::ImageLayout DstImageLayout, vk::ImageSubresourceLayers DstSubresourceLayers, vk::Extent3D ImageExtent, vk::Queue Queue);



    void* MapMemory(const BufferData& buffer);
    void UnmapMemory(const BufferData& buffer);

    VmaAllocator allocator;

    void DeleteAllocation(VmaAllocation allocation);

    std::unordered_map<std::string, IDdata> bufferLog;
    std::unordered_map<std::string, IDdata> imageLog;

    int bufferCounts = 0;

    void CleanUp();

    std::vector<ImageData*>        AllScene_Albedo_Images;
    std::vector<ImageData*>        AllScene_Normal_Images;
    std::vector<ImageData*>        AllScene_MetalicRoughness_Images;
    std::vector<ImageData*>        AllScene_Emissive_Images;

    std::vector<PaddedModelVertex>      AllScene_VertexGeometryData;
    std::vector<uint32_t>               AllScene_IndexGeometryData;
    std::vector<VertexAndIndexOffsets>  AllScene_VertexAndIndexOffsets;

    
    /////////////
    std::vector<BufferData> AllScene_IndexStorageBuffers;
    std::vector<void*>      AllScene_IndexStorageBuffersMappedMem;

    std::vector<BufferData> AllScene_VertexStorageBuffers;
    std::vector<void*>      AllScene_VertexStorageBuffersMappedMem;

    std::vector<BufferData> AllScene_OffsetStorageBuffers;
    std::vector<void*>      AllScene_OffsetStorageBuffersMappedMem;

    std::vector<BufferData> AllScene_TransformationUniformBuffers;
    std::vector<void*>      AllScene_TransformationUniformMappedMem;

    BufferData FullScreenQuadVertexBufferData;
    BufferData FullScreenQuadIndexBufferData;

    /////////////

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

private:

    vk::Device& logicalDevice;
    vk::PhysicalDevice& physicalDevice;
    vk::Instance& vulkanInstance;
    VulkanContext* vulkanContext;


};

static inline void BufferManagerDeleter(BufferManager* bufferManager) {

    if (bufferManager)
    {
        bufferManager->CleanUp();
        delete bufferManager;
    }
}