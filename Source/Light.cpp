#include "Light.h"
#include <stdexcept>
#include <memory>
#include <chrono>
#include "Camera.h"
#include "VulkanContext.h"
#include <backends/imgui_impl_vulkan.h>

Light::Light(VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* cameraref, BufferManager* buffermanger)
{
	vulkanContext = vulkancontext;
	commandPool = commandpool;
	camera = cameraref;
	bufferManager = buffermanger;
	
	CreateVertexAndIndexBuffer();
	CreateUniformBuffer();
	createDescriptorSetLayout();

	lightType       = 1;
	position        = glm::vec3(0.0f, -1, 0.0f);
	rotation        = glm::vec3(1.0f, 1.0f, 1.0f);
	scale           = glm::vec3(0.5f, 0.5f, 0.5f);
	color           = glm::vec3(1.0f, 1.0f, 1.0f);
	ambientStrength = 0.5f;
	lightIntensity  = 4.0f;
	CastShadow      = 1;

	transformMatrices.modelMatrix = glm::mat4(1.0f);
	transformMatrices.modelMatrix = glm::translate(transformMatrices.modelMatrix, position);
	transformMatrices.modelMatrix = glm::rotate   (transformMatrices.modelMatrix, glm::radians(rotation.x), glm::vec3(0.0f, 0.0f, 1.0f));
	transformMatrices.modelMatrix = glm::rotate   (transformMatrices.modelMatrix, glm::radians(rotation.y), glm::vec3(1.0f, 0.0f, 0.0f));
	transformMatrices.modelMatrix = glm::rotate   (transformMatrices.modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 1.0f, 0.0f));
	transformMatrices.modelMatrix = glm::scale    (transformMatrices.modelMatrix , scale);
}


void Light::CreateVertexAndIndexBuffer()
{
	VkDeviceSize VertexBufferSize = sizeof(vertices.data()[0]) * vertices.size();
	vertexBufferData.BufferID = "Light Vertex Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&vertexBufferData,vertices.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);
}

void Light::CreateUniformBuffer()
{
	vertexUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	VertexUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

	VkDeviceSize UniformBufferSize = sizeof(TransformMatrices);

	for (size_t i = 0; i < vertexUniformBuffers.size(); i++)
	{
		BufferData bufferdata;
		bufferdata.BufferID = "Light Uniform Buffer";
	    bufferManager->CreateBuffer(&bufferdata,UniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
		vertexUniformBuffers[i] = bufferdata;

		VertexUniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
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
		VertexbufferInfo.buffer = vertexUniformBuffers[i].buffer;
		VertexbufferInfo.offset = 0;
		VertexbufferInfo.range = sizeof(TransformMatrices);

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
	//ambientStrength = std::clamp(ambientStrength, 0.0f, 1.0f); //Clamp AmbientStrength

	Drawable::UpdateUniformBuffer(currentImage);

	transformMatrices.viewMatrix = camera->GetViewMatrix();
	transformMatrices.projectionMatrix = camera->GetProjectionMatrix();
	transformMatrices.projectionMatrix[1][1] *= -1;
	memcpy(VertexUniformBuffersMappedMem[currentImage], &transformMatrices, sizeof(transformMatrices));
}

void Light::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{
	//Send Light Color To The Fragment Shader 
	commandbuffer.pushConstants(pipelinelayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(color), &color);

	//Continue Rendering As Usual
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };
	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.draw(vertices.size(), 1, 0, 0);
}


void Light::CleanUp()
{
	Drawable::Destructor();
}
