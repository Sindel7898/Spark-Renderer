#include "RT_Reflections.h"
#include "VulkanContext.h"
#include "BufferManager.h"
#include "Camera.h"
#include "Light.h"

#include <stdexcept>

RT_Reflections::RT_Reflections( VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger)
{
	bufferManager = buffermanger;
	vulkanContext = vulkancontext;
	camera        = rcamera;
	commandPool   = commandpool;
	CreateUniformBuffer();
	createRayTracingDescriptorSetLayout();
}

void RT_Reflections::CreateUniformBuffer() {


}

void RT_Reflections::CreateStorageImage() {

	 swapchainextent = vk::Extent3D(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, 1);

	 
	 RT_ReflectionPassImage.ImageID = "RT Shadow Pass Image";
	 bufferManager->CreateImage(&RT_ReflectionPassImage, swapchainextent, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled);
	 RT_ReflectionPassImage.imageView = bufferManager->CreateImageView(&RT_ReflectionPassImage, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
	 RT_ReflectionPassImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);
	 
}
void RT_Reflections::DestroyStorageImage() {

		bufferManager->DestroyImage(RT_ReflectionPassImage);

}

void RT_Reflections::createRayTracingDescriptorSetLayout(){

	vk::DescriptorSetLayoutBinding TLASLayout{};
	TLASLayout.binding = 0;
	TLASLayout.descriptorCount = 1;
	TLASLayout.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
	TLASLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

	vk::DescriptorSetLayoutBinding PositionSamplerLayout{};
	PositionSamplerLayout.binding = 1;
	PositionSamplerLayout.descriptorCount = 1;
	PositionSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	PositionSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

	vk::DescriptorSetLayoutBinding NormalSamplerLayout{};
	NormalSamplerLayout.binding = 2;
	NormalSamplerLayout.descriptorCount = 1;
	NormalSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	NormalSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

	vk::DescriptorSetLayoutBinding Rt_ReflectionResultSamplerLayout{};
	Rt_ReflectionResultSamplerLayout.binding = 3;
	Rt_ReflectionResultSamplerLayout.descriptorCount = 1;
	Rt_ReflectionResultSamplerLayout.descriptorType = vk::DescriptorType::eStorageImage;
	Rt_ReflectionResultSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;


	std::array<vk::DescriptorSetLayoutBinding, 4> bindings = { TLASLayout,PositionSamplerLayout, 
		                                                       NormalSamplerLayout,Rt_ReflectionResultSamplerLayout };

	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();


	if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &RayTracingDescriptorSetLayout) != vk::Result::eSuccess)
	{
		throw std::runtime_error("Failed to create descriptorset layout!");
	}

}

void RT_Reflections::createRaytracedDescriptorSets(vk::DescriptorPool descriptorpool, vk::AccelerationStructureKHR TLAS,GBuffer gbuffer)
{
	{
		// create sets from the pool based on the layout
		// 	     
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, RayTracingDescriptorSetLayout);


		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = layouts.data();

		RayTracingDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, RayTracingDescriptorSets.data());

		////////////////////////////////////////////////////////////////////////////////////////////////
		//specifies what exactly to send
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			vk::WriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
			descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
			descriptorAccelerationStructureInfo.pAccelerationStructures = &TLAS;

			vk::WriteDescriptorSet TLAS_descriptorWrite{};
			TLAS_descriptorWrite.dstSet = RayTracingDescriptorSets[i];
			TLAS_descriptorWrite.dstBinding = 0;
			TLAS_descriptorWrite.dstArrayElement = 0;
			TLAS_descriptorWrite.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
			TLAS_descriptorWrite.descriptorCount = 1;
			TLAS_descriptorWrite.pNext = &descriptorAccelerationStructureInfo;
			
			/////////////////////////////////////////////////////////////////////////////////////
			vk::DescriptorImageInfo PositionImageInfo{};
			PositionImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			PositionImageInfo.imageView   = gbuffer.Position.imageView;
			PositionImageInfo.sampler     = gbuffer.Position.imageSampler;

			vk::WriteDescriptorSet PositionSamplerdescriptorWrite{};
			PositionSamplerdescriptorWrite.dstSet = RayTracingDescriptorSets[i];
			PositionSamplerdescriptorWrite.dstBinding = 1;
			PositionSamplerdescriptorWrite.dstArrayElement = 0;
			PositionSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			PositionSamplerdescriptorWrite.descriptorCount = 1;
			PositionSamplerdescriptorWrite.pImageInfo = &PositionImageInfo;
			/////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorImageInfo NormalimageInfo{};
			NormalimageInfo.imageLayout = vk::ImageLayout::eGeneral;
			NormalimageInfo.imageView   = gbuffer.Normal.imageView;
			NormalimageInfo.sampler     = gbuffer.Normal.imageSampler;

			vk::WriteDescriptorSet NormalSamplerdescriptorWrite{};
			NormalSamplerdescriptorWrite.dstSet = RayTracingDescriptorSets[i];
			NormalSamplerdescriptorWrite.dstBinding = 2;
			NormalSamplerdescriptorWrite.dstArrayElement = 0;
			NormalSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			NormalSamplerdescriptorWrite.descriptorCount = 1;
			NormalSamplerdescriptorWrite.pImageInfo = &NormalimageInfo;
			/////////////////////////////////////////////////////////////////////////////////////

		    vk::DescriptorImageInfo StoreageImageInfo{};
		    StoreageImageInfo.imageLayout = vk::ImageLayout::eGeneral;
		    StoreageImageInfo.imageView = RT_ReflectionPassImage.imageView;
		    StoreageImageInfo.sampler = RT_ReflectionPassImage.imageSampler;

			vk::WriteDescriptorSet StoreageImagSamplerdescriptorWrite{};
			StoreageImagSamplerdescriptorWrite.dstSet = RayTracingDescriptorSets[i];
			StoreageImagSamplerdescriptorWrite.dstBinding = 3;
			StoreageImagSamplerdescriptorWrite.dstArrayElement = 0;
			StoreageImagSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			StoreageImagSamplerdescriptorWrite.descriptorCount = 1;
			StoreageImagSamplerdescriptorWrite.pImageInfo = &StoreageImageInfo;



			std::array<vk::WriteDescriptorSet, 4> descriptorWrites{ TLAS_descriptorWrite,
																	PositionSamplerdescriptorWrite,
				                                                    NormalSamplerdescriptorWrite,
				                                                    StoreageImagSamplerdescriptorWrite};

			vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}

}


