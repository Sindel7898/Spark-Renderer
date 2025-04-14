#include "Drawable.h"
#include "VulkanContext.h"


Drawable::Drawable()
{
}

Drawable::~Drawable()
{
}

void Drawable::Destructor()
{
	if (vulkanContext)
	{

		if (descriptorSetLayout)
		{
			vulkanContext->LogicalDevice.destroyDescriptorSetLayout(descriptorSetLayout);
		}
	}

	if (bufferManager)
	{
		bufferManager->DestroyBuffer(vertexBufferData);
		bufferManager->DestroyBuffer(indexBufferData);

		for (auto& uniformBuffer : vertexUniformBuffers)
		{
			bufferManager->UnmapMemory(uniformBuffer);
			bufferManager->DestroyBuffer(uniformBuffer);
		}

		for (auto& uniformBuffer : fragmentUniformBuffers)
		{
			bufferManager->UnmapMemory(uniformBuffer);
			bufferManager->DestroyBuffer(uniformBuffer);
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

void Drawable::UpdateUniformBuffer(uint32_t currentImage, Light* lightref)
{
	BreakDownAndUpdateModelMatrix();
}



glm::mat4 Drawable::GetModelMatrix()
{
	return transformMatrices.modelMatrix;
}

void Drawable::SetModelMatrix(glm::mat4 newModelMatrix)
{
    transformMatrices.modelMatrix = newModelMatrix;
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
