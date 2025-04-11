#include "Light.h"
#include <stdexcept>
#include <memory>
#include <chrono>
#include "Camera.h"

Light::Light(VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* camera, BufferManager* buffermanger)
	      : vulkanContext(vulkancontext), commandPool(commandpool), camera(camera), bufferManger(buffermanger)
{

	CreateUniformBuffer();
	CreateVertex();
	createDescriptorSetLayout();

	LightLocation = glm::vec3(1.0f, 1.0f, 1.0f);
	LightScale = glm::vec3(1.0f, 1.0f, 1.0f);
	LightRotation = glm::vec3(1.0f, 1.0f, 1.0f);

	BaseColor = glm::vec4(1.0f,1.0f,1.0f,1.0f);
}


void Light::CreateVertex()
{
	VkDeviceSize VertexBufferSize = sizeof(vertices.data()[0]) * vertices.size();
	VertexBufferData = bufferManger->CreateGPUOptimisedBuffer(vertices.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);
}

void Light::CreateUniformBuffer()
{
	VertexuniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	VertexUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

	VkDeviceSize UniformBufferSize = sizeof(LightVertexUniformBufferObject);

	for (size_t i = 0; i < VertexuniformBuffers.size(); i++)
	{

		BufferData bufferdata = bufferManger->CreateBuffer(UniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
		VertexuniformBuffers[i] = bufferdata;

		VertexUniformBuffersMappedMem[i] = bufferManger->MapMemory(bufferdata);
	}
}


void Light::createDescriptorSetLayout()
{
	vk::DescriptorSetLayoutBinding VertexUniformBufferBinding{};
	VertexUniformBufferBinding.binding = 0;
	VertexUniformBufferBinding.descriptorCount = 1;
	VertexUniformBufferBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	VertexUniformBufferBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;


	std::array<vk::DescriptorSetLayoutBinding, 1> bindings = { VertexUniformBufferBinding};

	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();


	if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess)
	{
		throw std::runtime_error("Failed to create descriptorset layout!");
	}



}

void Light::createDescriptorSets(vk::DescriptorPool descriptorpool)
{
	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocinfo;
	allocinfo.descriptorPool = descriptorpool;
	allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocinfo.pSetLayouts = layouts.data();

	DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, DescriptorSets.data());

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		vk::DescriptorBufferInfo VertexbufferInfo{};
		VertexbufferInfo.buffer = VertexuniformBuffers[i].buffer;
		VertexbufferInfo.offset = 0;
		VertexbufferInfo.range = sizeof(LightVertexUniformBufferObject);

		vk::WriteDescriptorSet VertexUniformdescriptorWrite{};
		VertexUniformdescriptorWrite.dstSet = DescriptorSets[i];
		VertexUniformdescriptorWrite.dstBinding = 0;
		VertexUniformdescriptorWrite.dstArrayElement = 0;
		VertexUniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		VertexUniformdescriptorWrite.descriptorCount = 1;
		VertexUniformdescriptorWrite.pBufferInfo = &VertexbufferInfo;
        
		////////////////////////////////////////////////////////////////////////////////////////
		vulkanContext->LogicalDevice.updateDescriptorSets(1, &VertexUniformdescriptorWrite, 0, nullptr);
	}
}

void Light::UpdateUniformBuffer(uint32_t currentImage)
{
	ModelData.model = glm::mat4(1.0f);
	ModelData.model = glm::translate(ModelData.model, LightLocation);
	ModelData.model = glm::scale(ModelData.model, LightScale);
	ModelData.model = glm::rotate(ModelData.model, glm::radians(LightRotation.x), glm::vec3(0.0f, 0.0f, 1.0f));
	ModelData.model = glm::rotate(ModelData.model, glm::radians(LightRotation.y), glm::vec3(1.0f, 0.0f, 0.0f));
	ModelData.model = glm::rotate(ModelData.model, glm::radians(LightRotation.z), glm::vec3(0.0f, 1.0f, 0.0f));
	ModelData.view  = camera->GetViewMatrix();
	ModelData.proj  = camera->GetProjectionMatrix();
	ModelData.proj[1][1] *= -1;
	ModelData.BaseColor = BaseColor;
	memcpy(VertexUniformBuffersMappedMem[currentImage], &ModelData, sizeof(ModelData));

}

void Light::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { VertexBufferData.buffer };
	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.draw(vertices.size(), 1, 0, 0);
}

glm::mat4 Light::GetModelMatrix()
{
	return ModelData.model;
}

