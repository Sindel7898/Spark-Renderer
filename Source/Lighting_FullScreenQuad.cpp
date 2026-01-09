#include "Lighting_FullScreenQuad.h"
#include <stdexcept>
#include <chrono>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include "Light.h"
#include "Camera.h"
#include "RT_Shadows.h"

Lighting_FullScreenQuad::Lighting_FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext,Camera* cameraref, vk::CommandPool commandpool, SkyBox* skyboxref)
{
	camera = cameraref;
	bufferManager = buffermanager;
	vulkanContext = vulkancontext;
	commandPool   = commandpool;
	SkyBoxRef = skyboxref;
	CreateUniformBuffer();
	createDescriptorSetLayout();
}


void Lighting_FullScreenQuad::CreateUniformBuffer()
{
	//////////////////////////////////////////////////////////////
	fragmentUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	FragmentUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

	VkDeviceSize FragmentuniformBufferSize = sizeof(LightUniformData) * 100;

	for (size_t i = 0; i < fragmentUniformBuffers.size(); i++)
	{
		BufferData bufferdata;

		bufferdata.BufferID = " Lighting FullScreen Quad Fragment Uniform Buffer" + i;
		bufferManager->CreateBuffer(&bufferdata,FragmentuniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
		fragmentUniformBuffers[i] = bufferdata;

		FragmentUniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
	}
}

void Lighting_FullScreenQuad::CreateStorageImage() {

    swapchainextent = vk::Extent3D(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, 1);

	ResultingStorageImage.ImageID = " Lighting with rt Pass Image";
	bufferManager->CreateImage(&ResultingStorageImage, swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, false);
	ResultingStorageImage.imageView = bufferManager->CreateImageView(&ResultingStorageImage, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	ResultingStorageImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);
}

void Lighting_FullScreenQuad::DestroyStorageImage() {

	bufferManager->DestroyImage(ResultingStorageImage);


}

void Lighting_FullScreenQuad::createDescriptorSetLayout()
{
	{
		//////// Create set for Final Lighting Pass ////////////
		vk::DescriptorSetLayoutBinding PositionSamplerLayout{};
		PositionSamplerLayout.binding = 0;
		PositionSamplerLayout.descriptorCount = 1;
		PositionSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		PositionSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding NormalSamplerLayout{};
		NormalSamplerLayout.binding = 1;
		NormalSamplerLayout.descriptorCount = 1;
		NormalSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		NormalSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding AlbedoSamplerLayout{};
		AlbedoSamplerLayout.binding = 2;
		AlbedoSamplerLayout.descriptorCount = 1;
		AlbedoSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		AlbedoSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding MaterialsSamplerLayout{};
		MaterialsSamplerLayout.binding = 3;
		MaterialsSamplerLayout.descriptorCount = 1;
		MaterialsSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		MaterialsSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding ReflectiveCubeSamplerLayout{};
		ReflectiveCubeSamplerLayout.binding = 4;
		ReflectiveCubeSamplerLayout.descriptorCount = 1;
		ReflectiveCubeSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		ReflectiveCubeSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding EmisiveSamplerLayout{};
		EmisiveSamplerLayout.binding = 5;
		EmisiveSamplerLayout.descriptorCount = 1;
		EmisiveSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		EmisiveSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding ResultingImageLayout{};
		ResultingImageLayout.binding = 6;
		ResultingImageLayout.descriptorCount = 1;
		ResultingImageLayout.descriptorType = vk::DescriptorType::eStorageImage;
		ResultingImageLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding LightUniformBufferLayout{};
		LightUniformBufferLayout.binding = 7;
		LightUniformBufferLayout.descriptorCount = 1;
		LightUniformBufferLayout.descriptorType = vk::DescriptorType::eUniformBuffer;
		LightUniformBufferLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding TLASLayout{};
		TLASLayout.binding = 8;
		TLASLayout.descriptorCount = 1;
		TLASLayout.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
		TLASLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;


		std::array<vk::DescriptorSetLayoutBinding, 9> bindings = { PositionSamplerLayout,
																   NormalSamplerLayout,          
																   AlbedoSamplerLayout,          
			                                                       MaterialsSamplerLayout,
																   ReflectiveCubeSamplerLayout,
			                                                       EmisiveSamplerLayout,
																   ResultingImageLayout,
																   LightUniformBufferLayout,TLASLayout
		};

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
	}

}

void Lighting_FullScreenQuad::createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, GBuffer* Gbuffer, vk::AccelerationStructureKHR* TLAS)
{
	TLASr = TLAS;

	GbufferRef = Gbuffer;
	// create sets from the pool based on the layout
	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocinfo;
	allocinfo.descriptorPool = descriptorpool;
	allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocinfo.pSetLayouts = layouts.data();

	DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, DescriptorSets.data());

	UpdateDescrptorSets();
}

