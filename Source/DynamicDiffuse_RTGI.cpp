#include "DynamicDiffuse_RTGI.h"
#include "VulkanContext.h"
#include "BufferManager.h"
#include "Camera.h"
#include <stdexcept>
#include "AssetManager.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

DynamicDiffuse_RTGI::DynamicDiffuse_RTGI(const std::string filepath, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger)
{
	bufferManager = buffermanger;
	vulkanContext = vulkancontext;
	camera        = rcamera;
	commandPool   = commandpool;
	FilePath = filepath;

	CreateVertexAndIndexBuffer();
	CreateStorageBuffer();
	createRayTracingDescriptorSetLayout();
	UpdateGrid = true;
}
void DynamicDiffuse_RTGI::CreateVertexAndIndexBuffer() {

	storedModelData = &AssetManager::GetInstance().GetStoredModelData(FilePath);

	VkDeviceSize VertexBufferSize = sizeof(storedModelData->VertexData[0]) * storedModelData->VertexData.size();
	vertexBufferData.BufferID = "Model Vertex Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&vertexBufferData, storedModelData->VertexData.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(uint32_t) * storedModelData->IndexData.size();
	indexBufferData.BufferID = "Model Index Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&indexBufferData, storedModelData->IndexData.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, commandPool, vulkanContext->graphicsQueue);


}

void DynamicDiffuse_RTGI::CreateStorageBuffer()
{
	{
		ProbePositionsStorageBuffers.resize(1);

		VkDeviceSize ComputeStorageBufferSize = sizeof(glm::vec4) * 1000;

		for (size_t i = 0; i < 1; i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "Probe Positions Buffer" + i;
			bufferManager->CreateGPU_Only_Buffer(&bufferdata, ComputeStorageBufferSize, vk::BufferUsageFlagBits::eStorageBuffer, commandPool, vulkanContext->graphicsQueue);
			ProbePositionsStorageBuffers[i] = bufferdata;
		}
	}

	{
		ProbeFibonacciDirectionsStorageBuffers.resize(1);

		VkDeviceSize ComputeStorageBufferSize = sizeof(glm::vec4) * 1000;

		for (size_t i = 0; i < 1; i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "Probe Fibonacci Buffer" + i;
			bufferManager->CreateGPU_Only_Buffer(&bufferdata, ComputeStorageBufferSize, vk::BufferUsageFlagBits::eStorageBuffer, commandPool, vulkanContext->graphicsQueue);
			ProbeFibonacciDirectionsStorageBuffers[i] = bufferdata;
		}
	}
}

void DynamicDiffuse_RTGI::CreateStorageImage() {

	 swapchainextent = vk::Extent3D(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, 1);

	 IrradianceImageAtlasImage.ImageID = " DDGI Irradiance Atlas Image";
	 bufferManager->CreateImage(&IrradianceImageAtlasImage, swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, true);
	 IrradianceImageAtlasImage.imageView = bufferManager->CreateImageView(&IrradianceImageAtlasImage, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	 IrradianceImageAtlasImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);
     

	 ProbeVisibilityAtlasImage.ImageID = " DDGI Probe Visibility Atlas Image";
	 bufferManager->CreateImage(&ProbeVisibilityAtlasImage, swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, true);
	 ProbeVisibilityAtlasImage.imageView = bufferManager->CreateImageView(&ProbeVisibilityAtlasImage, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	 ProbeVisibilityAtlasImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);
}


void DynamicDiffuse_RTGI::DestroyStorageImage() {

	bufferManager->DestroyImage(IrradianceImageAtlasImage);
	bufferManager->DestroyImage(ProbeVisibilityAtlasImage);
}

void DynamicDiffuse_RTGI::UpdateProbeCount(glm::vec3 NewProbeCount)
{
	NumOfProbesX = (int)NewProbeCount.x;
	NumOfProbesY = (int)NewProbeCount.y;
	NumOfProbesZ = (int)NewProbeCount.z;


	if (LastNumOfProbesX != NumOfProbesX || LastNumOfProbesY != NumOfProbesY || LastNumOfProbesZ != NumOfProbesZ)
	{

		LastNumOfProbesX = NumOfProbesX;

		UpdateGrid = true;
	}
	else
	{
		UpdateGrid = false;
	}
}

