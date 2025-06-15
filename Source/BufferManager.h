#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include "stb_image.h"
#include <unordered_map>
#include <iostream>

struct IDdata {
    int instance;
    bool IsActive;
};

struct BufferData {

    std::string BufferID;
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
    void CreateBuffer(BufferData* BufferData, VkDeviceSize BufferSize, vk::BufferUsageFlags BufferUse, vk::CommandPool commandpool, vk::Queue queue);
    void AddBufferLog(BufferData* bufferdata);
    void RemoveBufferLog(BufferData bufferdata);

    ~BufferManager();


    ImageData CreateCubeMap(std::array<const char*,6> FilePaths, vk::CommandPool commandpool, vk::Queue Queue);

    ImageData CreateImage(vk::Extent3D imageExtent, vk::Format imageFormat, vk::ImageUsageFlags UsageFlag);
    vk::ImageView CreateImageView(vk::Image ImageToConvert, vk::Format ImageFormat, vk::ImageAspectFlags ImageAspectBits);
    vk::Sampler CreateImageSampler(vk::SamplerAddressMode addressMode = vk::SamplerAddressMode::eRepeat);

    void TransitionImage(vk::CommandBuffer CommandBuffer, vk::Image image, ImageTransitionData& imagetransinotdata);

    vk::CommandBuffer CreateSingleUseCommandBuffer(vk::CommandPool commandpool);
    void SubmitAndDestoyCommandBuffer(vk::CommandPool commandpool, vk::CommandBuffer CommandBuffer, vk::Queue Queue);



    void CreateGPUOptimisedBuffer(BufferData* bufferData,const void* Data, VkDeviceSize BufferSize, vk::BufferUsageFlags BufferUse, vk::CommandPool commandpool, vk::Queue queue);

    ImageData CreateTextureImage(const void* pixeldata, vk::DeviceSize imagesize, int texWidth, int textHeight, vk::Format ImageFormat, vk::CommandPool commandpool, vk::Queue Queue);


    void DestroyBuffer(BufferData& buffer);

    void DestroyImage(const ImageData& buffer);

    void CopyDataToBuffer(const void* data, BufferData Buffer);
    void CopyBufferToAnotherBuffer(vk::CommandPool commandpool, BufferData Buffer1, BufferData Buffer2, vk::Queue Queue);

    void* MapMemory(const BufferData& buffer);
    void UnmapMemory(const BufferData& buffer);
  
    VmaAllocator allocator;

    void DeleteAllocation(VmaAllocation allocation);

    std::unordered_map<std::string, IDdata> bufferLog;
    int bufferCounts = 0;

private:
    vk::Device& logicalDevice;
    vk::PhysicalDevice& physicalDevice;
    vk::Instance& vulkanInstance;

    std::unordered_map<std::string, bool> imageLog;
};

static inline void BufferManagerDeleter(BufferManager* bufferManager) {

    if (bufferManager)
    {
        std::cout << "You have " << bufferManager->bufferLog.size() << " Unfreed Buffers" << std::endl;

        if (bufferManager->bufferLog.size() != 0)
        {
            std::cout << "Unfreed Buffers "<< std::endl;

            for (auto Buffer : bufferManager->bufferLog)
            {
                std::cout << Buffer.first << std::endl;
            }
        }
        vmaDestroyAllocator(bufferManager->allocator);
        delete bufferManager;
    }
}
