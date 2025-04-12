#include "Model.h"
#include <stdexcept>
#include <chrono>

Model::Model(const std::string& filepath, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* camera, BufferManager* buffermanger)
	      : vulkanContext(vulkancontext), commandPool(commandpool), camera(camera), bufferManger(buffermanger), FilePath(filepath)
{

	AssetManager::GetInstance().LoadModel(FilePath);
	LoadTextures();
	CreateVertexAndIndexBuffer();
	CreateUniformBuffer();
	createDescriptorSetLayout();

}

void Model::LoadTextures()
{
	std::vector<StoredImageData> ModelTextures = AssetManager::GetInstance().GetStoredImageData(FilePath);
	StoredImageData AlbedoImageData = ModelTextures[0];

	AlbedoTextureData = bufferManger->CreateTextureImage(AlbedoImageData.imageData, AlbedoImageData.imageWidth, AlbedoImageData.imageHeight,vk::Format::eR8G8B8A8Srgb , commandPool, vulkanContext->graphicsQueue);

	StoredImageData NormalImageData = ModelTextures[1];

	NormalTextureData = bufferManger->CreateTextureImage(NormalImageData.imageData, NormalImageData.imageWidth, NormalImageData.imageHeight, vk::Format::eR8G8B8A8Unorm, commandPool, vulkanContext->graphicsQueue);


}

