#include "CombinedResult_FullScreenQuad.h"
#include <stdexcept>
#include <chrono>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include "Light.h"
#include "Camera.h"
#include <random>

CombinedResult_FullScreenQuad::CombinedResult_FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext,Camera* cameraref, vk::CommandPool commandpool): Drawable()
{
	camera = cameraref;
	bufferManager = buffermanager;
	vulkanContext = vulkancontext;
	commandPool   = commandpool;
	CreateVertexAndIndexBuffer();
	CreateUniformBuffer();
	createDescriptorSetLayout();

}


void CombinedResult_FullScreenQuad::CreateVertexAndIndexBuffer()
{

	VkDeviceSize VertexBufferSize = sizeof(quad[0]) * quad.size();
	vertexBufferData.BufferID = "SSAO_BLUR Vertex Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&vertexBufferData,quad.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(uint16_t) * quadIndices.size();
	indexBufferData.BufferID = "SSAO_BLUR Index Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&indexBufferData,quadIndices.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer, commandPool, vulkanContext->graphicsQueue);

}

void CombinedResult_FullScreenQuad::CreateImage(vk::Extent3D imageExtent)
{
	FinalResultImage.ImageID = "FinalResult  Image  Texture";
	bufferManager->CreateImage(&FinalResultImage, imageExtent, vulkanContext->swapchainformat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

	FinalResultImage.imageView = bufferManager->CreateImageView(&FinalResultImage, vulkanContext->swapchainformat, vk::ImageAspectFlagBits::eColor);
	FinalResultImage.imageSampler = bufferManager->CreateImageSampler();
}

void CombinedResult_FullScreenQuad::DestroyImage()
{

	bufferManager->DestroyImage(FinalResultImage);
}


void CombinedResult_FullScreenQuad::createDescriptorSetLayout()
{
	{
		//////// Create set for SSAO Pass ////////////
		vk::DescriptorSetLayoutBinding LightingResultDescriptorBinding{};
		LightingResultDescriptorBinding.binding = 0;
		LightingResultDescriptorBinding.descriptorCount = 1;
		LightingResultDescriptorBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		LightingResultDescriptorBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding SSGIDescriptorBinding{};
		SSGIDescriptorBinding.binding = 1;
		SSGIDescriptorBinding.descriptorCount = 1;
		SSGIDescriptorBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		SSGIDescriptorBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding SSAODescriptorBinding{};
		SSAODescriptorBinding.binding = 2;
		SSAODescriptorBinding.descriptorCount = 1;
		SSAODescriptorBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		SSAODescriptorBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding MaterialsDescriptorBinding{};
		MaterialsDescriptorBinding.binding = 3;
		MaterialsDescriptorBinding.descriptorCount = 1;
		MaterialsDescriptorBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		MaterialsDescriptorBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding AlbedoDescriptorBinding{};
		AlbedoDescriptorBinding.binding = 4;
		AlbedoDescriptorBinding.descriptorCount = 1;
		AlbedoDescriptorBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		AlbedoDescriptorBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		std::array<vk::DescriptorSetLayoutBinding, 5> ImageResultPassBinding = { LightingResultDescriptorBinding,SSGIDescriptorBinding,SSAODescriptorBinding,MaterialsDescriptorBinding,AlbedoDescriptorBinding };

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(ImageResultPassBinding.size());
		layoutInfo.pBindings = ImageResultPassBinding.data();

		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
	}
}


void CombinedResult_FullScreenQuad::UpdataeUniformBufferData()
{
}


void CombinedResult_FullScreenQuad::createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, ImageData LightingResultImage, ImageData SSGIImage, ImageData SSAOIImage, ImageData MaterialImage, ImageData AlbedoImage)
{
	// create sets from the pool based on the layout
	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocinfo;
	allocinfo.descriptorPool = descriptorpool;
	allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocinfo.pSetLayouts = layouts.data();
	 
	DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, DescriptorSets.data());

	////////////////////////////////////////////////////////////////////////////////////////////////
	//specifies what exactly to send
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		/////////////////////////////////////////////////////////////////////////////////////
		vk::DescriptorImageInfo LightingResultimageInfo{};
		LightingResultimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		LightingResultimageInfo.imageView   = LightingResultImage.imageView;
		LightingResultimageInfo.sampler    = LightingResultImage.imageSampler;

		vk::WriteDescriptorSet LightingResultSamplerdescriptorWrite{};
		LightingResultSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		LightingResultSamplerdescriptorWrite.dstBinding = 0;
		LightingResultSamplerdescriptorWrite.descriptorCount = 1;
		LightingResultSamplerdescriptorWrite.dstArrayElement = 0;
		LightingResultSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		LightingResultSamplerdescriptorWrite.pImageInfo = &LightingResultimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////
