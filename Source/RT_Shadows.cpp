#include "RayTracing.h"
#include "VulkanContext.h"
#include "BufferManager.h"
#include "Camera.h"
#include "Light.h"

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
		RayGen_UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		RayGen_UniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

		VkDeviceSize	RayGenuniformBufferSize = sizeof(RayGen_UniformBufferData);

		for (size_t i = 0; i < RayGen_UniformBuffers.size(); i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "RayGen Uniform Buffer" + i;
			bufferManager->CreateBuffer(&bufferdata, RayGenuniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
			RayGen_UniformBuffers[i] = bufferdata;

			RayGen_UniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
		}
	}



	{
		RayClosestHit_UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		RayClosestHit_UniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

		VkDeviceSize RayClosesetHitUniformBufferSize = sizeof(RayClosesetHit_UniformBufferData) * 100;

		for (size_t i = 0; i < RayGen_UniformBuffers.size(); i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "RayClosesetHit Uniform Buffer" + i;
			bufferManager->CreateBuffer(&bufferdata, RayClosesetHitUniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
			RayClosestHit_UniformBuffers[i] = bufferdata;

			RayClosestHit_UniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
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

	vk::DescriptorSetLayoutBinding RayGenUniformBufferLayout{};
	RayGenUniformBufferLayout.binding = 4;
	RayGenUniformBufferLayout.descriptorCount = 1;
	RayGenUniformBufferLayout.descriptorType = vk::DescriptorType::eUniformBuffer;
	RayGenUniformBufferLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

	vk::DescriptorSetLayoutBinding RayClosestHitUniformBufferLayout{};
	RayClosestHitUniformBufferLayout.binding = 5;
	RayClosestHitUniformBufferLayout.descriptorCount = 1;
	RayClosestHitUniformBufferLayout.descriptorType = vk::DescriptorType::eUniformBuffer;
	RayClosestHitUniformBufferLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;


	std::array<vk::DescriptorSetLayoutBinding, 6> bindings = { TLASLayout,PositionSamplerLayout, 
		                                                       NormalSamplerLayout,ShadowResultSamplerLayout,
		                                                       RayGenUniformBufferLayout,RayClosestHitUniformBufferLayout };

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
			rayuniformbufferInfo.buffer = RayGen_UniformBuffers[i].buffer;
			rayuniformbufferInfo.offset = 0;
			rayuniformbufferInfo.range = sizeof(RayGen_UniformBufferData);

			vk::WriteDescriptorSet RayUniformdescriptorWrite{};
			RayUniformdescriptorWrite.dstSet = RayTracingDescriptorSets[i];
			RayUniformdescriptorWrite.dstBinding = 4;
			RayUniformdescriptorWrite.dstArrayElement = 0;
			RayUniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
			RayUniformdescriptorWrite.descriptorCount = 1;
			RayUniformdescriptorWrite.pBufferInfo = &rayuniformbufferInfo;


			vk::DescriptorBufferInfo RayClosesetHit_UniformbufferInfo{};
			RayClosesetHit_UniformbufferInfo.buffer = RayClosestHit_UniformBuffers[i].buffer;
			RayClosesetHit_UniformbufferInfo.offset = 0;
			RayClosesetHit_UniformbufferInfo.range = sizeof(RayClosesetHit_UniformBufferData) * 100;

			vk::WriteDescriptorSet RayClosesetHit_UniformdescriptorWrite{};
			RayClosesetHit_UniformdescriptorWrite.dstSet = RayTracingDescriptorSets[i];
			RayClosesetHit_UniformdescriptorWrite.dstBinding = 5;
			RayClosesetHit_UniformdescriptorWrite.dstArrayElement = 0;
			RayClosesetHit_UniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
			RayClosesetHit_UniformdescriptorWrite.descriptorCount = 1;
			RayClosesetHit_UniformdescriptorWrite.pBufferInfo = &RayClosesetHit_UniformbufferInfo;



			std::array<vk::WriteDescriptorSet, 6> descriptorWrites{ TLAS_descriptorWrite,
																	PositionSamplerdescriptorWrite,
				                                                    NormalSamplerdescriptorWrite,
				                                                    StoreageImagSamplerdescriptorWrite,
			                                                        RayUniformdescriptorWrite,RayClosesetHit_UniformdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}

}

void RayTracing::UpdateUniformBuffer(uint32_t currentImage, std::vector<std::shared_ptr<Light>>& lightref)
{
	RayGen_UniformBufferData RayGent_UniformBufferData;
	RayGent_UniformBufferData.ViewMatrix = glm::inverse(camera->GetViewMatrix());
	RayGent_UniformBufferData.ProjectionMatrix = glm::inverse(camera->GetProjectionMatrix());
	RayGent_UniformBufferData.ProjectionMatrix[1][1] *= -1;

	RayGent_UniformBufferData.LightCount_Padding = glm::vec4(lightref.size(), 0.0f, 0.0f, 0.0f);

	memcpy(RayGen_UniformBuffersMappedMem[currentImage], &RayGent_UniformBufferData, sizeof(RayGent_UniformBufferData));

	std::vector<RayClosesetHit_UniformBufferData> LightsData;

	
	for (size_t i = 0; i < lightref.size(); i++)
	{
		RayClosesetHit_UniformBufferData lightInstancedata;
		lightInstancedata.LightPosition_Padding = glm::vec4(lightref[i]->position, 0.0f);
		lightInstancedata.LightType_Padding = glm::vec4(lightref[i]->lightType, 0.0f, 0.0f, 0.0f);

		LightsData.push_back(lightInstancedata);
	}

	memcpy(RayClosestHit_UniformBuffersMappedMem[currentImage], LightsData.data(), LightsData.size() * sizeof(RayClosesetHit_UniformBufferData));

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

	//vk::StridedDeviceAddressRegionKHR hitShaderSbtEntry{};

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