void RT_Reflections::UpdateUniformBuffer(uint32_t currentImage, std::vector<std::shared_ptr<Light>>& lightref)
{

}


uint32_t RT_Reflections::alignedSize(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}


void RT_Reflections::Draw(BufferData RayGenBuffer, BufferData RayHitBuffer, BufferData RayMisBuffer, vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
{
	vk::BufferDeviceAddressInfo raygenShaderBindingTableDeviceAdressesInfo;
	raygenShaderBindingTableDeviceAdressesInfo.buffer = RayGenBuffer.buffer;

	vk::BufferDeviceAddressInfo missShaderBindingTableDeviceAdressesInfo;
	missShaderBindingTableDeviceAdressesInfo.buffer = RayMisBuffer.buffer;

	vk::BufferDeviceAddressInfo hitShaderBindingTableDeviceAdressesInfo;
	hitShaderBindingTableDeviceAdressesInfo.buffer = RayHitBuffer.buffer;

	auto raygenShaderBindingTableAdress = vulkanContext->LogicalDevice.getBufferAddress(raygenShaderBindingTableDeviceAdressesInfo);
	auto missShaderBindingTableAdress   = vulkanContext->LogicalDevice.getBufferAddress(missShaderBindingTableDeviceAdressesInfo);
	auto hitShaderBindingTableAdress    = vulkanContext->LogicalDevice.getBufferAddress(hitShaderBindingTableDeviceAdressesInfo);


	const uint32_t handleSizeAligned = alignedSize(
		vulkanContext->RayTracingPipelineProperties.shaderGroupHandleSize,
		vulkanContext->RayTracingPipelineProperties.shaderGroupHandleAlignment);

	vk::StridedDeviceAddressRegionKHR    raygenShaderSbtEntry{};
	raygenShaderSbtEntry.deviceAddress = raygenShaderBindingTableAdress;
	raygenShaderSbtEntry.stride = handleSizeAligned;
	raygenShaderSbtEntry.size = handleSizeAligned;


	vk::StridedDeviceAddressRegionKHR  missShaderSbtEntry{};
	missShaderSbtEntry.deviceAddress = missShaderBindingTableAdress;
	missShaderSbtEntry.stride = handleSizeAligned;
	missShaderSbtEntry.size = handleSizeAligned;

	vk::StridedDeviceAddressRegionKHR hitShaderSbtEntry{};
	hitShaderSbtEntry.deviceAddress = hitShaderBindingTableAdress;
	hitShaderSbtEntry.stride = handleSizeAligned;
	hitShaderSbtEntry.size = handleSizeAligned;


	vk::StridedDeviceAddressRegionKHR callableShaderSbtEntry{};

	VkStridedDeviceAddressRegionKHR  TEMP_raygenShaderSbtEntry   = static_cast<VkStridedDeviceAddressRegionKHR>(raygenShaderSbtEntry);
	VkStridedDeviceAddressRegionKHR  TEMP_missShaderSbtEntry     = static_cast<VkStridedDeviceAddressRegionKHR>(missShaderSbtEntry);;
	VkStridedDeviceAddressRegionKHR  TEMP_hitShaderSbtEntry      = static_cast<VkStridedDeviceAddressRegionKHR>(hitShaderSbtEntry);;
	VkStridedDeviceAddressRegionKHR  TEMP_callableShaderSbtEntry = static_cast<VkStridedDeviceAddressRegionKHR>(callableShaderSbtEntry);;

	int width  = swapchainextent.width;
	int height = swapchainextent.height;
	int depth  = 1;
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, pipelinelayout, 0, 1, &RayTracingDescriptorSets[imageIndex],0,nullptr);
	
	vulkanContext->vkCmdTraceRaysKHR(
		commandbuffer,
		&TEMP_raygenShaderSbtEntry,
		&TEMP_missShaderSbtEntry,
		&TEMP_hitShaderSbtEntry,
		&TEMP_callableShaderSbtEntry,
		width,
		height,
		depth);
}


void RT_Reflections::CleanUp()
{
	if (bufferManager)
	{
		for (auto& RayGen_Buffer : RayGen_UniformBuffers)
		{
			if (RayGen_Buffer.buffer)
			{
				bufferManager->UnmapMemory(RayGen_Buffer);
				bufferManager->DestroyBuffer(RayGen_Buffer);
			}
		}

		for (auto& RayClosestHitBuffer : RayClosestHit_UniformBuffers)
		{
			if (RayClosestHitBuffer.buffer)
			{
				bufferManager->UnmapMemory(RayClosestHitBuffer);
				bufferManager->DestroyBuffer(RayClosestHitBuffer);
			}
		}

		vulkanContext->LogicalDevice.destroyDescriptorSetLayout(RayTracingDescriptorSetLayout);
		RayGen_UniformBuffers.clear();
		RayClosestHit_UniformBuffers.clear();
		RayGen_UniformBuffersMappedMem.clear();
		RayClosestHit_UniformBuffersMappedMem.clear();

	}

}



