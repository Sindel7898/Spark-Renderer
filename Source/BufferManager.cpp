#include "BufferManager.h"
#define VMA_IMPLEMENTATION
//#define VMA_DEBUG_LOG(format, ...) printf(format, __VA_ARGS__)
//#define VMA_DEBUG_MARGIN 16
//#define VMA_DEBUG_DETECT_CORRUPTION 1
//#define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#define STB_IMAGE_IMPLEMENTATION

BufferManager::BufferManager(vk::Device& LogicalDevice, vk::PhysicalDevice& PhysicalDevice,vk::Instance& VulkanInstance) : logicalDevice(LogicalDevice), physicalDevice(PhysicalDevice), vulkanInstance(VulkanInstance){

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0; // Use the appropriate Vulkan API version
	allocatorInfo.physicalDevice = physicalDevice;
	allocatorInfo.device = logicalDevice;
	allocatorInfo.instance = vulkanInstance;

	if (vmaCreateAllocator(&allocatorInfo, &allocator) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create VMA allocator!");
	}

}


void BufferManager::CreateGPUOptimisedBuffer(void* Data, VkDeviceSize BufferSize, VkBufferUsageFlagBits BufferUse, vk::CommandPool commandpool, vk::Queue queue)
{
	VmaAllocationCreateInfo AllocationInfo = {};
	AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
	AllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		                   VMA_ALLOCATION_CREATE_MAPPED_BIT;


	VkBufferCreateInfo BufferCreateInfo = {};
	                     BufferCreateInfo.size = BufferSize;
	                     BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	                     BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;

	 VkBuffer StagineBuffer;
	 VmaAllocation StagineBufferAllocation;

	if (vmaCreateBuffer(allocator, &BufferCreateInfo, &AllocationInfo, &StagineBuffer, &StagineBufferAllocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	BufferData StagingBufferData;
	StagingBufferData.buffer = StagineBuffer;
	StagingBufferData.size = BufferCreateInfo.size;
	StagingBufferData.usage = vk::BufferUsageFlagBits::eTransferSrc;
	StagingBufferData.allocation = StagineBufferAllocation;

	CopyDataToBuffer(Data, StagingBufferData);

	VkBufferCreateInfo VertexBufferInfo;
	VertexBufferInfo.size = BufferSize;
	VertexBufferInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT | BufferUse;
	VertexBufferInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo vertexAllocInfo = {};
	vertexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	vertexAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

	VkBuffer cVertexbuffer;
	VmaAllocation VertexBufferAllocation;

	if (vmaCreateBuffer(allocator, &VertexBufferInfo, &vertexAllocInfo, &cVertexbuffer, &VertexBufferAllocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	BufferData FinalBufferData;

	FinalBufferData.buffer = cVertexbuffer;
	FinalBufferData.size = VertexBufferInfo.size;
	FinalBufferData.usage = vk::BufferUsageFlagBits::eTransferDst;
	FinalBufferData.allocation = VertexBufferAllocation;

	CopyBufferToAnotherBuffer(commandpool, StagingBufferData, FinalBufferData, queue);

	DestroyBuffer(StagingBufferData);
}

ImageData BufferManager::CreateTextureImage(const char* FilePath, vk::CommandPool commandpool, vk::Queue Queue)
{
	ImageData imageData;

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

	stbi_image_free(pixels);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	vk::Extent3D imageExtent = { static_cast<uint32_t>(texWidth),static_cast<uint32_t>(textHeight),1 };

	vk::Image TextureImage = CreateImage(imageData,imageExtent, vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

	imageData.image = TextureImage;
	imageData.imageView = CreateImageView(TextureImage);
	imageData.imageSampler = CreateImageSampler();

	return imageData;
}

vk::Image BufferManager::CreateImage(ImageData imageData, vk::Extent3D imageExtent, vk::Format imageFormat, vk::ImageUsageFlags UsageFlag) {

	vk::ImageCreateInfo imagecreateinfo;
	imagecreateinfo.imageType = vk::ImageType::e2D;
	imagecreateinfo.extent = imageExtent;
	imagecreateinfo.mipLevels = 1;
	imagecreateinfo.arrayLayers = 1;
	imagecreateinfo.format = imageFormat;
	imagecreateinfo.tiling = vk::ImageTiling::eOptimal;
	imagecreateinfo.initialLayout = vk::ImageLayout::eUndefined;
	imagecreateinfo.usage = UsageFlag;
	imagecreateinfo.samples = vk::SampleCountFlagBits::e1; 
	imagecreateinfo.sharingMode = vk::SharingMode::eExclusive; 

	VkImageCreateInfo cimagecreateinfo = static_cast<VkImageCreateInfo> (imagecreateinfo);

	VmaAllocationCreateInfo imageAllocInfo = {};
	imageAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

	VkImage cTextureImage;

	if (vmaCreateImage(allocator, &cimagecreateinfo, &imageAllocInfo, &cTextureImage, &imageData.allocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create depth image!");
	}

	return  static_cast<vk::Image>(cTextureImage);
}

vk::ImageView BufferManager::CreateImageView(vk::Image ImageToConvert) {

	vk::ImageViewCreateInfo imageviewinfo{};
	imageviewinfo.image = ImageToConvert;
	imageviewinfo.viewType = vk::ImageViewType::e2D;
	imageviewinfo.format = vk::Format::eR8G8B8A8Srgb;
	imageviewinfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	imageviewinfo.subresourceRange.baseMipLevel = 0;
	imageviewinfo.subresourceRange.levelCount = 1;
	imageviewinfo.subresourceRange.baseArrayLayer = 0;
	imageviewinfo.subresourceRange.layerCount = 1;

   return logicalDevice.createImageView(imageviewinfo);
}

vk::Sampler BufferManager::CreateImageSampler() {

	vk::SamplerCreateInfo SamplerInfo{};
	SamplerInfo.magFilter = vk::Filter::eLinear;
	SamplerInfo.minFilter = vk::Filter::eLinear;
	SamplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	SamplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	SamplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	SamplerInfo.anisotropyEnable = vk::True;
	SamplerInfo.maxAnisotropy = physicalDevice.getProperties().limits.maxSamplerAnisotropy;
	SamplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	SamplerInfo.unnormalizedCoordinates = vk::False;
	SamplerInfo.compareEnable = vk::False;
	SamplerInfo.compareOp = vk::CompareOp::eAlways;
	SamplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	SamplerInfo.mipLodBias = 0.0f;
	SamplerInfo.minLod = 0.0f;
	SamplerInfo.maxLod = 0.0f;
	
	return logicalDevice.createSampler(SamplerInfo);
}

void BufferManager::TransitionImage(vk::CommandBuffer CommandBuffer, vk::Image image, ImageTransitionData& imagetransinotdata) {
	

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

void BufferManager::SubmitAndDestoyCommandBuffer(vk::CommandPool commandpool,vk::CommandBuffer CommandBuffer, vk::Queue Queue) {

	CommandBuffer.end();
	vk::SubmitInfo submitInfo = {};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &CommandBuffer;

	Queue.submit(1, &submitInfo, nullptr);
	Queue.waitIdle();

	logicalDevice.freeCommandBuffers(commandpool, 1, &CommandBuffer);
}

void BufferManager::CopyDataToBuffer(const void* data,  BufferData Buffer) {
	void* mappedData = MapMemory(Buffer);
	memcpy(mappedData, data, static_cast<size_t>(Buffer.size));
	UnmapMemory(Buffer);
}

void BufferManager::CopyBufferToAnotherBuffer(vk::CommandPool commandpool , BufferData Buffer1, BufferData Buffer2, vk::Queue Queue) {

	vk::CommandBuffer commandBuffer = CreateSingleUseCommandBuffer(commandpool);

	vk::BufferCopy copyregion{};
	copyregion.srcOffset = 0;
	copyregion.dstOffset = 0;
	copyregion.size = Buffer1.size;

	commandBuffer.copyBuffer(Buffer1.buffer, Buffer2.buffer, copyregion);

	SubmitAndDestoyCommandBuffer(commandpool, commandBuffer, Queue);
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
	vmaDestroyAllocator(allocator);

}