void DynamicDiffuse_RTGI::UpdateProbsOffset(glm::vec3 NewProbeOffset)
{
	ProbeOffset = NewProbeOffset;

	if (LastProbeOffset != ProbeOffset)
	{
		LastProbeOffset = ProbeOffset;

		UpdateGrid = true;
	}
	else
	{
		UpdateGrid = false;
	}
}

void DynamicDiffuse_RTGI::UpdateGridLocation(glm::vec3 NewGridLocation)
{
	GridLocation = NewGridLocation;

	if (LastProbeOffset != GridLocation)
	{
		LastProbeOffset = GridLocation;

		UpdateGrid = true;
	}
	else
	{
		UpdateGrid = false;
	}
}

void DynamicDiffuse_RTGI::createRayTracingDescriptorSetLayout(){

	{
		vk::DescriptorSetLayoutBinding IrradianceAtlasSamplerLayout{};
		IrradianceAtlasSamplerLayout.binding = 0;
		IrradianceAtlasSamplerLayout.descriptorCount = 1;
		IrradianceAtlasSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		IrradianceAtlasSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding  VisibilitiyAtlasSamplerLayout{};
		VisibilitiyAtlasSamplerLayout.binding = 1;
		VisibilitiyAtlasSamplerLayout.descriptorCount = 1;
		VisibilitiyAtlasSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		VisibilitiyAtlasSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding  ProbeLocationUniformBuffer{};
		ProbeLocationUniformBuffer.binding = 2;
		ProbeLocationUniformBuffer.descriptorCount = 1;
		ProbeLocationUniformBuffer.descriptorType = vk::DescriptorType::eStorageBuffer;
		ProbeLocationUniformBuffer.stageFlags = vk::ShaderStageFlagBits::eVertex;

		std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {
																   IrradianceAtlasSamplerLayout,
																   VisibilitiyAtlasSamplerLayout,
																   ProbeLocationUniformBuffer
		};

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &ProbeDescriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
	}


	{
		vk::DescriptorSetLayoutBinding ProbePositionStorageBufffer{};
		ProbePositionStorageBufffer.binding = 0;
		ProbePositionStorageBufffer.descriptorCount = 1;
		ProbePositionStorageBufffer.descriptorType = vk::DescriptorType::eStorageBuffer;
		ProbePositionStorageBufffer.stageFlags = vk::ShaderStageFlagBits::eCompute;

		vk::DescriptorSetLayoutBinding FibonacciDirectionsStorageBufffer{};
		FibonacciDirectionsStorageBufffer.binding = 1;
		FibonacciDirectionsStorageBufffer.descriptorCount = 1;
		FibonacciDirectionsStorageBufffer.descriptorType = vk::DescriptorType::eStorageBuffer;
		FibonacciDirectionsStorageBufffer.stageFlags = vk::ShaderStageFlagBits::eCompute;

		std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { ProbePositionStorageBufffer,FibonacciDirectionsStorageBufffer };

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &GridDescriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
	}


	//{
	//	vk::DescriptorSetLayoutBinding TLASLayout{};
	//	TLASLayout.binding = 0;
	//	TLASLayout.descriptorCount = 1;
	//	TLASLayout.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
	//	TLASLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;
	//
	//	vk::DescriptorSetLayoutBinding AlbedoAssetTexturesSamplerLayout{};
	//	AlbedoAssetTexturesSamplerLayout.binding = 1;
	//	AlbedoAssetTexturesSamplerLayout.descriptorCount = bufferManager->AllScene_Albedo_Images.size();
	//	AlbedoAssetTexturesSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	//	AlbedoAssetTexturesSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;
	//
	//	vk::DescriptorSetLayoutBinding NormalAssetTexturesSamplerLayout{};
	//	NormalAssetTexturesSamplerLayout.binding = 2;
	//	NormalAssetTexturesSamplerLayout.descriptorCount = bufferManager->AllScene_Normal_Images.size();
	//	NormalAssetTexturesSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	//	NormalAssetTexturesSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;
	//
	//	vk::DescriptorSetLayoutBinding MetalicRoughnessAssetTexturesSamplerLayout{};
	//	MetalicRoughnessAssetTexturesSamplerLayout.binding = 3;
	//	MetalicRoughnessAssetTexturesSamplerLayout.descriptorCount = bufferManager->AllScene_MetalicRoughness_Images.size();
	//	MetalicRoughnessAssetTexturesSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	//	MetalicRoughnessAssetTexturesSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;
	//
	//	vk::DescriptorSetLayoutBinding IndexStorageBuffersLayout{};
	//	IndexStorageBuffersLayout.binding = 4;
	//	IndexStorageBuffersLayout.descriptorCount = 1;
	//	IndexStorageBuffersLayout.descriptorType = vk::DescriptorType::eStorageBuffer;
	//	IndexStorageBuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;
	//
	//	vk::DescriptorSetLayoutBinding VertexStorageBuffersLayout{};
	//	VertexStorageBuffersLayout.binding = 5;
	//	VertexStorageBuffersLayout.descriptorCount = 1;
	//	VertexStorageBuffersLayout.descriptorType = vk::DescriptorType::eStorageBuffer;
	//	VertexStorageBuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;
	//
	//	vk::DescriptorSetLayoutBinding offsetStorageBuffersLayout{};
	//	offsetStorageBuffersLayout.binding = 6;
	//	offsetStorageBuffersLayout.descriptorCount = 1;
	//	offsetStorageBuffersLayout.descriptorType = vk::DescriptorType::eStorageBuffer;
	//	offsetStorageBuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;
	//
	//	vk::DescriptorSetLayoutBinding trasnformationUniformBuffersLayout{};
	//	trasnformationUniformBuffersLayout.binding = 7;
	//	trasnformationUniformBuffersLayout.descriptorCount = 1;
	//	trasnformationUniformBuffersLayout.descriptorType = vk::DescriptorType::eUniformBuffer;
	//	trasnformationUniformBuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;
	//
	//	vk::DescriptorSetLayoutBinding IrradianceAtlasStorageLayout{};
	//	IrradianceAtlasStorageLayout.binding = 8;
	//	IrradianceAtlasStorageLayout.descriptorCount = 1;
	//	IrradianceAtlasStorageLayout.descriptorType = vk::DescriptorType::eStorageBuffer;
	//	IrradianceAtlasStorageLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;
	//
	//	vk::DescriptorSetLayoutBinding  VisibilitiyAtlasStorageLayout{};
	//	VisibilitiyAtlasStorageLayout.binding = 9;
	//	VisibilitiyAtlasStorageLayout.descriptorCount = 1;
	//	VisibilitiyAtlasStorageLayout.descriptorType = vk::DescriptorType::eStorageBuffer;
	//	VisibilitiyAtlasStorageLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;
	//
	//	std::array<vk::DescriptorSetLayoutBinding, 9> bindings = {
	//															   TLASLayout,
	//															   AlbedoAssetTexturesSamplerLayout,
	//															   NormalAssetTexturesSamplerLayout,
	//															   IndexStorageBuffersLayout,
	//															   VertexStorageBuffersLayout,
	//															   offsetStorageBuffersLayout,
	//															   trasnformationUniformBuffersLayout,
	//															   IrradianceAtlasStorageLayout,VisibilitiyAtlasStorageLayout
	//	};
	//
	//
	//
	//	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	//	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	//	layoutInfo.pBindings = bindings.data();
	//
	//
	//	if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &RaytracingDescriptorSetLayout) != vk::Result::eSuccess)
	//	{
	//		throw std::runtime_error("Failed to create descriptorset layout!");
	//	}
	//
	//}




}


