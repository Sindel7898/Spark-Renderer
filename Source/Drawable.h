#pragma once
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include "BufferManager.h"
//#include <glm/fwd.hpp>
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

//struct TransformMatrices_LightColor {
//	alignas(16) glm::mat4 model;
//	alignas(16) glm::mat4 view;
//	alignas(16) glm::mat4 proj;
//	alignas(16) glm::vec4 BaseColor;
//};


class Drawable
{
public:
	  Drawable();
	 virtual ~Drawable();
	 virtual void Destructor();

protected:
	
     virtual void CreateVertexAndIndexBuffer() = 0;
     virtual void CreateUniformBuffer() = 0;
	 virtual void UpdateUniformBuffer(uint32_t currentImage, Light* lightref);
	 virtual void createDescriptorSetLayout() = 0;
	 virtual void createDescriptorSets(vk::DescriptorPool descriptorpool) = 0;
	 virtual void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) = 0;


	  void BreakDownAndUpdateModelMatrix();


	 std::vector<BufferData> vertexUniformBuffers;
	 std::vector<BufferData> fragmentUniformBuffers;

	 std::vector<void*> VertexUniformBuffersMappedMem;
	 std::vector<void*> FragmentUniformBuffersMappedMem;
	 
	 BufferData vertexBufferData;
	 BufferData indexBufferData;

	

	 TransformMatrices  transformMatrices;

	 BufferManager* bufferManager = nullptr;
	 VulkanContext* vulkanContext = nullptr;
	 Camera*        camera        = nullptr;


	 vk::CommandPool            commandPool;
	 std::vector<vk::DescriptorSet> DescriptorSets;

public:
	glm::mat4 GetModelMatrix();
	void SetModelMatrix(glm::mat4 newModelMatrix);

	vk::DescriptorSetLayout    descriptorSetLayout;

	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
};