void Lighting_FullScreenQuad::UpdateDescrptorSets()
{
	//specifies what exactly to send
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		/////////////////////////////////////////////////////////////////////////////////////
		vk::DescriptorImageInfo PositionimageInfo{};
		PositionimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		PositionimageInfo.imageView = GbufferRef->Position.imageView;
		PositionimageInfo.sampler = GbufferRef->Position.imageSampler;

		vk::WriteDescriptorSet PositionSamplerdescriptorWrite{};
		PositionSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		PositionSamplerdescriptorWrite.dstBinding = 0;
		PositionSamplerdescriptorWrite.dstArrayElement = 0;
		PositionSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		PositionSamplerdescriptorWrite.descriptorCount = 1;
		PositionSamplerdescriptorWrite.pImageInfo = &PositionimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////
		;
		vk::DescriptorImageInfo NormalimageInfo{};
		NormalimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		NormalimageInfo.imageView = GbufferRef->Normal.imageView;
		NormalimageInfo.sampler = GbufferRef->Normal.imageSampler;

		vk::WriteDescriptorSet NormalSamplerdescriptorWrite{};
		NormalSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		NormalSamplerdescriptorWrite.dstBinding = 1;
		NormalSamplerdescriptorWrite.dstArrayElement = 0;
		NormalSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		NormalSamplerdescriptorWrite.descriptorCount = 1;
		NormalSamplerdescriptorWrite.pImageInfo = &NormalimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////

		vk::DescriptorImageInfo AlbedoimageInfo{};
		AlbedoimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		AlbedoimageInfo.imageView = GbufferRef->Albedo.imageView;
		AlbedoimageInfo.sampler = GbufferRef->Albedo.imageSampler;

		vk::WriteDescriptorSet  AlbedoSamplerdescriptorWrite{};
		AlbedoSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		AlbedoSamplerdescriptorWrite.dstBinding = 2;
		AlbedoSamplerdescriptorWrite.dstArrayElement = 0;
		AlbedoSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		AlbedoSamplerdescriptorWrite.descriptorCount = 1;
		AlbedoSamplerdescriptorWrite.pImageInfo = &AlbedoimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////

		vk::DescriptorImageInfo MaterialsimageInfo{};
		MaterialsimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		MaterialsimageInfo.imageView = GbufferRef->Materials.imageView;
		MaterialsimageInfo.sampler = GbufferRef->Materials.imageSampler;

		vk::WriteDescriptorSet MaterialsSamplerdescriptorWrite{};
		MaterialsSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		MaterialsSamplerdescriptorWrite.dstBinding = 3;
		MaterialsSamplerdescriptorWrite.dstArrayElement = 0;
		MaterialsSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		MaterialsSamplerdescriptorWrite.descriptorCount = 1;
		MaterialsSamplerdescriptorWrite.pImageInfo = &MaterialsimageInfo;


		vk::DescriptorImageInfo ReflectiveCubeimageInfo{};
		ReflectiveCubeimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		ReflectiveCubeimageInfo.imageView = SkyBoxRef->SkyBoxImages[SkyBoxRef->SkyBoxIndex].imageView;
		ReflectiveCubeimageInfo.sampler   = SkyBoxRef->SkyBoxImages[SkyBoxRef->SkyBoxIndex].imageSampler;

		vk::WriteDescriptorSet ReflectiveCubeSamplerdescriptorWrite{};
		ReflectiveCubeSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		ReflectiveCubeSamplerdescriptorWrite.dstBinding = 4;
		ReflectiveCubeSamplerdescriptorWrite.dstArrayElement = 0;
		ReflectiveCubeSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		ReflectiveCubeSamplerdescriptorWrite.descriptorCount = 1;
		ReflectiveCubeSamplerdescriptorWrite.pImageInfo = &ReflectiveCubeimageInfo;

		vk::DescriptorImageInfo EmisiveimageInfo{};
		EmisiveimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		EmisiveimageInfo.imageView = GbufferRef->Emissive.imageView;
		EmisiveimageInfo.sampler = GbufferRef->Emissive.imageSampler;

		vk::WriteDescriptorSet EmisiveSamplerdescriptorWrite{};
		EmisiveSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		EmisiveSamplerdescriptorWrite.dstBinding = 5;
		EmisiveSamplerdescriptorWrite.dstArrayElement = 0;
		EmisiveSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		EmisiveSamplerdescriptorWrite.descriptorCount = 1;
		EmisiveSamplerdescriptorWrite.pImageInfo = &EmisiveimageInfo;

		vk::DescriptorImageInfo ResultingimageInfo{};
		ResultingimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		ResultingimageInfo.imageView = ResultingStorageImage.imageView;
		ResultingimageInfo.sampler = ResultingStorageImage.imageSampler;

		vk::WriteDescriptorSet ResultingdescriptorWrite{};
		ResultingdescriptorWrite.dstSet = DescriptorSets[i];
		ResultingdescriptorWrite.dstBinding = 6;
		ResultingdescriptorWrite.dstArrayElement = 0;
		ResultingdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
		ResultingdescriptorWrite.descriptorCount = 1;
		ResultingdescriptorWrite.pImageInfo = &ResultingimageInfo;

		/////////////////////////////////////////////////////////////////////////////////////

		vk::DescriptorBufferInfo LightUniformBufferInfo;
		LightUniformBufferInfo.buffer = fragmentUniformBuffers[i].buffer;
		LightUniformBufferInfo.offset = 0;
		LightUniformBufferInfo.range = sizeof(LightUniformData) * 100;

		vk::WriteDescriptorSet LightUniformBufferDescriptorWrite{};
		LightUniformBufferDescriptorWrite.dstSet = DescriptorSets[i];
		LightUniformBufferDescriptorWrite.dstBinding = 7;
		LightUniformBufferDescriptorWrite.dstArrayElement = 0;
		LightUniformBufferDescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		LightUniformBufferDescriptorWrite.descriptorCount = 1;
		LightUniformBufferDescriptorWrite.pBufferInfo = &LightUniformBufferInfo;

		vk::WriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
		descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
		descriptorAccelerationStructureInfo.pAccelerationStructures = TLASr;

		vk::WriteDescriptorSet TLAS_descriptorWrite{};
		TLAS_descriptorWrite.dstSet = DescriptorSets[i];
		TLAS_descriptorWrite.dstBinding = 8;
		TLAS_descriptorWrite.dstArrayElement = 0;
		TLAS_descriptorWrite.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
		TLAS_descriptorWrite.descriptorCount = 1;
		TLAS_descriptorWrite.pNext = &descriptorAccelerationStructureInfo;


		std::array<vk::WriteDescriptorSet, 9> descriptorWrites = {
																	PositionSamplerdescriptorWrite,      
																	NormalSamplerdescriptorWrite,        
																	AlbedoSamplerdescriptorWrite,        
																	MaterialsSamplerdescriptorWrite,
																	ReflectiveCubeSamplerdescriptorWrite,
																	EmisiveSamplerdescriptorWrite,ResultingdescriptorWrite,
																	LightUniformBufferDescriptorWrite,TLAS_descriptorWrite
		};

		vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}

