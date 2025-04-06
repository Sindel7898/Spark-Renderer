#include "Model.h"
#include <stdexcept>
#include <chrono>

Model::Model(const std::string& filepath, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* camera, BufferManager* buffermanger)
	      : vulkanContext(vulkancontext), commandPool(commandpool), camera(camera), bufferManger(buffermanger), filePath(filepath)
{
	/*meshLoader = std::make_unique<MeshLoader>();
	meshLoader->LoadModel(filepath);*/
	AssetManager::GetInstance().LoadModel(filepath);
	CreateVertexAndIndexBuffer();
	CreateUniformBuffer();
	createDescriptorSetLayout();
}

void Model::LoadTextures(const std::string& filepath)
{

	AssetManager::GetInstance().LoadTexture(filepath);
	StoredImageData imageadata =  AssetManager::GetInstance().GetStoredImageData(filepath);

	MeshTextureData = bufferManger->CreateTextureImage(imageadata.imageData, imageadata.imageWidth, imageadata.imageHeight, commandPool, vulkanContext->graphicsQueue);
}

void Model::CreateVertexAndIndexBuffer()
{

	storedModelData = AssetManager::GetInstance().GetStoredModelData(filePath);

	VkDeviceSize VertexBufferSize = sizeof(storedModelData.VertexData[0]) * storedModelData.VertexData.size();
	VertexBufferData = bufferManger->CreateGPUOptimisedBuffer(storedModelData.VertexData.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(storedModelData.IndexData[0]) * storedModelData.IndexData.size();
	IndexBufferData = bufferManger->CreateGPUOptimisedBuffer(storedModelData.IndexData.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer, commandPool, vulkanContext->graphicsQueue);

}

void Model::CreateUniformBuffer()
{
	uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	uniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

	VkDeviceSize UniformBufferSize = sizeof(UniformBufferObject1);

	for (size_t i = 0; i < uniformBuffers.size(); i++)
	{

		BufferData bufferdata = bufferManger->CreateBuffer(UniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
		uniformBuffers[i] = bufferdata;

		uniformBuffersMappedMem[i] = bufferManger->MapMemory(bufferdata);
	}
}

void Model::createDescriptorSetLayout()
{
	//Difines type that is sending and where to 
	vk::DescriptorSetLayoutBinding UniformBufferBinding{};
	UniformBufferBinding.binding = 0;
	UniformBufferBinding.descriptorCount = 1;
	UniformBufferBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	UniformBufferBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

	vk::DescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

	std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { UniformBufferBinding, samplerLayoutBinding };

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

		vk::DescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers[i].buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject1);

		vk::DescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.imageView = MeshTextureData.imageView;
		imageInfo.sampler = MeshTextureData.imageSampler;

		vk::WriteDescriptorSet UniformdescriptorWrite{};
		UniformdescriptorWrite.dstSet = DescriptorSets[i];
		UniformdescriptorWrite.dstBinding = 0;
		UniformdescriptorWrite.dstArrayElement = 0;
		UniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		UniformdescriptorWrite.descriptorCount = 1;
		UniformdescriptorWrite.pBufferInfo = &bufferInfo;

		vk::WriteDescriptorSet SamplerdescriptorWrite{};
		SamplerdescriptorWrite.dstSet = DescriptorSets[i];
		SamplerdescriptorWrite.dstBinding = 1;
		SamplerdescriptorWrite.dstArrayElement = 0;
		SamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		SamplerdescriptorWrite.descriptorCount = 1;
		SamplerdescriptorWrite.pImageInfo = &imageInfo;

		std::array<vk::WriteDescriptorSet, 2> descriptorWrites{ UniformdescriptorWrite ,SamplerdescriptorWrite };

		vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}


void Model::UpdateUniformBuffer(uint32_t currentImage, int XLocation, int YLocation, int ZLocation)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject1 ubo{};
	ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f, 10.0f, 10.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(XLocation, YLocation, ZLocation));
	ubo.view = camera->GetViewMatrix();
	ubo.proj = camera->GetProjectionMatrix();
	ubo.proj[1][1] *= -1;

	memcpy(uniformBuffersMappedMem[currentImage], &ubo, sizeof(ubo));
}

void Model::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { VertexBufferData.buffer };
	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(IndexBufferData.buffer, 0, vk::IndexType::eUint16);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(static_cast<uint32_t>(storedModelData.IndexData.size()), 1, 0, 0, 0);
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
		bufferManger->DestroyImage(MeshTextureData);
		bufferManger->DestroyBuffer(VertexBufferData);
		bufferManger->DestroyBuffer(IndexBufferData);

		for (auto& uniformBuffer : uniformBuffers)
		{
			bufferManger->UnmapMemory(uniformBuffer);
			bufferManger->DestroyBuffer(uniformBuffer);
		}

		uniformBuffers.clear();
	}
}

Model::~Model()
{
	Clean();
}


