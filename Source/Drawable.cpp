#include "Drawable.h"
#include "VulkanContext.h"
#include "Light.h"


Drawable::Drawable()
{
}

Drawable::~Drawable()
{
}

void Drawable::Destructor()
{
	//Clean up
	if (vulkanContext)
	{

		if (descriptorSetLayout)
		{
			vulkanContext->LogicalDevice.destroyDescriptorSetLayout(descriptorSetLayout);
		}
		
	}

	if (bufferManager)
	{
		if (vertexBufferData.buffer)
		{
			bufferManager->DestroyBuffer(vertexBufferData);
		}

		if (indexBufferData.buffer)
		{
			bufferManager->DestroyBuffer(indexBufferData);
		}


		for (auto& uniformBuffer : vertexUniformBuffers)
		{
			if (uniformBuffer.buffer)
			{
				bufferManager->UnmapMemory(uniformBuffer);
				bufferManager->DestroyBuffer(uniformBuffer);
			}
		}

		for (auto& uniformBuffer : fragmentUniformBuffers)
		{
			if (uniformBuffer.buffer)
			{
				bufferManager->UnmapMemory(uniformBuffer);
				bufferManager->DestroyBuffer(uniformBuffer);
			}
		}

		VertexUniformBuffersMappedMem.clear();
		FragmentUniformBuffersMappedMem.clear();

		vertexUniformBuffers.clear();
		fragmentUniformBuffers.clear();


		bufferManager = nullptr;
		vulkanContext = nullptr;
		camera = nullptr;
	}
}

void Drawable::CreateUniformBuffer()
{
}

void Drawable::UpdateUniformBuffer(uint32_t currentImage)
{
}

void Drawable::createDescriptorSets(vk::DescriptorPool descriptorpool)
{
}


