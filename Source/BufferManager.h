#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include "stb_image.h"


struct BufferData {
    vk::Buffer buffer;
    vk::DeviceSize size;
    vk::BufferUsageFlags usage;
    VmaAllocation allocation;
};

struct ImageData {
    vk::Image image;
    vk::ImageView imageView;
    vk::Sampler imageSampler;
    VmaAllocation allocation;
};


struct ImageTransitionData {
    vk::ImageLayout oldlayout;
    vk::ImageLayout newlayout;
    vk::AccessFlagBits SourceAccessflag;
    vk::AccessFlagBits DestinationAccessflag;
    vk::PipelineStageFlagBits SourceOnThePipeline;
    vk::PipelineStageFlagBits DestinationOnThePipeline;
    vk::ImageAspectFlagBits AspectFlag = vk::ImageAspectFlagBits::eColor;
};

class BufferManager
{
public:

    BufferManager(vk::Device& LogicalDevice, vk::PhysicalDevice& PhysicalDevice, vk::Instance& VulkanInstance);

    BufferData CreateBuffer(VkDeviceSize BufferSize, vk::BufferUsageFlagBits BufferUse, vk::CommandPool commandpool, vk::Queue queue);


    ImageData CreateTextureImage(const char* FilePath, vk::CommandPool commandpool, vk::Queue Queue);
    vk::Image CreateImage(ImageData imageData, vk::Extent3D imageExtent, vk::Format imageFormat, vk::ImageUsageFlags UsageFlag);
    vk::ImageView  CreateImageView(vk::Image ImageToConvert);
    vk::Sampler CreateImageSampler();

    void TransitionImage(vk::CommandBuffer CommandBuffer, vk::Image image, ImageTransitionData& imagetransinotdata);

    vk::CommandBuffer CreateSingleUseCommandBuffer(vk::CommandPool commandpool);
    void SubmitAndDestoyCommandBuffer(vk::CommandPool commandpool, vk::CommandBuffer CommandBuffer, vk::Queue Queue);




    BufferData CreateGPUOptimisedBuffer(const void* Data, VkDeviceSize BufferSize, vk::BufferUsageFlagBits BufferUse, vk::CommandPool commandpool, vk::Queue queue);

    void DestroyBuffer(const BufferData& buffer);

    void CopyDataToBuffer(const void* data, BufferData Buffer);
    void CopyBufferToAnotherBuffer(vk::CommandPool commandpool, BufferData Buffer1, BufferData Buffer2, vk::Queue Queue);

    void* MapMemory(const BufferData& buffer);
    void UnmapMemory(const BufferData& buffer);

    ~BufferManager();
  
    VmaAllocator allocator;

private:
    vk::Device& logicalDevice;
    vk::PhysicalDevice& physicalDevice;
    vk::Instance& vulkanInstance;
};

