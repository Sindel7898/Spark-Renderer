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


    BufferManager(vk::Device LogicalDevice);

    void CreateBuffer(BufferData bufferData, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags);

    ImageData CreateTextureImage(const char* FilePath, vk::CommandPool commandpool, vk::Queue Queue);
    void TransitionImage(vk::CommandBuffer CommandBuffer, vk::Image image, ImageTransitionData imagetransinotdata);

    vk::CommandBuffer CreateSingleUseCommandBuffer(vk::CommandPool commandpool);

    vk::CommandBuffer SubmitAndDestoyCommandBuffer(vk::CommandPool commandpool, vk::CommandBuffer CommandBuffer, vk::Queue Queue);



    void DestroyBuffer(const BufferData& buffer);

    void CopyDataToBuffer(const void* data, BufferData Buffer);

    void CopyBuffer(vk::CommandPool commandpool, BufferData Buffer1, BufferData Buffer2, vk::Queue Queue);

    void* MapMemory(const BufferData& buffer);
    void UnmapMemory(const BufferData& buffer);

    ~BufferManager();
  
private:
   
    vk::Device logicalDevice;
};