void DynamicDiffuse_RTGI::createRaytracedDescriptorSets(vk::DescriptorPool descriptorpool)
{
	{
		// create sets from the pool based on the layout
		// 	     
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, ProbeDescriptorSetLayout);


		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = layouts.data();

		ProbeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, ProbeDescriptorSets.data());

		////////////////////////////////////////////////////////////////////////////////////////////////
		//specifies what exactly to send
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			vk::DescriptorImageInfo IrradiancAtlasImageInfo{};
			IrradiancAtlasImageInfo.imageLayout =  vk::ImageLayout::eGeneral;
			IrradiancAtlasImageInfo.imageView   =  IrradianceImageAtlasImage.imageView;
			IrradiancAtlasImageInfo.sampler     =  IrradianceImageAtlasImage.imageSampler;
		

			vk::WriteDescriptorSet IrradianceAtlasImageSamplerdescriptorWrite{};
			IrradianceAtlasImageSamplerdescriptorWrite.dstSet          = ProbeDescriptorSets[i];
			IrradianceAtlasImageSamplerdescriptorWrite.dstBinding      = 0;
			IrradianceAtlasImageSamplerdescriptorWrite.dstArrayElement = 0;
			IrradianceAtlasImageSamplerdescriptorWrite.descriptorType  = vk::DescriptorType::eCombinedImageSampler;
			IrradianceAtlasImageSamplerdescriptorWrite.descriptorCount = 1;
			IrradianceAtlasImageSamplerdescriptorWrite.pImageInfo      = &IrradiancAtlasImageInfo;

			vk::DescriptorImageInfo ProbeVisibilityAtlasImageInfo{};
			ProbeVisibilityAtlasImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			ProbeVisibilityAtlasImageInfo.imageView   = ProbeVisibilityAtlasImage.imageView;
			ProbeVisibilityAtlasImageInfo.sampler     = ProbeVisibilityAtlasImage.imageSampler;


			vk::WriteDescriptorSet  ProbeVisibilityAtlasSamplerdescriptorWrite{};
			ProbeVisibilityAtlasSamplerdescriptorWrite.dstSet = ProbeDescriptorSets[i];
			ProbeVisibilityAtlasSamplerdescriptorWrite.dstBinding = 1;
			ProbeVisibilityAtlasSamplerdescriptorWrite.dstArrayElement = 0;
			ProbeVisibilityAtlasSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			ProbeVisibilityAtlasSamplerdescriptorWrite.descriptorCount = 1;
			ProbeVisibilityAtlasSamplerdescriptorWrite.pImageInfo = &ProbeVisibilityAtlasImageInfo;

			vk::DescriptorBufferInfo ProbeLocationbufferInfo{};
			ProbeLocationbufferInfo.buffer = ProbePositionsStorageBuffers[0].buffer;
			ProbeLocationbufferInfo.offset = 0;
			ProbeLocationbufferInfo.range = sizeof(glm::vec4) * 1000;

			vk::WriteDescriptorSet ProbeLocationbufferdescriptorWrite{};
			ProbeLocationbufferdescriptorWrite.dstSet = ProbeDescriptorSets[i];
			ProbeLocationbufferdescriptorWrite.dstBinding = 2;
			ProbeLocationbufferdescriptorWrite.dstArrayElement = 0;
			ProbeLocationbufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
			ProbeLocationbufferdescriptorWrite.descriptorCount = 1;
			ProbeLocationbufferdescriptorWrite.pBufferInfo = &ProbeLocationbufferInfo;


			std::array<vk::WriteDescriptorSet, 3> descriptorWrites{ IrradianceAtlasImageSamplerdescriptorWrite,
																	ProbeVisibilityAtlasSamplerdescriptorWrite,ProbeLocationbufferdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}


	{
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, GridDescriptorSetLayout);


		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = layouts.data();

		GridDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, GridDescriptorSets.data());

		////////////////////////////////////////////////////////////////////////////////////////////////
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			vk::DescriptorBufferInfo ProbeLocationbufferInfo{};
			ProbeLocationbufferInfo.buffer = ProbePositionsStorageBuffers[0].buffer;
			ProbeLocationbufferInfo.offset = 0;
			ProbeLocationbufferInfo.range = sizeof(glm::vec4) * 1000;

			vk::WriteDescriptorSet ProbeLocationbufferdescriptorWrite{};
			ProbeLocationbufferdescriptorWrite.dstSet = GridDescriptorSets[i];
			ProbeLocationbufferdescriptorWrite.dstBinding = 0;
			ProbeLocationbufferdescriptorWrite.dstArrayElement = 0;
			ProbeLocationbufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
			ProbeLocationbufferdescriptorWrite.descriptorCount = 1;
			ProbeLocationbufferdescriptorWrite.pBufferInfo = &ProbeLocationbufferInfo;

			vk::DescriptorBufferInfo FibonacciDirectionsbufferInfo{};
			FibonacciDirectionsbufferInfo.buffer = ProbeFibonacciDirectionsStorageBuffers[0].buffer;
			FibonacciDirectionsbufferInfo.offset = 0;
			FibonacciDirectionsbufferInfo.range = sizeof(glm::vec4) * 60;

			vk::WriteDescriptorSet FibonacciDirectionsbufferdescriptorWrite{};
			FibonacciDirectionsbufferdescriptorWrite.dstSet = GridDescriptorSets[i];
			FibonacciDirectionsbufferdescriptorWrite.dstBinding = 1;
			FibonacciDirectionsbufferdescriptorWrite.dstArrayElement = 0;
			FibonacciDirectionsbufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
			FibonacciDirectionsbufferdescriptorWrite.descriptorCount = 1;
			FibonacciDirectionsbufferdescriptorWrite.pBufferInfo = &FibonacciDirectionsbufferInfo;


			std::array<vk::WriteDescriptorSet, 2> descriptorWrites{ProbeLocationbufferdescriptorWrite,FibonacciDirectionsbufferdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}

}