void Lighting_FullScreenQuad::UpdateUniformBuffer(uint32_t currentImage, std::vector<std::shared_ptr<Light>>& lightref)
{
	if (SkyBoxRef->bSkyBoxUpdate)
	{
		UpdateDescrptorSets();
		SkyBoxRef->bSkyBoxUpdate = false;
	}

	std::vector<LightUniformData> lightDataspack;
	lightDataspack.reserve(lightref.size());

	for (int  i = 0; i < lightref.size(); i++)
	{
		if (lightref[i])
		{
			LightUniformData LightData;
			LightData.lightPositionAndLightType = glm::vec4(lightref[i]->position,lightref[i]->lightType);
			LightData.colorAndAmbientStrength   = glm::vec4(lightref[i]->color, lightref[i]->ambientStrength);
			LightData.CameraPositionAndLightIntensity = glm::vec4(camera->GetPosition().x, 
				                                                  camera->GetPosition().y,
				                                                  camera->GetPosition().z, 
				                                                  lightref[i]->lightIntensity);
			lightDataspack.push_back(LightData);
		}
	}

	LightCount = lightDataspack.size();
	memcpy(FragmentUniformBuffersMappedMem[currentImage], lightDataspack.data(), lightDataspack.size() * sizeof(LightUniformData));

}