void Model::CreateVertexAndIndexBuffer()
{

	storedModelData = AssetManager::GetInstance().GetStoredModelData(FilePath);

	VkDeviceSize VertexBufferSize = sizeof(storedModelData.VertexData[0]) * storedModelData.VertexData.size();
	VertexBufferData = bufferManger->CreateGPUOptimisedBuffer(storedModelData.VertexData.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(uint32_t) * storedModelData.IndexData.size();
	IndexBufferData = bufferManger->CreateGPUOptimisedBuffer(storedModelData.IndexData.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer, commandPool, vulkanContext->graphicsQueue);

}

void Model::CreateUniformBuffer()
{
	VertexuniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	VertexUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

	VkDeviceSize VertexuniformBufferSize = sizeof(VertexUniformBufferObject1);

	for (size_t i = 0; i < VertexuniformBuffers.size(); i++)
	{

		BufferData bufferdata = bufferManger->CreateBuffer(VertexuniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
		VertexuniformBuffers[i] = bufferdata;

		VertexUniformBuffersMappedMem[i] = bufferManger->MapMemory(bufferdata);
	}

	//////////////////////////////////////////////////////////////
	FragmentuniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	FragmentUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

	VkDeviceSize FragmentuniformBufferSize = sizeof(FragmentUniformBufferObject1);

	for (size_t i = 0; i < FragmentuniformBuffers.size(); i++)
	{

		BufferData bufferdata = bufferManger->CreateBuffer(FragmentuniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
		FragmentuniformBuffers[i] = bufferdata;

		FragmentUniformBuffersMappedMem[i] = bufferManger->MapMemory(bufferdata);
	}
}



void Model::createDescriptorSetLayout()
{
	//Difines type that is sending and where to 
	vk::DescriptorSetLayoutBinding VertexUniformBufferBinding{};
	VertexUniformBufferBinding.binding = 0;
	VertexUniformBufferBinding.descriptorCount = 1;
	VertexUniformBufferBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	VertexUniformBufferBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

	vk::DescriptorSetLayoutBinding FragmentUniformBufferBinding{};
	FragmentUniformBufferBinding.binding = 1;
	FragmentUniformBufferBinding.descriptorCount = 1;
	FragmentUniformBufferBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	FragmentUniformBufferBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 2;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutBinding NormalSamplerLayoutBinding{};
	NormalSamplerLayoutBinding.binding = 3;
	NormalSamplerLayoutBinding.descriptorCount = 1;
	NormalSamplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	NormalSamplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

	std::array<vk::DescriptorSetLayoutBinding, 4> bindings = { VertexUniformBufferBinding,
		                                                       FragmentUniformBufferBinding, 
		                                                       samplerLayoutBinding,NormalSamplerLayoutBinding };

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

		vk::DescriptorBufferInfo vertexbufferInfo{};
		vertexbufferInfo.buffer = VertexuniformBuffers[i].buffer;
		vertexbufferInfo.offset = 0;
		vertexbufferInfo.range = sizeof(VertexUniformBufferObject1);

		vk::WriteDescriptorSet VertexUniformdescriptorWrite{};
		VertexUniformdescriptorWrite.dstSet = DescriptorSets[i];
		VertexUniformdescriptorWrite.dstBinding = 0;
		VertexUniformdescriptorWrite.dstArrayElement = 0;
		VertexUniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		VertexUniformdescriptorWrite.descriptorCount = 1;
		VertexUniformdescriptorWrite.pBufferInfo = &vertexbufferInfo;
		/////////////////////////////////////////////////////////////////////////////////////
		vk::DescriptorBufferInfo FragmentbufferInfo{};
		FragmentbufferInfo.buffer = FragmentuniformBuffers[i].buffer;
		FragmentbufferInfo.offset = 0;
		FragmentbufferInfo.range = sizeof(FragmentUniformBufferObject1);

		vk::WriteDescriptorSet FragmentUniformdescriptorWrite{};
		FragmentUniformdescriptorWrite.dstSet = DescriptorSets[i];
		FragmentUniformdescriptorWrite.dstBinding = 1;
		FragmentUniformdescriptorWrite.dstArrayElement = 0;
		FragmentUniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		FragmentUniformdescriptorWrite.descriptorCount = 1;
		FragmentUniformdescriptorWrite.pBufferInfo = &FragmentbufferInfo;
		/////////////////////////////////////////////////////////////////////////////////////
		vk::DescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.imageView = AlbedoTextureData.imageView;
		imageInfo.sampler = AlbedoTextureData.imageSampler;

		vk::WriteDescriptorSet SamplerdescriptorWrite{};
		SamplerdescriptorWrite.dstSet = DescriptorSets[i];
		SamplerdescriptorWrite.dstBinding = 2;
		SamplerdescriptorWrite.dstArrayElement = 0;
		SamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		SamplerdescriptorWrite.descriptorCount = 1;
		SamplerdescriptorWrite.pImageInfo = &imageInfo;
		/////////////////////////////////////////////////////////////////////////////////////

			/////////////////////////////////////////////////////////////////////////////////////
		vk::DescriptorImageInfo NormalimageInfo{};
		NormalimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		NormalimageInfo.imageView = NormalTextureData.imageView;
		NormalimageInfo.sampler = NormalTextureData.imageSampler;

		vk::WriteDescriptorSet NormalSamplerdescriptorWrite{};
		NormalSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		NormalSamplerdescriptorWrite.dstBinding = 3;
		NormalSamplerdescriptorWrite.dstArrayElement = 0;
		NormalSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		NormalSamplerdescriptorWrite.descriptorCount = 1;
		NormalSamplerdescriptorWrite.pImageInfo = &NormalimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////


		std::array<vk::WriteDescriptorSet, 4> descriptorWrites{ VertexUniformdescriptorWrite,
			                                                    FragmentUniformdescriptorWrite ,
			                                                    SamplerdescriptorWrite,NormalSamplerdescriptorWrite };

		vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}


void Model::UpdateUniformBuffer(uint32_t currentImage, Light* Light)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	ModelData.model = glm::mat4(1.0f);
	ModelData.model = glm::translate(ModelData.model, position);
	ModelData.model = glm::rotate(ModelData.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	ModelData.model = glm::rotate(ModelData.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	ModelData.model = glm::rotate(ModelData.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	ModelData.model = glm::scale(ModelData.model, scale);

	ModelData.view = camera->GetViewMatrix();
	ModelData.proj = camera->GetProjectionMatrix();
	ModelData.proj[1][1] *= -1;

	memcpy(VertexUniformBuffersMappedMem[currentImage], &ModelData, sizeof(ModelData));

	LightData.LightLocation = Light->LightLocation;
	LightData.BaseColor = Light->BaseColor;
	LightData.AmbientStrength = Light->AmbientStrength;
	memcpy(FragmentUniformBuffersMappedMem[currentImage], &LightData, sizeof(LightData));

}

glm::mat4 Model::GetModelMatrix()
{
	return ModelData.model;
}

void Model::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { VertexBufferData.buffer };
	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(IndexBufferData.buffer, 0, vk::IndexType::eUint32);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(storedModelData.IndexData.size(), 1, 0, 0, 0);
}

void Model::Clean()
{
	if (vulkanContext)
	{
		vk::Device device = vulkanContext->LogicalDevice;

		if (descriptorSetLayout)
		{
			device.destroyDescriptorSetLayout(descriptorSetLayout);
		}
	}

	if (bufferManger)
	{
		bufferManger->DestroyImage(AlbedoTextureData);
		bufferManger->DestroyImage(NormalTextureData);
		bufferManger->DestroyBuffer(VertexBufferData);
		bufferManger->DestroyBuffer(IndexBufferData);

		for (auto& uniformBuffer : VertexuniformBuffers)
		{
			bufferManger->UnmapMemory(uniformBuffer);
			bufferManger->DestroyBuffer(uniformBuffer);
		}

		for (auto& uniformBuffer : FragmentuniformBuffers)
		{
			bufferManger->UnmapMemory(uniformBuffer);
			bufferManger->DestroyBuffer(uniformBuffer);
		}

		VertexUniformBuffersMappedMem.clear();
		FragmentUniformBuffersMappedMem.clear();

		VertexuniformBuffers.clear();
		FragmentuniformBuffers.clear();

	}
}

Model::~Model()
{
	Clean();
}


