#include "BufferManager.h"
#define VMA_IMPLEMENTATION
//#define VMA_DEBUG_LOG(format, ...) printf(format, __VA_ARGS__)
//#define VMA_DEBUG_MARGIN 16
//#define VMA_DEBUG_DETECT_CORRUPTION 1
//#define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#define STB_IMAGE_IMPLEMENTATION

VmaAllocator allocator;


BufferManager::BufferManager(vk::Device LogicalDevice) : logicalDevice(LogicalDevice) {


}


void BufferManager::CreateBuffer(BufferData bufferData, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags)
{
	
	VkBufferCreateInfo BufferCreateInfo = {};
	                     BufferCreateInfo.size = (VkDeviceSize)bufferData.size;
	                     BufferCreateInfo.usage = (VkBufferUsageFlags)bufferData.usage;
	                     BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;

	 VmaAllocationCreateInfo AllocCreateInfo = {};
	 AllocCreateInfo.usage = memoryUsage;
	 AllocCreateInfo.flags = flags;

	
	 VkBuffer vkBuffer;
	 VmaAllocation vkAllocation;

	if (vmaCreateBuffer(allocator, &BufferCreateInfo, &AllocCreateInfo, &vkBuffer, &vkAllocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	bufferData.buffer = vk::Buffer(vkBuffer);
	bufferData.allocation = vkAllocation;
}

 vk::Image BufferManager::CreateTextureImage(const char* FilePath, vk::CommandPool commandpool, vk::Queue Queue)
{
	int texWidth, textHeight, textchannels;

	stbi_uc* pixels = stbi_load(FilePath, &texWidth, &textHeight, &textchannels, STBI_rgb_alpha);
	vk::DeviceSize imagesize = texWidth * textHeight * 4;

	VkBufferCreateInfo cStagingBufferCreateInfo = {};
	cStagingBufferCreateInfo.size = imagesize;
	cStagingBufferCreateInfo.usage = (VkBufferUsageFlags)vk::BufferUsageFlagBits::eTransferSrc;
	cStagingBufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo AllocCreateInfo = {};
	AllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	AllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VkBuffer cStagingBuffer;
	VmaAllocation StagingBufferAllocation;

	if (vmaCreateBuffer(allocator, &cStagingBufferCreateInfo, &AllocCreateInfo, &cStagingBuffer, &StagingBufferAllocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	BufferData Buffer;
	Buffer.buffer = vk::Buffer(cStagingBuffer);
	Buffer.size = imagesize;
	Buffer.allocation = StagingBufferAllocation;
	Buffer.usage = vk::BufferUsageFlagBits::eTransferSrc;

	CopyDataToBuffer(pixels, Buffer);
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	vk::Extent3D imageExtent = { static_cast<uint32_t>(texWidth),static_cast<uint32_t>(textHeight),1 };

	vk::ImageCreateInfo imagecreateinfo;
	imagecreateinfo.imageType = vk::ImageType::e2D;
	imagecreateinfo.extent = imageExtent;
	imagecreateinfo.mipLevels = 1;
	imagecreateinfo.arrayLayers = 1;
	imagecreateinfo.format = vk::Format::eR8G8B8A8Srgb;
	imagecreateinfo.tiling = vk::ImageTiling::eOptimal;
	imagecreateinfo.initialLayout = vk::ImageLayout::eUndefined;
	imagecreateinfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
	imagecreateinfo.samples = vk::SampleCountFlagBits::e1;

	VkImageCreateInfo cimagecreateinfo = static_cast<VkImageCreateInfo> (imagecreateinfo);
	VkImage cTextureImage;

	VmaAllocationCreateInfo imageAllocInfo = {};
	imageAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	imageAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

	VmaAllocation imageAllocation;

	vmaCreateImage(allocator, &cimagecreateinfo, &imageAllocInfo, &cTextureImage, &imageAllocation, nullptr);

	vk::Image TextureImage = static_cast<vk::Image> (cTextureImage);

	vk::CommandBuffer CommandBuffer = CreateSingleUseCommandBuffer(commandpool);

	ImageTransitionData DataToTransitionInfo;
	DataToTransitionInfo.oldlayout = vk::ImageLayout::eUndefined;
	DataToTransitionInfo.newlayout = vk::ImageLayout::eTransferDstOptimal;
	DataToTransitionInfo.SourceAccessflag = vk::AccessFlagBits::eNone;
	DataToTransitionInfo.DestinationAccessflag = vk::AccessFlagBits::eTransferWrite;
	DataToTransitionInfo.AspectFlag = vk::ImageAspectFlagBits::eColor;
	DataToTransitionInfo.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	DataToTransitionInfo.SourceOnThePipeline = vk::PipelineStageFlagBits::eTransfer;

	TransitionImage(CommandBuffer,TextureImage,DataToTransitionInfo);

	vk::BufferImageCopy copyRegion = {};
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;
	copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	copyRegion.imageSubresource.mipLevel = 0;
	copyRegion.imageSubresource.baseArrayLayer = 0;
	copyRegion.imageSubresource.layerCount = 1;
	copyRegion.imageOffset = vk::Offset3D{ 0, 0, 0 };
	copyRegion.imageExtent = imageExtent;

	CommandBuffer.copyBufferToImage(Buffer.buffer, TextureImage, vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);

	ImageTransitionData TransitionImageToShaderData;
	TransitionImageToShaderData.oldlayout = vk::ImageLayout::eTransferDstOptimal;
	TransitionImageToShaderData.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	TransitionImageToShaderData.SourceAccessflag = vk::AccessFlagBits::eTransferWrite;
	TransitionImageToShaderData.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
	TransitionImageToShaderData.AspectFlag = vk::ImageAspectFlagBits::eColor;
	TransitionImageToShaderData.SourceOnThePipeline = vk::PipelineStageFlagBits::eTransfer;
	TransitionImageToShaderData.SourceOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

	TransitionImage(CommandBuffer, TextureImage, TransitionImageToShaderData);

	SubmitAndDestoyCommandBuffer(commandpool, CommandBuffer, Queue);

	DestroyBuffer(Buffer);

	ImageData imageData;
	imageData.image = TextureImage;
	imageData.allocation = imageAllocation;

	return imageData;
}



void BufferManager::TransitionImage(vk::CommandBuffer CommandBuffer, vk::Image image, ImageTransitionData imagetransinotdata) {
	

	vk::ImageMemoryBarrier acquireBarrier{};
	acquireBarrier.oldLayout = imagetransinotdata.oldlayout;
	acquireBarrier.newLayout = imagetransinotdata.newlayout;
	acquireBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	acquireBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	acquireBarrier.image = image;
	acquireBarrier.subresourceRange.aspectMask = imagetransinotdata.AspectFlag;
	acquireBarrier.subresourceRange.baseMipLevel = 0;
	acquireBarrier.subresourceRange.levelCount = 1;
	acquireBarrier.subresourceRange.baseArrayLayer = 0;
	acquireBarrier.subresourceRange.layerCount = 1;
	acquireBarrier.srcAccessMask = imagetransinotdata.SourceAccessflag;
	acquireBarrier.dstAccessMask = imagetransinotdata.DestinationAccessflag;

	CommandBuffer.pipelineBarrier(
		imagetransinotdata.SourceOnThePipeline,
		imagetransinotdata.DestinationOnThePipeline,
		vk::DependencyFlags(),
		0, nullptr,
		0, nullptr,
		1, &acquireBarrier
	);

}

vk::CommandBuffer BufferManager::CreateSingleUseCommandBuffer(vk::CommandPool commandpool) {

	vk::CommandBuffer CommandBuffer;

	vk::CommandBufferAllocateInfo CommandBufferAllocateInfo;
	CommandBufferAllocateInfo.commandPool = commandpool;
	CommandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
	CommandBufferAllocateInfo.commandBufferCount = 1;

	CommandBuffer = logicalDevice.allocateCommandBuffers(CommandBufferAllocateInfo)[0];

	vk::CommandBufferBeginInfo CommandBufferBeginInfo;
	CommandBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	CommandBuffer.begin(CommandBufferBeginInfo);

	return CommandBuffer;
}

vk::CommandBuffer BufferManager::SubmitAndDestoyCommandBuffer(vk::CommandPool commandpool,vk::CommandBuffer CommandBuffer, vk::Queue Queue) {

	CommandBuffer.end();
	vk::SubmitInfo submitInfo = {};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &CommandBuffer;

	Queue.submit(1, &submitInfo, nullptr);
	Queue.waitIdle();

	logicalDevice.freeCommandBuffers(commandpool, 1, &CommandBuffer);

	return CommandBuffer;
}

void BufferManager::CopyDataToBuffer(const void* data,  BufferData Buffer) {
	void* mappedData = MapMemory(Buffer);
	memcpy(mappedData, data, static_cast<size_t>(Buffer.size));
	UnmapMemory(Buffer);
}

void BufferManager::CopyBuffer(vk::CommandPool commandpool , BufferData Buffer1, BufferData Buffer2, vk::Queue Queue) {


	vk::CommandBufferAllocateInfo allocateinfo{};
	allocateinfo.commandPool = commandpool;
	allocateinfo.commandBufferCount = 1;
	allocateinfo.level = vk::CommandBufferLevel::ePrimary;


	vk::CommandBuffer commandBuffer = logicalDevice.allocateCommandBuffers(allocateinfo)[0];

	vk::CommandBufferBeginInfo CBBegininfo{};
	CBBegininfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	commandBuffer.begin(CBBegininfo);

	vk::BufferCopy copyregion{};
	copyregion.srcOffset = 0;
	copyregion.dstOffset = 0;
	copyregion.size = Buffer1.size;

	commandBuffer.copyBuffer(Buffer1.buffer, Buffer2.buffer, copyregion);

	commandBuffer.end();

	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	Queue.submit(1, &submitInfo, nullptr);
	Queue.waitIdle(); // Wait for the task to be done 

	logicalDevice.freeCommandBuffers(commandpool, 1, &commandBuffer);
}

void* BufferManager::MapMemory(const BufferData& buffer) {
	void* data;
	vmaMapMemory(allocator, buffer.allocation, &data);
	return data;
}

void BufferManager::UnmapMemory(const BufferData& buffer) {
	vmaUnmapMemory(allocator, buffer.allocation);
}

void BufferManager::DestroyBuffer(const BufferData& buffer) {
	vmaDestroyBuffer(allocator, static_cast<VkBuffer>(buffer.buffer), buffer.allocation);
}

BufferManager::~BufferManager()
{
}
