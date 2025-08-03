#include "RayTracing.h"
#include "VulkanContext.h"
#include "BufferManager.h"
#include "Camera.h"

#include <stdexcept>

RayTracing::RayTracing( VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger)
{
	bufferManager = buffermanger;
	vulkanContext = vulkancontext;
	camera        = rcamera;
	commandPool   = commandpool;
	CreateUniformBuffer();
	createRayTracingDescriptorSetLayout();
}
void RayTracing::CreateUniformBuffer() {

	{
		UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		UniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

		VkDeviceSize	RayuniformBufferSize = sizeof(RayUniformBufferData);

		for (size_t i = 0; i < UniformBuffers.size(); i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "Ray Uniform Buffer" + i;
			bufferManager->CreateBuffer(&bufferdata, RayuniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
			UniformBuffers[i] = bufferdata;

			UniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
		}
	}

}
void RayTracing::CreateStorageImage() {

	vk::Extent3D swapchainextent = vk::Extent3D(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, 1);

	ShadowPassImage.ImageID = "RT Shadow Pass Image";
	bufferManager->CreateImage(&ShadowPassImage, swapchainextent, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled);
	ShadowPassImage.imageView = bufferManager->CreateImageView(&ShadowPassImage, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
	ShadowPassImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

}
void RayTracing::DestroyStorageImage() {

	bufferManager->DestroyImage(ShadowPassImage);


}

void RayTracing::createRayTracingDescriptorSetLayout(){

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

	vk::DescriptorSetLayoutBinding ShadowResultSamplerLayout{};
	ShadowResultSamplerLayout.binding = 3;
	ShadowResultSamplerLayout.descriptorCount = 1;
	ShadowResultSamplerLayout.descriptorType = vk::DescriptorType::eStorageImage;
	ShadowResultSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;


	vk::DescriptorSetLayoutBinding RayUniformBufferLayout{};
	RayUniformBufferLayout.binding = 4;
	RayUniformBufferLayout.descriptorCount = 1;
	RayUniformBufferLayout.descriptorType = vk::DescriptorType::eUniformBuffer;
	RayUniformBufferLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;


	std::array<vk::DescriptorSetLayoutBinding, 5> bindings = { TLASLayout,PositionSamplerLayout, 
		                                                       NormalSamplerLayout,ShadowResultSamplerLayout,RayUniformBufferLayout };

	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();


	if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &RayTracingDescriptorSetLayout) != vk::Result::eSuccess)
	{
		throw std::runtime_error("Failed to create descriptorset layout!");
	}

}

void RayTracing::createRaytracedDescriptorSets(vk::DescriptorPool descriptorpool, vk::AccelerationStructureKHR TLAS,GBuffer gbuffer)
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
			PositionImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
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
			NormalimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
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
			StoreageImageInfo.imageView   = ShadowPassImage.imageView;
			StoreageImageInfo.sampler     = ShadowPassImage.imageSampler;

			vk::WriteDescriptorSet StoreageImagSamplerdescriptorWrite{};
			StoreageImagSamplerdescriptorWrite.dstSet = RayTracingDescriptorSets[i];
			StoreageImagSamplerdescriptorWrite.dstBinding = 3;
			StoreageImagSamplerdescriptorWrite.dstArrayElement = 0;
			StoreageImagSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			StoreageImagSamplerdescriptorWrite.descriptorCount = 1;
			StoreageImagSamplerdescriptorWrite.pImageInfo = &StoreageImageInfo;

			/////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorBufferInfo rayuniformbufferInfo{};
			rayuniformbufferInfo.buffer = UniformBuffers[i].buffer;
			rayuniformbufferInfo.offset = 0;
			rayuniformbufferInfo.range = sizeof(RayUniformBufferData);

			vk::WriteDescriptorSet RayUniformdescriptorWrite{};
			RayUniformdescriptorWrite.dstSet = RayTracingDescriptorSets[i];
			RayUniformdescriptorWrite.dstBinding = 4;
			RayUniformdescriptorWrite.dstArrayElement = 0;
			RayUniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
			RayUniformdescriptorWrite.descriptorCount = 1;
			RayUniformdescriptorWrite.pBufferInfo = &rayuniformbufferInfo;


			std::array<vk::WriteDescriptorSet, 5> descriptorWrites{ TLAS_descriptorWrite,
																	PositionSamplerdescriptorWrite,
				                                                    NormalSamplerdescriptorWrite,
				                                                    StoreageImagSamplerdescriptorWrite,
			                                                        RayUniformdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}

}

void RayTracing::UpdateUniformBuffer(uint32_t currentImage)
{
	RayUniformBufferData RayData;
	RayData.ViewMatrix = camera->GetViewMatrix();
	RayData.ProjectionMatrix = camera->GetProjectionMatrix();
	RayData.ProjectionMatrix[1][1] *= -1;
	RayData.DirectionalLightPosition_AndPadding = glm::vec4(0, -1, 0,0);

	memcpy(UniformBuffersMappedMem[currentImage], &RayData, sizeof(RayData));
}


uint32_t RayTracing::alignedSize(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}


void RayTracing::Draw(BufferData RayGenBuffer, BufferData RayHitBuffer, BufferData RayMisBuffer, vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
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

	int width  = vulkanContext->swapchainExtent.width;
	int height = vulkanContext->swapchainExtent.height;
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


void RayTracing::CleanUp()
{
	if (bufferManager)
	{
		bufferManager->DestroyImage(ShadowPassImage);

	}
}