;
        vk::DescriptorImageInfo SSGIImageResultimageInfo{};
		SSGIImageResultimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		SSGIImageResultimageInfo.imageView   = SSGIImage.imageView;
		SSGIImageResultimageInfo.sampler     = SSGIImage.imageSampler;
        
        vk::WriteDescriptorSet SSGISamplerdescriptorWrite{};
		SSGISamplerdescriptorWrite.dstSet = DescriptorSets[i];
		SSGISamplerdescriptorWrite.dstBinding = 1;
		SSGISamplerdescriptorWrite.descriptorCount = 1;
		SSGISamplerdescriptorWrite.dstArrayElement = 0;
		SSGISamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		SSGISamplerdescriptorWrite.pImageInfo = &SSGIImageResultimageInfo;
        /////////////////////////////////////////////////////////////////////////////////////


		vk::DescriptorImageInfo SSAOImageResultimageInfo{};
		SSAOImageResultimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		SSAOImageResultimageInfo.imageView = SSAOIImage.imageView;
		SSAOImageResultimageInfo.sampler = SSAOIImage.imageSampler;

		vk::WriteDescriptorSet SSAOSamplerdescriptorWrite{};
		SSAOSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		SSAOSamplerdescriptorWrite.dstBinding = 2;
		SSAOSamplerdescriptorWrite.descriptorCount = 1;
		SSAOSamplerdescriptorWrite.dstArrayElement = 0;
		SSAOSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		SSAOSamplerdescriptorWrite.pImageInfo = &SSAOImageResultimageInfo;


		vk::DescriptorImageInfo MaterialsImageResultimageInfo{};
		MaterialsImageResultimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		MaterialsImageResultimageInfo.imageView = MaterialImage.imageView;
		MaterialsImageResultimageInfo.sampler = MaterialImage.imageSampler;

		vk::WriteDescriptorSet MaterialsSamplerdescriptorWrite{};
		MaterialsSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		MaterialsSamplerdescriptorWrite.dstBinding = 3;
		MaterialsSamplerdescriptorWrite.descriptorCount = 1;
		MaterialsSamplerdescriptorWrite.dstArrayElement = 0;
		MaterialsSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		MaterialsSamplerdescriptorWrite.pImageInfo = &MaterialsImageResultimageInfo;


		vk::DescriptorImageInfo AlbedoImageResultimageInfo{};
		AlbedoImageResultimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		AlbedoImageResultimageInfo.imageView = AlbedoImage.imageView;
		AlbedoImageResultimageInfo.sampler = AlbedoImage.imageSampler;

		vk::WriteDescriptorSet AlbedoSamplerdescriptorWrite{};
		AlbedoSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		AlbedoSamplerdescriptorWrite.dstBinding = 4;
		AlbedoSamplerdescriptorWrite.descriptorCount = 1;
		AlbedoSamplerdescriptorWrite.dstArrayElement = 0;
		AlbedoSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		AlbedoSamplerdescriptorWrite.pImageInfo = &AlbedoImageResultimageInfo;



		std::array<vk::WriteDescriptorSet, 5> descriptorWrites = { LightingResultSamplerdescriptorWrite,
																	SSGISamplerdescriptorWrite,SSAOSamplerdescriptorWrite,MaterialsSamplerdescriptorWrite,AlbedoSamplerdescriptorWrite };

		vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}


void CombinedResult_FullScreenQuad::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };

	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint16);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(quadIndices.size(), 1, 0, 0, 0);
}

void CombinedResult_FullScreenQuad::CleanUp()
{

	Drawable::Destructor();
}

