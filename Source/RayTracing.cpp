#include "RayTracing.h"
#include "VulkanContext.h"
#include "BufferManager.h"

#include <stdexcept>

RayTracing::RayTracing( VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger)
{
	bufferManager = buffermanger;
	vulkanContext = vulkancontext;
	camera        = rcamera;
	CreateUniformBuffer();
	CreateStorageImage();
}
void RayTracing::CreateUniformBuffer() {


}
void RayTracing::CreateStorageImage() {

	vk::Extent3D swapchainextent = vk::Extent3D(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, 1);

	bufferManager->CreateImage(&ShadowPassImage, swapchainextent, vulkanContext->swapchainformat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled);
	ShadowPassImage.imageView = bufferManager->CreateImageView(&ShadowPassImage, vulkanContext->swapchainformat, vk::ImageAspectFlagBits::eColor);
	ShadowPassImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);


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

	std::array<vk::DescriptorSetLayoutBinding, 4> bindings = { TLASLayout,PositionSamplerLayout, 
		                                                       NormalSamplerLayout,ShadowResultSamplerLayout };

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

		DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, DescriptorSets.data());

		////////////////////////////////////////////////////////////////////////////////////////////////
		//specifies what exactly to send
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			vk::WriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
			descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
			descriptorAccelerationStructureInfo.pAccelerationStructures = &TLAS;

			vk::WriteDescriptorSet TLAS_descriptorWrite{};
			TLAS_descriptorWrite.dstSet = DescriptorSets[i];
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
			PositionSamplerdescriptorWrite.dstSet = DescriptorSets[i];
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
			NormalSamplerdescriptorWrite.dstSet = DescriptorSets[i];
			NormalSamplerdescriptorWrite.dstBinding = 2;
			NormalSamplerdescriptorWrite.dstArrayElement = 0;
			NormalSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			NormalSamplerdescriptorWrite.descriptorCount = 1;
			NormalSamplerdescriptorWrite.pImageInfo = &NormalimageInfo;
			/////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorImageInfo StoreageImageInfo{};
			StoreageImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			StoreageImageInfo.imageView   = ShadowPassImage.imageView;
			StoreageImageInfo.sampler     = ShadowPassImage.imageSampler;

			vk::WriteDescriptorSet StoreageImagSamplerdescriptorWrite{};
			StoreageImagSamplerdescriptorWrite.dstSet = DescriptorSets[i];
			StoreageImagSamplerdescriptorWrite.dstBinding = 3;
			StoreageImagSamplerdescriptorWrite.dstArrayElement = 0;
			StoreageImagSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			StoreageImagSamplerdescriptorWrite.descriptorCount = 1;
			StoreageImagSamplerdescriptorWrite.pImageInfo = &StoreageImageInfo;

			/////////////////////////////////////////////////////////////////////////////////////


			std::array<vk::WriteDescriptorSet, 4> descriptorWrites{ TLAS_descriptorWrite,
																	PositionSamplerdescriptorWrite,
				                                                    NormalSamplerdescriptorWrite,
				                                                    StoreageImagSamplerdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}

}

void RayTracing::UpdateUniformBuffer(uint32_t currentImage)
{

}


void RayTracing::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{

}


void RayTracing::CleanUp()
{
	if (bufferManager)
	{
		bufferManager->DestroyImage(ShadowPassImage);

	}
}