uint32_t Lighting_FullScreenQuad::alignedSize(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

void Lighting_FullScreenQuad::Draw(BufferData RayGenBuffer, BufferData RayHitBuffer, BufferData RayMisBuffer, vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
{
	vk::BufferDeviceAddressInfo raygenShaderBindingTableDeviceAdressesInfo;
	raygenShaderBindingTableDeviceAdressesInfo.buffer = RayGenBuffer.buffer;

	vk::BufferDeviceAddressInfo missShaderBindingTableDeviceAdressesInfo;
	missShaderBindingTableDeviceAdressesInfo.buffer = RayMisBuffer.buffer;

	vk::BufferDeviceAddressInfo hitShaderBindingTableDeviceAdressesInfo;
	hitShaderBindingTableDeviceAdressesInfo.buffer = RayHitBuffer.buffer;

	auto raygenShaderBindingTableAdress = vulkanContext->LogicalDevice.getBufferAddress(raygenShaderBindingTableDeviceAdressesInfo);
	auto missShaderBindingTableAdress = vulkanContext->LogicalDevice.getBufferAddress(missShaderBindingTableDeviceAdressesInfo);
	auto hitShaderBindingTableAdress = vulkanContext->LogicalDevice.getBufferAddress(hitShaderBindingTableDeviceAdressesInfo);


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

	VkStridedDeviceAddressRegionKHR  TEMP_raygenShaderSbtEntry = static_cast<VkStridedDeviceAddressRegionKHR>(raygenShaderSbtEntry);
	VkStridedDeviceAddressRegionKHR  TEMP_missShaderSbtEntry = static_cast<VkStridedDeviceAddressRegionKHR>(missShaderSbtEntry);;
	VkStridedDeviceAddressRegionKHR  TEMP_hitShaderSbtEntry = static_cast<VkStridedDeviceAddressRegionKHR>(hitShaderSbtEntry);;
	VkStridedDeviceAddressRegionKHR  TEMP_callableShaderSbtEntry = static_cast<VkStridedDeviceAddressRegionKHR>(callableShaderSbtEntry);;

	int width  = vulkanContext->swapchainExtent.width;
	int height = vulkanContext->swapchainExtent.height;
	int depth = 1;


	commandbuffer.pushConstants(pipelinelayout, vk::ShaderStageFlagBits::eRaygenKHR, 0, sizeof(int), &LightCount);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);

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

void Lighting_FullScreenQuad::CleanUp()
{
	if (bufferManager)
	{
		for (auto& RayGen_Buffer : fragmentUniformBuffers)
		{
			if (RayGen_Buffer.buffer)
			{
				bufferManager->UnmapMemory(RayGen_Buffer);
				bufferManager->DestroyBuffer(RayGen_Buffer);
			}
		}

		vulkanContext->LogicalDevice.destroyDescriptorSetLayout(descriptorSetLayout);

		fragmentUniformBuffers.clear();
	}
	
}

