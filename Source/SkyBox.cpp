#include "SkyBox.h"
#include <stdexcept>
#include <memory>
#include <chrono>

SkyBox::SkyBox(std::array<const char*, 6> filePaths, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* cameraref, BufferManager* buffermanger)
{
	vulkanContext = vulkancontext;
	commandPool = commandpool;
	camera = cameraref;
	bufferManager = buffermanger;

	MeshTextureData = bufferManager->CreateCubeMap (filePaths, commandPool, vulkanContext->graphicsQueue);

	CreateUniformBuffer();
	CreateVertexAndIndexBuffer();
	createDescriptorSetLayout();
}


void SkyBox::CreateVertexAndIndexBuffer()
{
	VkDeviceSize VertexBufferSize = sizeof(vertices.data()[0]) * vertices.size();
	vertexBufferData.BufferID = "SkyBox Vertex Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&vertexBufferData,vertices.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);
}

void SkyBox::CreateUniformBuffer()
{
	vertexUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	VertexUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

	VkDeviceSize UniformBufferSize = sizeof(TransformMatrices);

	for (size_t i = 0; i < vertexUniformBuffers.size(); i++)
	{
		BufferData bufferdata;
		bufferdata.BufferID = "SkyBox Uniform Buffer";

		 bufferManager->CreateBuffer(&bufferdata,UniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
		vertexUniformBuffers[i] = bufferdata;

		VertexUniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
	}
}


void SkyBox::createDescriptorSetLayout()
{
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

void SkyBox::createDescriptorSets(vk::DescriptorPool descriptorpool)
{
	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocinfo;
	allocinfo.descriptorPool = descriptorpool;
	allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocinfo.pSetLayouts = layouts.data();

	DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, DescriptorSets.data());

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		vk::DescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = vertexUniformBuffers[i].buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(TransformMatrices);

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

void SkyBox::UpdateUniformBuffer(uint32_t currentImage)
{

	Drawable::UpdateUniformBuffer(currentImage);

	transformMatrices.modelMatrix = glm::mat4(1.0f);
	transformMatrices.viewMatrix = camera->GetViewMatrix();
	transformMatrices.projectionMatrix = camera->GetProjectionMatrix();
	transformMatrices.projectionMatrix[1][1] *= -1;

	memcpy(VertexUniformBuffersMappedMem[currentImage], &transformMatrices, sizeof(transformMatrices));
}

void SkyBox::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };
	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.draw(vertices.size(), 1, 0, 0);
}

void SkyBox::CleanUp()
{
	
    bufferManager->DestroyImage(MeshTextureData);
	Drawable::Destructor();
}


