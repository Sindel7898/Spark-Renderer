#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include "stb_image.h"
#include <unordered_map>
#include <iostream>
#include <algorithm> 
#include <cmath> 
#include <cstdint>
#include "VertexInputLayouts.h"          

struct IDdata {
    int instance;
    // bool IsActive; // not needed anymore 
};

struct BufferData {

    std::string BufferID;
    vk::Buffer buffer{};
    vk::DeviceSize size{};
    vk::BufferUsageFlags usage{};
    VmaAllocation allocation{};
};

struct ImageData {
    std::string ImageID;
    vk::Image image{};
    vk::ImageView imageView{};
    vk::Sampler imageSampler{};
    VmaAllocation allocation{};
    uint32_t  miplevels = 1;
};


struct ImageTransitionData {
    vk::ImageLayout oldlayout{};
    vk::ImageLayout newlayout{};
    vk::AccessFlags SourceAccessflag = vk::AccessFlagBits::eNone;
    vk::AccessFlags DestinationAccessflag = vk::AccessFlagBits::eNone;
    vk::PipelineStageFlags SourceOnThePipeline = vk::PipelineStageFlagBits::eNone;
    vk::PipelineStageFlags DestinationOnThePipeline = vk::PipelineStageFlagBits::eNone;
    vk::ImageAspectFlags AspectFlag = vk::ImageAspectFlagBits::eColor;
    int BaseMipLevel = 0;
    int LevelCount = 1;
    int LayerCount = 1;

};

struct VertexAndIndexOffsets {

    uint32_t VertexOffset;
    uint32_t IndexOffset;
};

struct PaddedModelVertex {

    glm::vec4 vert_Padding;
    glm::vec4 text_Padding;
    glm::vec4 normal_Padding;
    glm::vec4 tangent_Padding;
};


class VulkanContext;

class BufferManager
{
public:

    BufferManager(VulkanContext* VulkcanContext);
    void CreateBuffer(BufferData* BufferData, VkDeviceSize BufferSize, vk::BufferUsageFlags BufferUse, vk::CommandPool commandpool, vk::Queue queue);
    void CreateDeviceBuffer(BufferData* bufferData, VkDeviceSize BufferSize, vk::BufferUsageFlags BufferUse, vk::CommandPool commandpool, vk::Queue queue);
    void AddBufferLog(BufferData* bufferdata);
    void RemoveBufferLog(BufferData bufferdata);

    void CreateImage(ImageData* imageData, vk::Extent3D imageExtent, vk::Format imageFormat, vk::ImageUsageFlags UsageFlag, bool bMipMaps = false);

    void AddImageLog(ImageData* imageData);
    void RemoveImageLog(ImageData imageData);


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

    std::vector<PaddedModelVertex> AllScene_VertexGeometryData;
    std::vector<uint32_t>          AllScene_IndexGeometryData;
    std::vector<VertexAndIndexOffsets>     AllScene_VertexAndIndexOffsets;

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