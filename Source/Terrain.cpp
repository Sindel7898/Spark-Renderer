#include "Terrain.h"
#include <stdexcept>
#include <chrono>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

Terrain::Terrain(const std::string filepath, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger)
	      : Drawable()
{

	FilePath = filepath;
	vulkanContext = vulkancontext;
	commandPool = commandpool;
	camera = rcamera;
	bufferManager = buffermanger;

	position = glm::vec3(0.0f,-3.02f,0.0f);
	rotation = glm::vec3(0.0f,0.0f,0.0f);
	scale =    glm::vec3(50, 0.0f, 50.0f);

	transformMatrices.modelMatrix = glm::mat4(1.0f);
	transformMatrices.modelMatrix = glm::translate(transformMatrices.modelMatrix, position);
	transformMatrices.modelMatrix = glm::rotate(transformMatrices.modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	transformMatrices.modelMatrix = glm::rotate(transformMatrices.modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	transformMatrices.modelMatrix = glm::rotate(transformMatrices.modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	transformMatrices.modelMatrix = glm::scale(transformMatrices.modelMatrix, scale);

	CreateVertexAndIndexBuffer();
	LoadTextures();
	CreateUniformBuffer();
	createDescriptorSetLayout();
}

void Terrain::LoadTextures()
{

	std::vector<StoredImageData> ModelTextures = AssetManager::GetInstance().GetStoredImageData(FilePath);


	StoredImageData AlbedoImageData = ModelTextures[0];
	vk::DeviceSize AlbedoImagesize = AlbedoImageData.imageWidth * AlbedoImageData.imageHeight * 4;

	albedoTextureData = bufferManager->CreateTextureImage(AlbedoImageData.imageData, AlbedoImagesize, AlbedoImageData.imageWidth, AlbedoImageData.imageHeight,vk::Format::eR8G8B8A8Srgb , commandPool, vulkanContext->graphicsQueue);

	StoredImageData NormalImageData = ModelTextures[1];
	vk::DeviceSize NormalImagesize = NormalImageData.imageWidth * NormalImageData.imageHeight * 4;

	normalTextureData = bufferManager->CreateTextureImage(NormalImageData.imageData, NormalImagesize, NormalImageData.imageWidth, NormalImageData.imageHeight, vk::Format::eR8G8B8A8Unorm, commandPool, vulkanContext->graphicsQueue);


	StoredImageData MetallicRoughnessImageData = ModelTextures[2];
	vk::DeviceSize  MetallicRoughnessImagesize = MetallicRoughnessImageData.imageWidth * MetallicRoughnessImageData.imageHeight * 4;

	MetallicRoughnessTextureData = bufferManager->CreateTextureImage(MetallicRoughnessImageData.imageData, MetallicRoughnessImagesize, MetallicRoughnessImageData.imageWidth, MetallicRoughnessImageData.imageHeight, vk::Format::eR8G8B8A8Unorm, commandPool, vulkanContext->graphicsQueue);


	StoredImageData HeighMapImageData = ModelTextures[3];
	vk::DeviceSize  HeighMapImageDataImagesize = HeighMapImageData.imageWidth * HeighMapImageData.imageHeight * 4;

	HeighMapTextureData = bufferManager->CreateTextureImage(HeighMapImageData.imageData, HeighMapImageDataImagesize, HeighMapImageData.imageWidth, HeighMapImageData.imageHeight, vk::Format::eR8G8B8A8Unorm, commandPool, vulkanContext->graphicsQueue);

}

void Terrain::CreateVertexAndIndexBuffer()
{
	storedModelData = &AssetManager::GetInstance().GetStoredModelData(FilePath);

	VkDeviceSize VertexBufferSize = sizeof(storedModelData->VertexData[0]) * storedModelData->VertexData.size();
	vertexBufferData.BufferID = "Terrain Vertex Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&vertexBufferData,storedModelData->VertexData.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(uint32_t) * storedModelData->IndexData.size();
	indexBufferData.BufferID = "Terrain Index Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&indexBufferData,storedModelData->IndexData.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer, commandPool, vulkanContext->graphicsQueue);

}


void Terrain::CreateUniformBuffer()
{
	{
		vertexUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		VertexUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

		VkDeviceSize VertexuniformBufferSize = sizeof(VertexUniformData);

		for (size_t i = 0; i < vertexUniformBuffers.size(); i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "Terrain Vertex Uniform Buffer" + i;
			bufferManager->CreateBuffer(&bufferdata,VertexuniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
			vertexUniformBuffers[i] = bufferdata;

			VertexUniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
		}
	}
}



void Terrain::createDescriptorSetLayout()
{
	vk::DescriptorSetLayoutBinding VertexUniformBufferBinding{};
	VertexUniformBufferBinding.binding = 0;
	VertexUniformBufferBinding.descriptorCount = 1;
	VertexUniformBufferBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	VertexUniformBufferBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

	vk::DescriptorSetLayoutBinding AlbedoSamplerLayout{};
	AlbedoSamplerLayout.binding = 1;
	AlbedoSamplerLayout.descriptorCount = 1;
	AlbedoSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	AlbedoSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutBinding NormalSamplerLayout{};
	NormalSamplerLayout.binding = 2;
	NormalSamplerLayout.descriptorCount = 1;
	NormalSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	NormalSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutBinding MetallicRoughnessSamplerLayout{};
	MetallicRoughnessSamplerLayout.binding = 3;
	MetallicRoughnessSamplerLayout.descriptorCount = 1;
	MetallicRoughnessSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	MetallicRoughnessSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutBinding HighMapSamplerLayout{};
	HighMapSamplerLayout.binding = 4;
	HighMapSamplerLayout.descriptorCount = 1;
	HighMapSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	HighMapSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;

	std::array<vk::DescriptorSetLayoutBinding, 5> bindings = { VertexUniformBufferBinding,
															   AlbedoSamplerLayout,NormalSamplerLayout,MetallicRoughnessSamplerLayout,HighMapSamplerLayout };

	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();


	if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess)
	{
		throw std::runtime_error("Failed to create descriptorset layout!");
	}

}

void Terrain::createDescriptorSets(vk::DescriptorPool descriptorpool)
{
	{
	     // create sets from the pool based on the layout
		 // 	     
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
	     
	     	vk::DescriptorBufferInfo vertexbufferInfo{};
	     	vertexbufferInfo.buffer = vertexUniformBuffers[i].buffer;
	     	vertexbufferInfo.offset = 0;
	     	vertexbufferInfo.range = sizeof(TransformMatrices);
	     
	     	vk::WriteDescriptorSet VertexUniformdescriptorWrite{};
	     	VertexUniformdescriptorWrite.dstSet = DescriptorSets[i];
	     	VertexUniformdescriptorWrite.dstBinding = 0;
	     	VertexUniformdescriptorWrite.dstArrayElement = 0;
	     	VertexUniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
	     	VertexUniformdescriptorWrite.descriptorCount = 1;
	     	VertexUniformdescriptorWrite.pBufferInfo = &vertexbufferInfo;
	     	;
	     	/////////////////////////////////////////////////////////////////////////////////////
	     	vk::DescriptorImageInfo imageInfo{};
	     	imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	     	imageInfo.imageView = albedoTextureData.imageView;
	     	imageInfo.sampler = albedoTextureData.imageSampler;
	     
	     	vk::WriteDescriptorSet SamplerdescriptorWrite{};
	     	SamplerdescriptorWrite.dstSet = DescriptorSets[i];
	     	SamplerdescriptorWrite.dstBinding = 1;
	     	SamplerdescriptorWrite.dstArrayElement = 0;
	     	SamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	     	SamplerdescriptorWrite.descriptorCount = 1;
	     	SamplerdescriptorWrite.pImageInfo = &imageInfo;
	     	/////////////////////////////////////////////////////////////////////////////////////
	     
	     	/////////////////////////////////////////////////////////////////////////////////////
	     	vk::DescriptorImageInfo NormalimageInfo{};
	     	NormalimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	     	NormalimageInfo.imageView = normalTextureData.imageView;
	     	NormalimageInfo.sampler = normalTextureData.imageSampler;
	     
	     	vk::WriteDescriptorSet NormalSamplerdescriptorWrite{};
	     	NormalSamplerdescriptorWrite.dstSet = DescriptorSets[i];
	     	NormalSamplerdescriptorWrite.dstBinding = 2;
	     	NormalSamplerdescriptorWrite.dstArrayElement = 0;
	     	NormalSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	     	NormalSamplerdescriptorWrite.descriptorCount = 1;
	     	NormalSamplerdescriptorWrite.pImageInfo = &NormalimageInfo;
	     	/////////////////////////////////////////////////////////////////////////////////////
	     
			vk::DescriptorImageInfo MetallicRoughnessimageInfo{};
			MetallicRoughnessimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			MetallicRoughnessimageInfo.imageView = MetallicRoughnessTextureData.imageView;
			MetallicRoughnessimageInfo.sampler   = MetallicRoughnessTextureData.imageSampler;

			vk::WriteDescriptorSet MetallicRoughnessSamplerdescriptorWrite{};
			MetallicRoughnessSamplerdescriptorWrite.dstSet = DescriptorSets[i];
			MetallicRoughnessSamplerdescriptorWrite.dstBinding = 3;
			MetallicRoughnessSamplerdescriptorWrite.dstArrayElement = 0;
			MetallicRoughnessSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			MetallicRoughnessSamplerdescriptorWrite.descriptorCount = 1;
			MetallicRoughnessSamplerdescriptorWrite.pImageInfo = &MetallicRoughnessimageInfo;
			/////////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorImageInfo HeighMapimageInfo{};
			HeighMapimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			HeighMapimageInfo.imageView   = HeighMapTextureData.imageView;
			HeighMapimageInfo.sampler     = HeighMapTextureData.imageSampler;

			vk::WriteDescriptorSet HeighMapSamplerdescriptorWrite{};
			HeighMapSamplerdescriptorWrite.dstSet = DescriptorSets[i];
			HeighMapSamplerdescriptorWrite.dstBinding = 4;
			HeighMapSamplerdescriptorWrite.dstArrayElement = 0;
			HeighMapSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			HeighMapSamplerdescriptorWrite.descriptorCount = 1;
			HeighMapSamplerdescriptorWrite.pImageInfo = &HeighMapimageInfo;
	     
	     	std::array<vk::WriteDescriptorSet, 5> descriptorWrites{ VertexUniformdescriptorWrite,
	     															SamplerdescriptorWrite,NormalSamplerdescriptorWrite,MetallicRoughnessSamplerdescriptorWrite,HeighMapSamplerdescriptorWrite };
	     
	     	vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	     }
	}

}


void Terrain::UpdateUniformBuffer(uint32_t currentImage)
{
	Drawable::UpdateUniformBuffer(currentImage);
	
	transformMatrices.viewMatrix = camera->GetViewMatrix();
	transformMatrices.projectionMatrix = camera->GetProjectionMatrix();
	transformMatrices.projectionMatrix[1][1] *= -1;

	memcpy(VertexUniformBuffersMappedMem[currentImage], &transformMatrices, sizeof(transformMatrices));
}


void Terrain::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };
	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint32);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(storedModelData->IndexData.size(), 1, 0, 0, 0);
}


void Terrain::CleanUp()
{
	if (bufferManager)
	{
		storedModelData = nullptr;
		bufferManager->DestroyImage(albedoTextureData);
		bufferManager->DestroyImage(normalTextureData);
		bufferManager->DestroyImage(MetallicRoughnessTextureData);
		bufferManager->DestroyImage(HeighMapTextureData);

	}
	
	Drawable::Destructor();
}



