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
	Instantiate();

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

	VkDeviceSize UniformBufferSize = sizeof(InstanceTransformMatrices);

	for (size_t i = 0; i < vertexUniformBuffers.size(); i++)
	{
		BufferData bufferdata;
		bufferdata.BufferID = "Light Uniform Buffer";
	    bufferManager->CreateBuffer(&bufferdata,UniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
		vertexUniformBuffers[i] = bufferdata;

		VertexUniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
	}

	{
		Light_GPU_DataUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		Light_GPU_DataUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

		VkDeviceSize ModeluniformBufferSize = sizeof(Light_GPU_InstanceData) * 300;

		for (size_t i = 0; i < Light_GPU_DataUniformBuffers.size(); i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "Model Vertex Uniform Buffer" + i;
			bufferManager->CreateBuffer(&bufferdata, ModeluniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
			Light_GPU_DataUniformBuffers[i] = bufferdata;

			Light_GPU_DataUniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
		}
	}

}

void Light::Instantiate()
{
	vulkanContext->ResetTemporalAccumilation();

	if (!Instances.empty())
	{
		int LastIndex = Instances.size() - 1;

		Light_InstanceData* NewInstance = new Light_InstanceData(Instances[LastIndex], vulkanContext);

		Instances.push_back(NewInstance);
		GPU_InstancesData.push_back(NewInstance->gpu_InstanceData);

	}
	else
	{
		Light_InstanceData* NewInstance = new Light_InstanceData(nullptr, vulkanContext);
		glm::mat4 BaseModelMatrix = glm::mat4(1);
		NewInstance->SetModelMatrix(BaseModelMatrix);

		Instances.push_back(NewInstance);
		GPU_InstancesData.push_back(NewInstance->gpu_InstanceData);
	}
}

void Light::Destroy(int instanceIndex)
{
	vulkanContext->ResetTemporalAccumilation();

	if (!Instances.empty() && Instances[instanceIndex] && GPU_InstancesData[instanceIndex])
	{
		Instances.erase(Instances.begin() + instanceIndex);
		GPU_InstancesData.erase(GPU_InstancesData.begin() + instanceIndex);
	}
}

void Light::createDescriptorSetLayout()
{
	vk::DescriptorSetLayoutBinding VertexUniformBufferBinding{};
	VertexUniformBufferBinding.binding = 0;
	VertexUniformBufferBinding.descriptorCount = 1;
	VertexUniformBufferBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	VertexUniformBufferBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

	vk::DescriptorSetLayoutBinding ModelUniformBufferBinding{};
	ModelUniformBufferBinding.binding = 1;
	ModelUniformBufferBinding.descriptorCount = 1;
	ModelUniformBufferBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	ModelUniformBufferBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

	std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { VertexUniformBufferBinding,ModelUniformBufferBinding };

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
		VertexbufferInfo.range = sizeof(InstanceTransformMatrices);

		vk::WriteDescriptorSet VertexUniformdescriptorWrite{};
		VertexUniformdescriptorWrite.dstSet = DescriptorSets[i];
		VertexUniformdescriptorWrite.dstBinding = 0;
		VertexUniformdescriptorWrite.dstArrayElement = 0;
		VertexUniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		VertexUniformdescriptorWrite.descriptorCount = 1;
		VertexUniformdescriptorWrite.pBufferInfo = &VertexbufferInfo;
        
		vk::DescriptorBufferInfo ModelbufferInfo{};
		ModelbufferInfo.buffer = Light_GPU_DataUniformBuffers[i].buffer;
		ModelbufferInfo.offset = 0;
		ModelbufferInfo.range = sizeof(Light_GPU_InstanceData) * 300;

		vk::WriteDescriptorSet ModelUniformdescriptorWrite{};
		ModelUniformdescriptorWrite.dstSet = DescriptorSets[i];
		ModelUniformdescriptorWrite.dstBinding = 1;
		ModelUniformdescriptorWrite.dstArrayElement = 0;
		ModelUniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		ModelUniformdescriptorWrite.descriptorCount = 1;
		ModelUniformdescriptorWrite.pBufferInfo = &ModelbufferInfo;

		std::array<vk::WriteDescriptorSet, 2> descriptorWrites = { VertexUniformdescriptorWrite,ModelUniformdescriptorWrite };

		////////////////////////////////////////////////////////////////////////////////////////
		vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}

void Light::UpdateUniformBuffer(uint32_t currentImage)
{
	//ambientStrength = std::clamp(ambientStrength, 0.0f, 1.0f); //Clamp AmbientStrength

	Drawable::UpdateUniformBuffer(currentImage);

	InstancetransformMatrices.viewMatrix = camera->GetViewMatrix();
	InstancetransformMatrices.projectionMatrix = camera->GetProjectionMatrix();
	InstancetransformMatrices.projectionMatrix[1][1] *= -1;
	memcpy(VertexUniformBuffersMappedMem[currentImage], &InstancetransformMatrices, sizeof(InstancetransformMatrices));
	

	for (size_t i = 0; i < GPU_InstancesData.size(); i++) {
		Light_GPU_InstanceData* instanceData = GPU_InstancesData[i].get();
		memcpy((char*)Light_GPU_DataUniformBuffersMappedMem[currentImage] + i * sizeof(Light_GPU_InstanceData), instanceData, sizeof(Light_GPU_InstanceData));
	}

}

void Light::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{

	//Continue Rendering As Usual
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };
	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.draw(vertices.size(), Instances.size(), 0, 0);
}


void Light::CleanUp()
{
	Drawable::Destructor();
}
