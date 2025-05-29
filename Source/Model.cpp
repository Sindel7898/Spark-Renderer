#include "Model.h"
#include <stdexcept>
#include <chrono>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

Model::Model(const std::string filepath, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger)
	      : Drawable()
{

	FilePath = filepath;
	vulkanContext = vulkancontext;
	commandPool = commandpool;
	camera = rcamera;
	bufferManager = buffermanger;

	position = glm::vec3(1.0f,1.0f,1.0f);
	rotation = glm::vec3(90.0f,0.0f,0.0f);
	scale =    glm::vec3(2.0f, 2.0f, 2.0f);

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
	//CreateBottomLevelAccelerationStructure();
}

void Model::LoadTextures()
{
	std::vector<StoredImageData> ModelTextures = AssetManager::GetInstance().GetStoredImageData(FilePath);


	StoredImageData AlbedoImageData = ModelTextures[0];
	vk::DeviceSize AlbedoImagesize = AlbedoImageData.imageWidth * AlbedoImageData.imageHeight * 4;

	albedoTextureData = bufferManager->CreateTextureImage(AlbedoImageData.imageData, AlbedoImagesize, AlbedoImageData.imageWidth, AlbedoImageData.imageHeight,vk::Format::eR8G8B8A8Srgb , commandPool, vulkanContext->graphicsQueue);

	StoredImageData NormalImageData = ModelTextures[1];
	vk::DeviceSize NormalImagesize = NormalImageData.imageWidth * NormalImageData.imageHeight * 4;

	normalTextureData = bufferManager->CreateTextureImage(NormalImageData.imageData, NormalImagesize, NormalImageData.imageWidth, NormalImageData.imageHeight, vk::Format::eR8G8B8A8Unorm, commandPool, vulkanContext->graphicsQueue);
}

void Model::CreateVertexAndIndexBuffer()
{

	storedModelData = &AssetManager::GetInstance().GetStoredModelData(FilePath);

	transformMatrices.modelMatrix = storedModelData->modelMatrix;

	VkDeviceSize VertexBufferSize = sizeof(storedModelData->VertexData[0]) * storedModelData->VertexData.size();
	vertexBufferData = bufferManager->CreateGPUOptimisedBuffer(storedModelData->VertexData.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(uint32_t) * storedModelData->IndexData.size();
	indexBufferData = bufferManager->CreateGPUOptimisedBuffer(storedModelData->IndexData.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer, commandPool, vulkanContext->graphicsQueue);

}


void Model::CreateUniformBuffer()
{
	{
		vertexUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		VertexUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

		VkDeviceSize VertexuniformBufferSize = sizeof(VertexUniformData);

		for (size_t i = 0; i < vertexUniformBuffers.size(); i++)
		{

			BufferData bufferdata = bufferManager->CreateBuffer(VertexuniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
			vertexUniformBuffers[i] = bufferdata;

			VertexUniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
		}
	}
}



void Model::createDescriptorSetLayout()
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


	std::array<vk::DescriptorSetLayoutBinding, 3> bindings = { VertexUniformBufferBinding,
															   AlbedoSamplerLayout,NormalSamplerLayout, };

	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();


	if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess)
	{
		throw std::runtime_error("Failed to create descriptorset layout!");
	}

}

void Model::createDescriptorSets(vk::DescriptorPool descriptorpool)
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
	     
	     
	     	std::array<vk::WriteDescriptorSet, 3> descriptorWrites{ VertexUniformdescriptorWrite,
	     															SamplerdescriptorWrite,NormalSamplerdescriptorWrite };
	     
	     	vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	     }
	}

}


void Model::UpdateUniformBuffer(uint32_t currentImage, Light* lightref)
{
	Drawable::UpdateUniformBuffer(currentImage, lightref);
	
	transformMatrices.viewMatrix = camera->GetViewMatrix();
	transformMatrices.projectionMatrix = camera->GetProjectionMatrix();
	transformMatrices.projectionMatrix[1][1] *= -1;

	vertexdata.transformMatrices = transformMatrices;
	vertexdata.LightViewMatrix = lightref->ViewMatrix;
	vertexdata.LightProjectionMatrix = lightref->ProjectionMatrix;
	vertexdata.LightProjectionMatrix[1][1] *= -1;

	memcpy(VertexUniformBuffersMappedMem[currentImage], &vertexdata, sizeof(VertexUniformData));
}


void Model::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };
	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint32);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(storedModelData->IndexData.size(), 1, 0, 0, 0);
}


void Model::CleanUp()
{
	if (bufferManager)
	{
		storedModelData = nullptr;
		bufferManager->DestroyImage(albedoTextureData);
		bufferManager->DestroyImage(normalTextureData);

		/*bufferManager->DestroyBuffer(bottomLevelASBuffer);
		bufferManager->DestroyBuffer(scratchBuffer);*/
	}
	
	Drawable::Destructor();
}

//uint64_t Model::GetBLASAddressInfo()
//{
//	vk::AccelerationStructureDeviceAddressInfoKHR addressInfo{};
//	addressInfo.accelerationStructure = bottomLevelAS;
//	return vulkanContext->vkGetAccelerationStructureDeviceAddressKHR(vulkanContext->LogicalDevice, (VkAccelerationStructureDeviceAddressInfoKHR*)&addressInfo);
//}


