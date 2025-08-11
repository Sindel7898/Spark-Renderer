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
	BreakDownAndUpdateModelMatrix();
}

void Drawable::createDescriptorSets(vk::DescriptorPool descriptorpool)
{
}



glm::mat4 Drawable::GetModelMatrix()
{
	return transformMatrices.modelMatrix;
}

void Drawable::SetPosition(glm::vec3 newposition)
{
	position = newposition;
	UpdateModelMatrix();
}

void Drawable::SetRotation(glm::vec3 rotationAxis)
{
	rotation = rotationAxis;
	UpdateModelMatrix();
}


void Drawable::SetScale(glm::vec3 newscale)
{
	scale = newscale;
	UpdateModelMatrix();
}


void Drawable::SetModelMatrix(glm::mat4 newModelMatrix)
{
    transformMatrices.modelMatrix = newModelMatrix;
}

void Drawable::UpdateModelMatrix()
{
	transformMatrices.modelMatrix = glm::mat4(1.0f);
	transformMatrices.modelMatrix = glm::translate(transformMatrices.modelMatrix, position);
	transformMatrices.modelMatrix = glm::rotate(transformMatrices.modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	transformMatrices.modelMatrix = glm::rotate(transformMatrices.modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	transformMatrices.modelMatrix = glm::rotate(transformMatrices.modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	transformMatrices.modelMatrix = glm::scale(transformMatrices.modelMatrix, scale);
}

void Drawable::BreakDownAndUpdateModelMatrix()
{

    glm::vec3 Newscale;
    glm::quat Newrotation;
    glm::vec3 Newtranslation;
    glm::vec3 Newskew;
    glm::vec4 Newperspective;
    glm::decompose(transformMatrices.modelMatrix, Newscale, Newrotation, Newtranslation, Newskew, Newperspective);

    position = Newtranslation;
    rotation = glm::degrees(glm::eulerAngles(glm::conjugate(Newrotation)));
    scale = Newscale;

}