uint32_t DynamicDiffuse_RTGI::alignedSize(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}


void DynamicDiffuse_RTGI::Draw(BufferData RayGenBuffer, BufferData RayHitBuffer, BufferData RayMisBuffer, vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
{
	//vk::BufferDeviceAddressInfo raygenShaderBindingTableDeviceAdressesInfo;
	//raygenShaderBindingTableDeviceAdressesInfo.buffer = RayGenBuffer.buffer;
	//
	//vk::BufferDeviceAddressInfo missShaderBindingTableDeviceAdressesInfo;
	//missShaderBindingTableDeviceAdressesInfo.buffer = RayMisBuffer.buffer;
	//
	//vk::BufferDeviceAddressInfo hitShaderBindingTableDeviceAdressesInfo;
	//hitShaderBindingTableDeviceAdressesInfo.buffer = RayHitBuffer.buffer;
	//
	//auto raygenShaderBindingTableAdress = vulkanContext->LogicalDevice.getBufferAddress(raygenShaderBindingTableDeviceAdressesInfo);
	//auto missShaderBindingTableAdress   = vulkanContext->LogicalDevice.getBufferAddress(missShaderBindingTableDeviceAdressesInfo);
	//auto hitShaderBindingTableAdress    = vulkanContext->LogicalDevice.getBufferAddress(hitShaderBindingTableDeviceAdressesInfo);
	//
	//
	//const uint32_t handleSizeAligned = alignedSize(
	//	vulkanContext->RayTracingPipelineProperties.shaderGroupHandleSize,
	//	vulkanContext->RayTracingPipelineProperties.shaderGroupHandleAlignment);
	//
	//vk::StridedDeviceAddressRegionKHR    raygenShaderSbtEntry{};
	//raygenShaderSbtEntry.deviceAddress = raygenShaderBindingTableAdress;
	//raygenShaderSbtEntry.stride = handleSizeAligned;
	//raygenShaderSbtEntry.size = handleSizeAligned;
	//
	//
	//vk::StridedDeviceAddressRegionKHR  missShaderSbtEntry{};
	//missShaderSbtEntry.deviceAddress = missShaderBindingTableAdress;
	//missShaderSbtEntry.stride = handleSizeAligned;
	//missShaderSbtEntry.size = handleSizeAligned;
	//
	//vk::StridedDeviceAddressRegionKHR hitShaderSbtEntry{};
	//hitShaderSbtEntry.deviceAddress = hitShaderBindingTableAdress;
	//hitShaderSbtEntry.stride = handleSizeAligned;
	//hitShaderSbtEntry.size = handleSizeAligned;
	//
	////vk::StridedDeviceAddressRegionKHR hitShaderSbtEntry{};
	//
	//vk::StridedDeviceAddressRegionKHR callableShaderSbtEntry{};
	//
	//VkStridedDeviceAddressRegionKHR  TEMP_raygenShaderSbtEntry   = static_cast<VkStridedDeviceAddressRegionKHR>(raygenShaderSbtEntry);
	//VkStridedDeviceAddressRegionKHR  TEMP_missShaderSbtEntry     = static_cast<VkStridedDeviceAddressRegionKHR>(missShaderSbtEntry);;
	//VkStridedDeviceAddressRegionKHR  TEMP_hitShaderSbtEntry      = static_cast<VkStridedDeviceAddressRegionKHR>(hitShaderSbtEntry);;
	//VkStridedDeviceAddressRegionKHR  TEMP_callableShaderSbtEntry = static_cast<VkStridedDeviceAddressRegionKHR>(callableShaderSbtEntry);;
	//
	//int width  = swapchainextent.width;
	//int height = swapchainextent.height;
	//int depth  = 1;
	//commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, pipelinelayout, 0, 1, &RayTracingDescriptorSets[imageIndex],0,nullptr);
	//
	//vulkanContext->vkCmdTraceRaysKHR(
	//	commandbuffer,
	//	&TEMP_raygenShaderSbtEntry,
	//	&TEMP_missShaderSbtEntry,
	//	&TEMP_hitShaderSbtEntry,
	//	&TEMP_callableShaderSbtEntry,
	//	width,
	//	height,
	//	depth);
}

void DynamicDiffuse_RTGI::DrawNode(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex, const std::vector<std::shared_ptr<Node>>& nodes)
{
	for (const auto& node : nodes) {

		if (!node) continue;


		for (int i = 0; i < node->meshPrimitives.size(); i++)
		{

			if (node->meshPrimitives[i].numIndices > 0) {

				CameraConstantBuffer cameraConstantBuffer;
				cameraConstantBuffer.ViewMatrix = camera->GetViewMatrix();
				cameraConstantBuffer.ProjectionMatrix = camera->GetProjectionMatrix();
				cameraConstantBuffer.ProjectionMatrix[1][1] *= -1;

				commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(CameraConstantBuffer), &cameraConstantBuffer);
				commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &ProbeDescriptorSets[imageIndex], 0, nullptr);

				commandBuffer.drawIndexed(node->meshPrimitives[i].numIndices, NumOfProbesX * NumOfProbesY * NumOfProbesZ, node->meshPrimitives[i].indicesStart, 0, 0);
			}
		}



		DrawNode(commandBuffer, pipelineLayout, imageIndex, node->children);
	}
}

