#include "Grass.h"
#include <stdexcept>
#include <chrono>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <random>
#include "imgui.h"

Grass::Grass(const std::string filepath, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger)
	      : Drawable()
{

	FilePath = filepath;
	vulkanContext = vulkancontext;
	commandPool = commandpool;
	camera = rcamera;
	bufferManager = buffermanger;

	GeneratePositionalData();
	CreateVertexAndIndexBuffer();
	LoadTextures();
	CreateUniformBuffer();
	createDescriptorSetLayout();
}

void Grass::LoadTextures()
{
	//
	//std::vector<StoredImageData> ModelTextures = AssetManager::GetInstance().GetStoredImageData("../Textures/Cube/Terrain/HighResPlane.gltf");
	//
	//StoredImageData HeighMapImageData = ModelTextures[3];
	//vk::DeviceSize  HeighMapImageDataImagesize = HeighMapImageData.imageWidth * HeighMapImageData.imageHeight * 4;
	//
	// bufferManager->CreateTextureImage(&HeighMapImageData,HeighMapImageData.imageData, HeighMapImageDataImagesize, HeighMapImageData.imageWidth, HeighMapImageData.imageHeight, vk::Format::eR8G8B8A8Unorm, commandPool, vulkanContext->graphicsQueue);

}

void Grass::GeneratePositionalData()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dist(-0.4f, 0.4f);

	int gridSize = 141;
	float spacing = 0.4f;

	InsanceData.reserve(gridSize * gridSize);

	float start = -(gridSize / 2) * spacing;

	for (int x = 0; x < gridSize; x++)
	{
		for (int z = 0; z< gridSize; z++)
		{
			GrassData instanceGrassData;

			float randomOffsetX = dist(gen);
			float randomOffsetZ = dist(gen);

			float posX = start + x * spacing + randomOffsetX;
			float posZ = start + z * spacing + randomOffsetZ;

			glm::vec3 Position(posX, -3, posZ);

			glm::vec3 Rotation(0, 0, 0);
			glm::vec3 Scale   (1.3, 1.3, 1.3);

			glm::mat4 model = glm::mat4(1.0f);
			          model = glm::translate(model, Position);
			          model = glm::rotate(model, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			          model = glm::rotate(model, glm::radians(Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			          model = glm::rotate(model, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			          model = glm::scale(model, Scale);

			instanceGrassData.ModelMatrix = model;

			InsanceData.push_back(instanceGrassData);
		}
	}

	GrassDatavertexBufferSize = InsanceData.size() * sizeof(GrassData);
}

void Grass::CreateVertexAndIndexBuffer()
{
	storedModelData = &AssetManager::GetInstance().GetStoredModelData(FilePath);

	VkDeviceSize VertexBufferSize = sizeof(storedModelData->VertexData[0]) * storedModelData->VertexData.size();
	vertexBufferData.BufferID = "Grass Vertex Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&vertexBufferData, storedModelData->VertexData.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(uint32_t) * storedModelData->IndexData.size();
	indexBufferData.BufferID = "Grass Index Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&indexBufferData, storedModelData->IndexData.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer, commandPool, vulkanContext->graphicsQueue);

}


void Grass::CreateUniformBuffer()
{
	{
		vertexUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		VertexUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

		VkDeviceSize VertexuniformBufferSize = sizeof(InstanceTransformMatrices);

		for (size_t i = 0; i < vertexUniformBuffers.size(); i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "Meshed_Model Vertex Uniform Buffer" + i;
			bufferManager->CreateBuffer(&bufferdata,VertexuniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
			vertexUniformBuffers[i] = bufferdata;

			VertexUniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
		}
	}


	{
		GrassDataStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		GrassDataStorageBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);


		for (size_t i = 0; i < GrassDataStorageBuffers.size(); i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "GrassData Vertex Uniform Buffer" + i;

			bufferManager->CreateBuffer(&bufferdata, GrassDatavertexBufferSize, vk::BufferUsageFlagBits::eStorageBuffer, commandPool, vulkanContext->graphicsQueue);
			GrassDataStorageBuffers[i] = bufferdata;

			GrassDataStorageBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
		}
	}
}

void Grass::createDescriptorSetLayout()
{
	vk::DescriptorSetLayoutBinding VertexUniformBufferBinding{};
	VertexUniformBufferBinding.binding = 0;
	VertexUniformBufferBinding.descriptorCount = 1;
	VertexUniformBufferBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	VertexUniformBufferBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

	vk::DescriptorSetLayoutBinding  GrassDataVertexUniformBufferBinding{};
	GrassDataVertexUniformBufferBinding.binding = 1;
	GrassDataVertexUniformBufferBinding.descriptorCount = 1;
	GrassDataVertexUniformBufferBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
	GrassDataVertexUniformBufferBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;


	vk::DescriptorSetLayoutBinding HeightMapVertexUniformBufferBinding{};
	HeightMapVertexUniformBufferBinding.binding = 2;
	HeightMapVertexUniformBufferBinding.descriptorCount = 1;
	HeightMapVertexUniformBufferBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	HeightMapVertexUniformBufferBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

	std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {VertexUniformBufferBinding,GrassDataVertexUniformBufferBinding,HeightMapVertexUniformBufferBinding };

	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();


	if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess)
	{
		throw std::runtime_error("Failed to create descriptorset layout!");
	}

}

void Grass::createDescriptorSets(vk::DescriptorPool descriptorpool)
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
	     	vertexbufferInfo.range = sizeof(InstanceTransformMatrices);
	     
	     	vk::WriteDescriptorSet VertexUniformdescriptorWrite{};
	     	VertexUniformdescriptorWrite.dstSet = DescriptorSets[i];
	     	VertexUniformdescriptorWrite.dstBinding = 0;
	     	VertexUniformdescriptorWrite.dstArrayElement = 0;
	     	VertexUniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
	     	VertexUniformdescriptorWrite.descriptorCount = 1;
	     	VertexUniformdescriptorWrite.pBufferInfo = &vertexbufferInfo;

			vk::DescriptorBufferInfo  GrassDatavertexbufferInfo{};
			GrassDatavertexbufferInfo.buffer = GrassDataStorageBuffers[i].buffer;
			GrassDatavertexbufferInfo.offset = 0;
			GrassDatavertexbufferInfo.range = GrassDatavertexBufferSize;

			vk::WriteDescriptorSet  GrassDataVertexUniformdescriptorWrite{};
			GrassDataVertexUniformdescriptorWrite.dstSet = DescriptorSets[i];
			GrassDataVertexUniformdescriptorWrite.dstBinding = 1;
			GrassDataVertexUniformdescriptorWrite.dstArrayElement = 0;
			GrassDataVertexUniformdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
			GrassDataVertexUniformdescriptorWrite.descriptorCount = 1;
			GrassDataVertexUniformdescriptorWrite.pBufferInfo = &GrassDatavertexbufferInfo;

			vk::DescriptorImageInfo TerrainHeightMapInfo{};
			TerrainHeightMapInfo.imageView = HeighMapTextureData.imageView;
			TerrainHeightMapInfo.sampler = HeighMapTextureData.imageSampler;
			TerrainHeightMapInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

			vk::WriteDescriptorSet TerrainHeightSamplerdescriptorWrite{};
			TerrainHeightSamplerdescriptorWrite.dstSet = DescriptorSets[i];
			TerrainHeightSamplerdescriptorWrite.dstBinding = 2;
			TerrainHeightSamplerdescriptorWrite.dstArrayElement = 0;
			TerrainHeightSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			TerrainHeightSamplerdescriptorWrite.descriptorCount = 1;
			TerrainHeightSamplerdescriptorWrite.pImageInfo = &TerrainHeightMapInfo;

	     
	     	std::array<vk::WriteDescriptorSet, 3> descriptorWrites{VertexUniformdescriptorWrite,GrassDataVertexUniformdescriptorWrite,TerrainHeightSamplerdescriptorWrite
	     															//SamplerdescriptorWrite,NormalSamplerdescriptorWrite,MetallicRoughnessSamplerdescriptorWrite 
																	};
	     
	     	vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	     }
	}

}


void Grass::UpdateUniformBuffer(uint32_t currentImage)
{
	Drawable::UpdateUniformBuffer(currentImage);
	
	InstancetransformMatrices.viewMatrix = camera->GetViewMatrix();
	InstancetransformMatrices.projectionMatrix = camera->GetProjectionMatrix();
	InstancetransformMatrices.projectionMatrix[1][1] *= -1;

	memcpy(VertexUniformBuffersMappedMem[currentImage], &InstancetransformMatrices, sizeof(InstancetransformMatrices));


	memcpy(GrassDataStorageBuffersMappedMem[currentImage], InsanceData.data(), GrassDatavertexBufferSize);

}


void Grass::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{	
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };

	float deltatime = ImGui::GetTime();

	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint32);
	commandbuffer.pushConstants(pipelinelayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(deltatime), &deltatime);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(storedModelData->IndexData.size(), 20000, 0, 0, 0);
}


void Grass::CleanUp()
{
	if (bufferManager)
	{
		bufferManager->DestroyImage(albedoTextureData);
		bufferManager->DestroyImage(normalTextureData);
		bufferManager->DestroyImage(MetallicRoughnessTextureData);
	}
	
	Drawable::Destructor();
}




