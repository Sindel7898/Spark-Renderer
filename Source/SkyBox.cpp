#include "SkyBox.h"
#include <stdexcept>
#include <memory>
#include <chrono>

SkyBox::SkyBox(std::array<const char*, 6> filePaths, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* camera, BufferManager* buffermanger)
	      : vulkanContext(vulkancontext), commandPool(commandpool), camera(camera), bufferManger(buffermanger)
{
	MeshTextureData = bufferManger->CreateCubeMap(filePaths, commandPool, vulkanContext->graphicsQueue);

	CreateUniformBuffer();
	CreateVertex();
	createDescriptorSetLayout();
}


void SkyBox::CreateVertex()
{
	VkDeviceSize VertexBufferSize = sizeof(vertices.data()[0]) * vertices.size();
	VertexBufferData = bufferManger->CreateGPUOptimisedBuffer(vertices.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);
}

void SkyBox::CreateUniformBuffer()
{
	uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	uniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

	VkDeviceSize UniformBufferSize = sizeof(SkyBoxUniformBufferObject);

	for (size_t i = 0; i < uniformBuffers.size(); i++)
	{

		BufferData bufferdata = bufferManger->CreateBuffer(UniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
		uniformBuffers[i] = bufferdata;

		uniformBuffersMappedMem[i] = bufferManger->MapMemory(bufferdata);
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
		bufferInfo.buffer = uniformBuffers[i].buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(SkyBoxUniformBufferObject);

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
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	SkyBoxUniformBufferObject ubo{};
	ubo.model = glm::mat4(1.0f);
	ubo.view  = camera->GetViewMatrix();
	ubo.proj  = camera->GetProjectionMatrix();
	ubo.proj[1][1] *= -1;

	memcpy(uniformBuffersMappedMem[currentImage], &ubo, sizeof(ubo));
}

void SkyBox::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { VertexBufferData.buffer };
	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.draw(vertices.size(), 1, 0, 0);
}