void DynamicDiffuse_RTGI::DispatchGridCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex)
{
	if (UpdateGrid)
	{
		uint32_t numElements = NumOfProbesX * NumOfProbesY * NumOfProbesZ;
		uint32_t localSizeX = 32;
		uint32_t workGroupsX = numElements / localSizeX;

		GridData gridData;
		gridData.probeCount = glm::vec4(NumOfProbesX, NumOfProbesY, NumOfProbesZ, 1);
		gridData.probeOffset = glm::vec4(ProbeOffset, 1);
		gridData.probeBaseLocation = glm::vec4(GridLocation, 1);

		commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(GridData), &gridData);

		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, 1, &GridDescriptorSets[imageIndex], 0, nullptr);
		commandBuffer.dispatch(workGroupsX, 1, 1);
	}
}

void DynamicDiffuse_RTGI::Draw(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer vertexBuffers[] = { vertexBufferData.buffer };

	commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
	commandBuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint32);

	DrawNode(commandBuffer, pipelineLayout, imageIndex, storedModelData->nodes);
}



void DynamicDiffuse_RTGI::CleanUp()
{

	for (auto& buffer : ProbePositionsStorageBuffers)
	{
		if (buffer.buffer)
		{
			//bufferManager->UnmapMemory(buffer);
			bufferManager->DestroyBuffer(buffer);
		}
	}

	for (auto& buffer : ProbeFibonacciDirectionsStorageBuffers)
	{
		if (buffer.buffer)
		{
			//bufferManager->UnmapMemory(buffer);
			bufferManager->DestroyBuffer(buffer);
		}
	}


	ProbePositionsStorageBuffers.clear();

	if (vertexBufferData.buffer)
	{
		bufferManager->DestroyBuffer(vertexBufferData);
	}

	if (indexBufferData.buffer)
	{
		bufferManager->DestroyBuffer(indexBufferData);
	}

	vulkanContext->LogicalDevice.destroyDescriptorSetLayout(ProbeDescriptorSetLayout);


	ProbeLocations.clear();
}



