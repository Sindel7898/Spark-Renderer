#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include "stb_image.h"

struct BufferData {

    vk::Buffer buffer{};
    vk::DeviceSize size{};
    vk::BufferUsageFlags usage{};
    VmaAllocation allocation{};
};

struct ImageData {
    vk::Image image{};
    vk::ImageView imageView{};
    vk::Sampler imageSampler{};
    VmaAllocation allocation{};
};


struct ImageTransitionData {
    vk::ImageLayout oldlayout{};
    vk::ImageLayout newlayout{};
    vk::AccessFlags SourceAccessflag = vk::AccessFlagBits::eNone;
    vk::AccessFlags DestinationAccessflag = vk::AccessFlagBits::eNone;
    vk::PipelineStageFlags SourceOnThePipeline = vk::PipelineStageFlagBits::eNone;
    vk::PipelineStageFlags DestinationOnThePipeline = vk::PipelineStageFlagBits::eNone;
    vk::ImageAspectFlags AspectFlag = vk::ImageAspectFlagBits::eColor;
};

class BufferManager
{
public:

    BufferManager(vk::Device& LogicalDevice, vk::PhysicalDevice& PhysicalDevice, vk::Instance& VulkanInstance);
    ~BufferManager();

    BufferData CreateBuffer(VkDeviceSize BufferSize, vk::BufferUsageFlags BufferUse, vk::CommandPool commandpool, vk::Queue queue);


    ImageData CreateCubeMap(std::array<const char*,6> FilePaths, vk::CommandPool commandpool, vk::Queue Queue);

    ImageData CreateImage(vk::Extent3D imageExtent, vk::Format imageFormat, vk::ImageUsageFlags UsageFlag);
    vk::ImageView CreateImageView(vk::Image ImageToConvert, vk::Format ImageFormat, vk::ImageAspectFlags ImageAspectBits);
    vk::Sampler CreateImageSampler(vk::SamplerAddressMode addressMode = vk::SamplerAddressMode::eRepeat);

    void TransitionImage(vk::CommandBuffer CommandBuffer, vk::Image image, ImageTransitionData& imagetransinotdata);

    vk::CommandBuffer CreateSingleUseCommandBuffer(vk::CommandPool commandpool);
    void SubmitAndDestoyCommandBuffer(vk::CommandPool commandpool, vk::CommandBuffer CommandBuffer, vk::Queue Queue);



    BufferData CreateGPUOptimisedBuffer(const void* Data, VkDeviceSize BufferSize, vk::BufferUsageFlags BufferUse, vk::CommandPool commandpool, vk::Queue queue);

    ImageData CreateTextureImage(const void* pixeldata, vk::DeviceSize imagesize, int texWidth, int textHeight, vk::Format ImageFormat, vk::CommandPool commandpool, vk::Queue Queue);


    void DestroyBuffer(const BufferData& buffer);

    void DestroyImage(const ImageData& buffer);

    void CopyDataToBuffer(const void* data, BufferData Buffer);
    void CopyBufferToAnotherBuffer(vk::CommandPool commandpool, BufferData Buffer1, BufferData Buffer2, vk::Queue Queue);

    void* MapMemory(const BufferData& buffer);
    void UnmapMemory(const BufferData& buffer);
  
    VmaAllocator allocator;

    void DeleteAllocation(VmaAllocation allocation);

private:
    vk::Device& logicalDevice;
    vk::PhysicalDevice& physicalDevice;
    vk::Instance& vulkanInstance;
};

static inline void BufferManagerDeleter(BufferManager* bufferManager) {

    if (bufferManager)
    {
        vmaDestroyAllocator(bufferManager->allocator);
        delete bufferManager;
    }
}
