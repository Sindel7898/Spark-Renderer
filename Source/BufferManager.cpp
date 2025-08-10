#include "BufferManager.h"
#define VMA_IMPLEMENTATION
#define VMA_DEBUG_LOG(format, ...) printf(format, __VA_ARGS__)
#define VMA_DEBUG_MARGIN 16
#define VMA_DEBUG_DETECT_CORRUPTION 1
#define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1

BufferManager::BufferManager(vk::Device& LogicalDevice, vk::PhysicalDevice& PhysicalDevice,vk::Instance& VulkanInstance) : logicalDevice(LogicalDevice), physicalDevice(PhysicalDevice), vulkanInstance(VulkanInstance){

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0; // Use the appropriate Vulkan API version
	allocatorInfo.physicalDevice = physicalDevice;
	allocatorInfo.device = logicalDevice;
	allocatorInfo.instance = vulkanInstance;
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

	if (vmaCreateAllocator(&allocatorInfo, &allocator) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create VMA allocator!");
	}

}

void BufferManager::CreateBuffer(BufferData* bufferData,VkDeviceSize BufferSize, vk::BufferUsageFlags BufferUse, vk::CommandPool commandpool, vk::Queue queue)
{
	VmaAllocationCreateInfo AllocationInfo = {};
	AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
	AllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		VMA_ALLOCATION_CREATE_MAPPED_BIT;


	vk::BufferCreateInfo StagingBufferCreateInfo = {};
	StagingBufferCreateInfo.size = BufferSize;
	StagingBufferCreateInfo.usage = BufferUse;
	StagingBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

	VkBuffer Buffer;
	VmaAllocation allocation;
	VkBufferCreateInfo cBufferCreateInfo = StagingBufferCreateInfo;

	if (vmaCreateBuffer(allocator, &cBufferCreateInfo, &AllocationInfo, &Buffer, &allocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	bufferData->buffer = vk::Buffer(Buffer);
	bufferData->size = cBufferCreateInfo.size;
	bufferData->usage = BufferUse;
	bufferData->allocation = allocation;

	AddBufferLog(bufferData);
}

void BufferManager::CreateDeviceBuffer(BufferData* bufferData, VkDeviceSize BufferSize, vk::BufferUsageFlags BufferUse, vk::CommandPool commandpool, vk::Queue queue)
{

	VmaAllocationCreateInfo AllocationInfo = {};
	AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	AllocationInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

	vk::BufferCreateInfo StagingBufferCreateInfo = {};
	StagingBufferCreateInfo.size = BufferSize;
	StagingBufferCreateInfo.usage = BufferUse;
	StagingBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

	VkBuffer Buffer;
	VmaAllocation allocation;
	VkBufferCreateInfo cBufferCreateInfo = StagingBufferCreateInfo;

	if (vmaCreateBuffer(allocator, &cBufferCreateInfo, &AllocationInfo, &Buffer, &allocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	bufferData->buffer = vk::Buffer(Buffer);
	bufferData->size = cBufferCreateInfo.size;
	bufferData->usage = BufferUse;
	bufferData->allocation = allocation;

	AddBufferLog(bufferData);
}


void BufferManager::AddBufferLog(BufferData* bufferData)
{
	if (bufferData->BufferID.empty())
	{
	  throw	std::runtime_error("A buffer you are trying to create does not have an ID");
	}

	std::string ConstructedID = bufferData->BufferID;

	auto FoundLog = bufferLog.find(ConstructedID);

	if (FoundLog == bufferLog.end()) {
		IDdata data;
		data.instance = 0;

		bufferLog.emplace(bufferData->BufferID,data);
	}
	else
	{
		IDdata data;
		data.instance = FoundLog->second.instance + 1;

		auto newLogID = FoundLog->first + "instance" + std::to_string(data.instance);
		bufferData->BufferID = newLogID;
		bufferLog.emplace(newLogID, data);
	}
}

void BufferManager::RemoveBufferLog(BufferData bufferData)
{
	if (bufferData.BufferID.empty())
	{
		throw std::runtime_error("A buffer you are trying to destroy does not have an ID");
	}
	auto FoundLog = bufferLog.find(bufferData.BufferID);

	if (FoundLog != bufferLog.end()) {
		bufferLog.erase(FoundLog);
	}
}

void BufferManager::CreateGPUOptimisedBuffer(BufferData* bufferData,const void* Data, VkDeviceSize BufferSize, vk::BufferUsageFlags BufferUse, vk::CommandPool commandpool, vk::Queue queue)
{
	VmaAllocationCreateInfo AllocationInfo = {};
	AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
	AllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		                   VMA_ALLOCATION_CREATE_MAPPED_BIT;


	vk::BufferCreateInfo StagingBufferCreateInfo = {};
						 StagingBufferCreateInfo.size = BufferSize;
						 StagingBufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	                     StagingBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

	 VkBuffer StagineBuffer;
	 VmaAllocation StagineBufferAllocation;
	 VkBufferCreateInfo cStagingBufferCreateInfo = StagingBufferCreateInfo;

	if (vmaCreateBuffer(allocator, &cStagingBufferCreateInfo, &AllocationInfo, &StagineBuffer, &StagineBufferAllocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}


	BufferData StagingBufferData;
	StagingBufferData.buffer = vk::Buffer(StagineBuffer);
	StagingBufferData.size = cStagingBufferCreateInfo.size;
	StagingBufferData.usage = vk::BufferUsageFlagBits::eTransferSrc;
	StagingBufferData.allocation = StagineBufferAllocation;
	StagingBufferData.BufferID = "GPU Staging Buffer";
	AddBufferLog(&StagingBufferData);
	CopyDataToBuffer(Data, StagingBufferData);

	vk::BufferCreateInfo VertexBufferInfo;
	VertexBufferInfo.size = BufferSize;
	VertexBufferInfo.usage = vk::BufferUsageFlagBits::eTransferDst | BufferUse;
	VertexBufferInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo vertexAllocInfo = {};
	vertexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	vertexAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;


	VkBuffer cVertexbuffer;
	VmaAllocation VertexBufferAllocation;
	VkBufferCreateInfo cVertexBufferInfo = VertexBufferInfo;

	if (vmaCreateBuffer(allocator, &cVertexBufferInfo, &vertexAllocInfo, &cVertexbuffer, &VertexBufferAllocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	BufferData FinalBufferData;
	FinalBufferData.buffer = vk::Buffer(cVertexbuffer);
	FinalBufferData.size = VertexBufferInfo.size;
	FinalBufferData.usage = vk::BufferUsageFlagBits::eTransferDst | BufferUse;
	FinalBufferData.allocation = VertexBufferAllocation;

	CopyBufferToAnotherBuffer(commandpool, StagingBufferData, FinalBufferData, queue);
	
	DestroyBuffer(StagingBufferData);

	bufferData->buffer = FinalBufferData.buffer;
	bufferData->size = FinalBufferData.size;
	bufferData->usage = FinalBufferData.usage;
	bufferData->allocation = FinalBufferData.allocation;
	bufferData->BufferID = "GPU Optimised Final Buffer";

	AddBufferLog(bufferData);

}

ImageData BufferManager::CreateTextureImage(const void* pixeldata, vk::DeviceSize imagesize, int texWidth, int textHeight,vk::Format ImageFormat, vk::CommandPool commandpool, vk::Queue Queue)
{

	vk::BufferCreateInfo StagingBufferCreateInfo = {};
	StagingBufferCreateInfo.size = imagesize;
	StagingBufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	StagingBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo AllocCreateInfo = {};
	AllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	AllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VkBuffer cStagingBuffer;
	VmaAllocation StagingBufferAllocation;

	VkBufferCreateInfo cStagingBufferCreateInfo = static_cast<VkBufferCreateInfo>(StagingBufferCreateInfo);

	if (vmaCreateBuffer(allocator, &cStagingBufferCreateInfo, &AllocCreateInfo, &cStagingBuffer, &StagingBufferAllocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	BufferData StagineBuffer;
	StagineBuffer.buffer = vk::Buffer(cStagingBuffer);
	StagineBuffer.size = imagesize;
	StagineBuffer.allocation = StagingBufferAllocation;
	StagineBuffer.usage = vk::BufferUsageFlagBits::eTransferSrc;
	StagineBuffer.BufferID = "Texture Staging Buffer";
	AddBufferLog(&StagineBuffer);

	CopyDataToBuffer(pixeldata, StagineBuffer);

	//stbi_image_free(pixeldata);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	vk::Extent3D imageExtent = { static_cast<uint32_t>(texWidth),static_cast<uint32_t>(textHeight),1 };

	 ImageData TextureImageData;
	 TextureImageData.ImageID = "StagineBuffer texture";

	 CreateImage(&TextureImageData,imageExtent, ImageFormat, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	vk::CommandBuffer CommandBuffer = CreateSingleUseCommandBuffer(commandpool);

	ImageTransitionData DataToTransitionInfo;
	DataToTransitionInfo.oldlayout = vk::ImageLayout::eUndefined;
	DataToTransitionInfo.newlayout = vk::ImageLayout::eTransferDstOptimal;
	DataToTransitionInfo.SourceAccessflag = vk::AccessFlagBits::eNone;
	DataToTransitionInfo.DestinationAccessflag = vk::AccessFlagBits::eTransferWrite;
	DataToTransitionInfo.AspectFlag = vk::ImageAspectFlagBits::eColor;
	DataToTransitionInfo.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	DataToTransitionInfo.DestinationOnThePipeline = vk::PipelineStageFlagBits::eTransfer;

	TransitionImage(CommandBuffer, &TextureImageData,DataToTransitionInfo);

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

	CommandBuffer.copyBufferToImage(StagineBuffer.buffer, TextureImageData.image, vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);

	ImageTransitionData TransitionImageToShaderData;
	TransitionImageToShaderData.oldlayout = vk::ImageLayout::eTransferDstOptimal;
	TransitionImageToShaderData.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	TransitionImageToShaderData.SourceAccessflag = vk::AccessFlagBits::eTransferWrite;
	TransitionImageToShaderData.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
	TransitionImageToShaderData.AspectFlag = vk::ImageAspectFlagBits::eColor;
	TransitionImageToShaderData.SourceOnThePipeline = vk::PipelineStageFlagBits::eTransfer;
	TransitionImageToShaderData.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

	TransitionImage(CommandBuffer, &TextureImageData, TransitionImageToShaderData);

	SubmitAndDestoyCommandBuffer(commandpool, CommandBuffer, Queue);

	DestroyBuffer(StagineBuffer);

	TextureImageData.imageView = CreateImageView(&TextureImageData, ImageFormat, vk::ImageAspectFlagBits::eColor);
	TextureImageData.imageSampler = CreateImageSampler();

	return TextureImageData;
}

ImageData BufferManager::LoadTextureImage(std::string FilePath, vk::Format ImageFormat, vk::CommandPool commandpool, vk::Queue Queue)
{
	int texWidth, textHeight, texChannels;

	stbi_uc* pixels = stbi_load(FilePath.c_str(), &texWidth, &textHeight, &texChannels, STBI_rgb_alpha);
	const void* pixeldata = pixels;

	vk::DeviceSize imagesize = texWidth * textHeight * 4;;

	vk::BufferCreateInfo StagingBufferCreateInfo = {};
	StagingBufferCreateInfo.size = imagesize;
	StagingBufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	StagingBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo AllocCreateInfo = {};
	AllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	AllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VkBuffer cStagingBuffer;
	VmaAllocation StagingBufferAllocation;

	VkBufferCreateInfo cStagingBufferCreateInfo = static_cast<VkBufferCreateInfo>(StagingBufferCreateInfo);

	if (vmaCreateBuffer(allocator, &cStagingBufferCreateInfo, &AllocCreateInfo, &cStagingBuffer, &StagingBufferAllocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	BufferData StagineBuffer;
	StagineBuffer.buffer = vk::Buffer(cStagingBuffer);
	StagineBuffer.size = imagesize;
	StagineBuffer.allocation = StagingBufferAllocation;
	StagineBuffer.usage = vk::BufferUsageFlagBits::eTransferSrc;
	StagineBuffer.BufferID = "Texture Staging Buffer";
	AddBufferLog(&StagineBuffer);

	CopyDataToBuffer(pixeldata, StagineBuffer);

	//stbi_image_free(pixeldata);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	vk::Extent3D imageExtent = { static_cast<uint32_t>(texWidth),static_cast<uint32_t>(textHeight),1 };

	ImageData TextureImageData;
	TextureImageData.ImageID = "StagineBuffer texture";

	CreateImage(&TextureImageData, imageExtent, ImageFormat, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	vk::CommandBuffer CommandBuffer = CreateSingleUseCommandBuffer(commandpool);

	ImageTransitionData DataToTransitionInfo;
	DataToTransitionInfo.oldlayout = vk::ImageLayout::eUndefined;
	DataToTransitionInfo.newlayout = vk::ImageLayout::eTransferDstOptimal;
	DataToTransitionInfo.SourceAccessflag = vk::AccessFlagBits::eNone;
	DataToTransitionInfo.DestinationAccessflag = vk::AccessFlagBits::eTransferWrite;
	DataToTransitionInfo.AspectFlag = vk::ImageAspectFlagBits::eColor;
	DataToTransitionInfo.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	DataToTransitionInfo.DestinationOnThePipeline = vk::PipelineStageFlagBits::eTransfer;

	TransitionImage(CommandBuffer, &TextureImageData, DataToTransitionInfo);

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

	CommandBuffer.copyBufferToImage(StagineBuffer.buffer, TextureImageData.image, vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);

	ImageTransitionData TransitionImageToShaderData;
	TransitionImageToShaderData.oldlayout = vk::ImageLayout::eTransferDstOptimal;
	TransitionImageToShaderData.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	TransitionImageToShaderData.SourceAccessflag = vk::AccessFlagBits::eTransferWrite;
	TransitionImageToShaderData.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
	TransitionImageToShaderData.AspectFlag = vk::ImageAspectFlagBits::eColor;
	TransitionImageToShaderData.SourceOnThePipeline = vk::PipelineStageFlagBits::eTransfer;
	TransitionImageToShaderData.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

	TransitionImage(CommandBuffer, &TextureImageData, TransitionImageToShaderData);

	SubmitAndDestoyCommandBuffer(commandpool, CommandBuffer, Queue);

	DestroyBuffer(StagineBuffer);

	TextureImageData.imageView = CreateImageView(&TextureImageData, ImageFormat, vk::ImageAspectFlagBits::eColor);
	TextureImageData.imageSampler = CreateImageSampler();

	return TextureImageData;
}

void BufferManager::CreateCubeMap(ImageData* imageData,std::array<const char*, 6> filePaths, vk::CommandPool commandpool, vk::Queue Queue)
{
	struct FaceData {
		stbi_uc* pixels;
		int width;
		int height;
	};

	std::vector<FaceData> faces;
	vk::DeviceSize totalSize = 0;

	int faceWidth = 0, faceHeight = 0;

	for (int i = 0; i < filePaths.size(); i++) {

		int texWidth, texHeight, texChannels;

		stbi_uc* pixels = stbi_load(filePaths[i], &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		faceWidth = texWidth;
		faceHeight = texHeight;

		vk::DeviceSize faceSize = texWidth * texHeight * 4;
		
		totalSize += faceSize;

		faces.push_back({ pixels, texWidth, texHeight });
	}

	vk::BufferCreateInfo stagingBufferInfo = {};
	stagingBufferInfo.size = totalSize;
	stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	stagingBufferInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo stagingAllocInfo = {};
	stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VkBuffer cStagingBuffer;
	VmaAllocation stagingAllocation;
	if (vmaCreateBuffer(allocator, reinterpret_cast<VkBufferCreateInfo*>(&stagingBufferInfo), &stagingAllocInfo,&cStagingBuffer,&stagingAllocation,nullptr) != VK_SUCCESS) {
		
		for (auto& face : faces) {
			stbi_image_free(face.pixels);
		}

		throw std::runtime_error("Failed to create staging buffer for cube map!");
	}

	void* mappedData;
	vmaMapMemory(allocator, stagingAllocation, &mappedData);
	vk::DeviceSize offset = 0;

	for (auto& face : faces) {
		
		vk::DeviceSize faceSize = face.width * face.height * 4;

		memcpy(static_cast<char*>(mappedData) + offset, face.pixels, faceSize);

		stbi_image_free(face.pixels);
		offset += faceSize;
	}

	vmaUnmapMemory(allocator, stagingAllocation);


	// Create the cube map image (6 array layers)
	vk::Extent3D imageExtent = { static_cast<uint32_t>(faceWidth),
								 static_cast<uint32_t>(faceHeight),
								1 };

	imageData->miplevels = std::floor(std::log2(std::max(imageExtent.width, imageExtent.height))) + 1;

	vk::ImageCreateInfo imageInfo = {};
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.extent = imageExtent;
	imageInfo.mipLevels = imageData->miplevels; // set amount of mips image should have
	imageInfo.arrayLayers = 6;
	imageInfo.format = vk::Format::eR8G8B8A8Srgb;
	imageInfo.tiling = vk::ImageTiling::eOptimal;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled;
	imageInfo.sharingMode = vk::SharingMode::eExclusive;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	imageInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible; 


	VmaAllocationCreateInfo imageAllocInfo = {};
	imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VkImage cCubeImage;
	VmaAllocation cubeAllocation;
	if (vmaCreateImage(allocator,reinterpret_cast<VkImageCreateInfo*>(&imageInfo),&imageAllocInfo,&cCubeImage,&cubeAllocation,nullptr) != VK_SUCCESS) {
		vmaDestroyBuffer(allocator, cStagingBuffer, stagingAllocation);
		throw std::runtime_error("Failed to create cube map image!");
	}

	imageData->image = vk::Image(cCubeImage);
	imageData->allocation = cubeAllocation;

	AddImageLog(imageData);
	// Transfer data from staging buffer to cube map image
	vk::CommandBuffer cmdBuffer = CreateSingleUseCommandBuffer(commandpool);

	vk::ImageMemoryBarrier acquireBarrier{};
	acquireBarrier.oldLayout = vk::ImageLayout::eUndefined;
	acquireBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
	acquireBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	acquireBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	acquireBarrier.image = imageData->image;
	acquireBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	acquireBarrier.subresourceRange.baseMipLevel = 0;
	acquireBarrier.subresourceRange.levelCount = imageData->miplevels; // set number of levels the image has (MIPMAP)
	acquireBarrier.subresourceRange.layerCount = 6;
	acquireBarrier.subresourceRange.baseArrayLayer = 0;
	acquireBarrier.srcAccessMask = {};
	acquireBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

	cmdBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eTransfer,
		vk::DependencyFlags(),
		0, nullptr,
		0, nullptr,
		1, &acquireBarrier
	);

	// Copy each face from the staging buffer into the image
	std::vector<vk::BufferImageCopy> copyRegions;
	offset = 0;

	for (uint32_t face = 0; face < 6; face++) {
		vk::BufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = offset;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;
		copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = face;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageOffset = vk::Offset3D{ 0, 0, 0 };
		copyRegion.imageExtent = imageExtent;

		copyRegions.push_back(copyRegion);
		offset += faceWidth * faceHeight * 4;
	}

	cmdBuffer.copyBufferToImage(
		vk::Buffer(cStagingBuffer),
		imageData->image,
		vk::ImageLayout::eTransferDstOptimal,
		static_cast<uint32_t>(copyRegions.size()),
		copyRegions.data()
	);



	GenerateMipMaps(imageData, &cmdBuffer, static_cast<float>(faceWidth), static_cast<float>(faceHeight), Queue,6);

	SubmitAndDestoyCommandBuffer(commandpool, cmdBuffer, Queue);

	// Clean up staging resources
	vmaDestroyBuffer(allocator, cStagingBuffer, stagingAllocation);

	// Create cube map view
	vk::ImageViewCreateInfo viewInfo = {};
	viewInfo.image = imageData->image;
	viewInfo.viewType = vk::ImageViewType::eCube; // This makes it a cube map
	viewInfo.format = vk::Format::eR8G8B8A8Srgb;
	viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 6; // All six faces
	viewInfo.subresourceRange.levelCount = imageData->miplevels;

	imageData->imageView = logicalDevice.createImageView(viewInfo);
	imageData->imageSampler = CreateImageSampler();
}

void BufferManager::GenerateMipMaps(ImageData* imageData, vk::CommandBuffer* cmdBuffer, float width, float height, vk::Queue graphicsqueue, int layerCount) {

	int mipLevels = imageData->miplevels;
	float mipWidth = width;
	float mipHeight = height;

	for (uint32_t i = 1; i < mipLevels; ++i) {

		// Transition the previos mipmap so that it can be used as the base texture for the next mipmap
		vk::ImageMemoryBarrier barrier{};
		barrier.image = imageData->image;
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = layerCount;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

		cmdBuffer->pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
			{}, 0, nullptr, 0, nullptr, 1, &barrier
		);

		// image data for previous mipmap
		vk::ImageBlit blit{};
		blit.srcOffsets[0] = vk::Offset3D{ 0, 0, 0 };
		blit.srcOffsets[1] = vk::Offset3D{ static_cast<int32_t>(mipWidth), static_cast<int32_t>(mipHeight), 1 };
		blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = layerCount;


		float nextMipWidth  = mipWidth  > 1 ? mipWidth  / 2 : 1; // image width division
		float nextMipHeight = mipHeight > 1 ? mipHeight / 2 : 1; // image height division

		// image data for new mipmap
		blit.dstOffsets[0] = vk::Offset3D{ 0, 0, 0 };
		blit.dstOffsets[1] = vk::Offset3D{ static_cast<int32_t>(nextMipWidth), static_cast<int32_t>(nextMipHeight), 1 };
		blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = layerCount;

		cmdBuffer->blitImage(
			imageData->image, vk::ImageLayout::eTransferSrcOptimal,
			imageData->image, vk::ImageLayout::eTransferDstOptimal,
			1, &blit, vk::Filter::eLinear
		);

		//transition so it can be used in the shader
		barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		cmdBuffer->pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
			{}, nullptr, nullptr, barrier);


		//set new size mipmap we just created
		mipWidth  = nextMipWidth;
		mipHeight = nextMipHeight;
	}

	vk::ImageMemoryBarrier lastBarrier{};
	lastBarrier.image = imageData->image;
	lastBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	lastBarrier.subresourceRange.baseMipLevel = mipLevels - 1;
	lastBarrier.subresourceRange.levelCount = 1;
	lastBarrier.subresourceRange.baseArrayLayer = 0;
	lastBarrier.subresourceRange.layerCount = layerCount;
	lastBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	lastBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	lastBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
	lastBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

	cmdBuffer->pipelineBarrier(
		vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
		{}, nullptr, nullptr, lastBarrier);
}


void BufferManager::CreateImage(ImageData* imageData,vk::Extent3D imageExtent, vk::Format imageFormat, vk::ImageUsageFlags UsageFlag,bool bMipMaps) {


	if (imageData->ImageID.empty())
	{
		throw std::exception("An image you are trying to create does not have an ID");
	}


	vk::ImageCreateInfo imagecreateinfo;
	imagecreateinfo.imageType = vk::ImageType::e2D;
	imagecreateinfo.extent = imageExtent;

	if (bMipMaps)
	{
		imageData->miplevels = std::floor(std::log2(std::max(imageExtent.width, imageExtent.height))) + 1;
		imagecreateinfo.mipLevels = imageData->miplevels;
		std::cout << imageData->miplevels << std::endl;
	}
	else
	{
		imagecreateinfo.mipLevels = 1;

	}
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

	VmaAllocation ImageAllocation;

	if (vmaCreateImage(allocator, &cimagecreateinfo, &imageAllocInfo, &cTextureImage, &ImageAllocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create image!");
	}

	imageData->image = vk::Image(cTextureImage);
	imageData->allocation = ImageAllocation;

	AddImageLog(imageData);
}

void BufferManager::AddImageLog(ImageData* imageData)
{
	if (imageData->ImageID.empty())
	{
		throw	std::runtime_error("A buffer you are trying to create does not have an ID");
	}

	std::string ConstructedID = imageData->ImageID;

	auto FoundLog = imageLog.find(ConstructedID);

	if (FoundLog == imageLog.end()) {
		IDdata data;
		data.instance = 0;

		imageLog.emplace(imageData->ImageID, data);
	}
	else
	{
		IDdata data;
		data.instance = FoundLog->second.instance + 1;

		auto newLogID = FoundLog->first + "instance" + std::to_string(data.instance);
		imageData->ImageID = newLogID;
		imageLog.emplace(newLogID, data);
	}
}

void BufferManager::RemoveImageLog(ImageData imageData)
{
	if (imageData.ImageID.empty())
	{
		throw std::runtime_error("A buffer you are trying to destroy does not have an ID");
	}
	auto FoundLog = imageLog.find(imageData.ImageID);

	if (FoundLog != imageLog.end()) {
		imageLog.erase(FoundLog);
	}
}
vk::ImageView BufferManager::CreateImageView(ImageData* imageData, vk::Format ImageFormat = vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlags ImageAspectBits = vk::ImageAspectFlagBits::eColor) {

	vk::ImageViewCreateInfo imageviewinfo{};
	imageviewinfo.image = imageData->image;
	imageviewinfo.viewType = vk::ImageViewType::e2D;
	imageviewinfo.format = ImageFormat;
	imageviewinfo.subresourceRange.aspectMask = ImageAspectBits;
	imageviewinfo.subresourceRange.baseMipLevel = 0;
	imageviewinfo.subresourceRange.levelCount = imageData->miplevels;
	imageviewinfo.subresourceRange.baseArrayLayer = 0;
	imageviewinfo.subresourceRange.layerCount = 1;

   return logicalDevice.createImageView(imageviewinfo);
}

vk::Sampler BufferManager::CreateImageSampler(vk::SamplerAddressMode addressMode) {

	vk::SamplerCreateInfo SamplerInfo{};
	SamplerInfo.magFilter = vk::Filter::eLinear;
	SamplerInfo.minFilter = vk::Filter::eLinear;
	SamplerInfo.addressModeU = addressMode;
	SamplerInfo.addressModeV = addressMode;
	SamplerInfo.addressModeW = addressMode;
	SamplerInfo.anisotropyEnable = vk::True;
	SamplerInfo.maxAnisotropy = physicalDevice.getProperties().limits.maxSamplerAnisotropy;
	SamplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	SamplerInfo.unnormalizedCoordinates = vk::False;
	SamplerInfo.compareEnable = vk::False;
	SamplerInfo.compareOp = vk::CompareOp::eNever;
	SamplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	SamplerInfo.mipLodBias = 0.0f;
	SamplerInfo.minLod = 0.0f;
	SamplerInfo.maxLod = 10.0f;
	
	return logicalDevice.createSampler(SamplerInfo);
}

void BufferManager::TransitionImage(vk::CommandBuffer CommandBuffer, ImageData* imageData, ImageTransitionData& imagetransinotdata) {
	

	vk::ImageMemoryBarrier acquireBarrier{};
	acquireBarrier.oldLayout = imagetransinotdata.oldlayout;
	acquireBarrier.newLayout = imagetransinotdata.newlayout;
	acquireBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	acquireBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	acquireBarrier.image = imageData->image;
	acquireBarrier.subresourceRange.aspectMask = imagetransinotdata.AspectFlag;
	acquireBarrier.subresourceRange.baseMipLevel = imagetransinotdata.BaseMipLevel;
	acquireBarrier.subresourceRange.levelCount = imagetransinotdata.LevelCount;
	acquireBarrier.subresourceRange.baseArrayLayer = 0;
	acquireBarrier.subresourceRange.layerCount = imagetransinotdata.LayerCount;
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

void BufferManager::DestroyBuffer(BufferData& buffer) {

	
	RemoveBufferLog(buffer);
	vmaDestroyBuffer(allocator, static_cast<VkBuffer>(buffer.buffer), buffer.allocation);
}

void BufferManager::DestroyImage(const ImageData& imagedata) {

	if (imagedata.imageView != VK_NULL_HANDLE)
	{
		logicalDevice.destroyImageView(imagedata.imageView);
	}

	if (imagedata.imageSampler != VK_NULL_HANDLE)
	{
		logicalDevice.destroySampler(imagedata.imageSampler);
	}

	if (imagedata.image != VK_NULL_HANDLE && imagedata.allocation != VK_NULL_HANDLE)
	{
		vmaDestroyImage(allocator, imagedata.image, imagedata.allocation);
	}

	RemoveImageLog(imagedata);
}

BufferManager::~BufferManager()
{
	//vmaDestroyAllocator(allocator);
}

void BufferManager::DeleteAllocation(VmaAllocation allocation)
{
	if (allocation) {
		vmaFreeMemory(allocator, allocation);
		delete allocation;
	}
}
