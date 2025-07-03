#pragma once
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include "BufferManager.h"
#include <glm/fwd.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

class VulkanContext;
class Camera;
class Light;
 
struct TransformMatrices {
	alignas(16) glm::mat4 modelMatrix;
	alignas(16) glm::mat4 viewMatrix;
	alignas(16) glm::mat4 projectionMatrix;
};

struct InstanceTransformMatrices {
	alignas(16) glm::mat4 viewMatrix;
	alignas(16) glm::mat4 projectionMatrix;
};

class Drawable
{
public:
	  Drawable();
	 virtual ~Drawable();
	 void Destructor();

protected:
	
     virtual void CreateVertexAndIndexBuffer() = 0;
     virtual void CreateUniformBuffer();
	 virtual void UpdateUniformBuffer(uint32_t currentImage);
	 virtual void createDescriptorSetLayout() = 0;
	 virtual void createDescriptorSets(vk::DescriptorPool descriptorpool);
	 virtual void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) = 0;

	  void BreakDownAndUpdateModelMatrix();


	 std::vector<BufferData> vertexUniformBuffers;
	 std::vector<BufferData> fragmentUniformBuffers;

	 std::vector<void*> VertexUniformBuffersMappedMem;
	 std::vector<void*> FragmentUniformBuffersMappedMem;

	 BufferData vertexBufferData;
	 BufferData indexBufferData;

	

	 TransformMatrices  transformMatrices;
	 TransformMatrices  ShadowPasstransformMatrices;
	 InstanceTransformMatrices InstancetransformMatrices;



	 BufferManager* bufferManager = nullptr;
	 VulkanContext* vulkanContext = nullptr;
	 Camera*        camera        = nullptr;


	 vk::CommandPool                commandPool;
	 std::vector<vk::DescriptorSet> DescriptorSets;

public:
	glm::mat4 GetModelMatrix();
	void SetPosition(glm::vec3 newposition);
	void SetRotation(glm::vec3 newrotation, float radians);
	void SetScale(glm::vec3 newscale);
	void SetModelMatrix(glm::mat4 newModelMatrix);

	vk::DescriptorSetLayout  descriptorSetLayout;

	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

	glm::mat4 ViewMatrix;
	glm::mat4 ProjectionMatrix;
};

