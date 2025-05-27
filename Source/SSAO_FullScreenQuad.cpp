#include "SSAO_FullScreenQuad.h"
#include <stdexcept>
#include <chrono>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include "Light.h"
#include "Camera.h"

SSA0_FullScreenQuad::SSA0_FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext,Camera* cameraref, vk::CommandPool commandpool): Drawable()
{
	camera = cameraref;
	bufferManager = buffermanager;
	vulkanContext = vulkancontext;
	commandPool   = commandpool;
	createSSAOImage();
	CreateVertexAndIndexBuffer();
	CreateUniformBuffer();
	createDescriptorSetLayout();
}


void SSA0_FullScreenQuad::CreateVertexAndIndexBuffer()
{

	VkDeviceSize VertexBufferSize = sizeof(quad[0]) * quad.size();
	vertexBufferData = bufferManager->CreateGPUOptimisedBuffer(quad.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(uint16_t) * quadIndices.size();
	indexBufferData = bufferManager->CreateGPUOptimisedBuffer(quadIndices.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer, commandPool, vulkanContext->graphicsQueue);

}


void SSA0_FullScreenQuad::createDescriptorSetLayout()
{
	{
		//////// Create set for SSAO Pass ////////////
		vk::DescriptorSetLayoutBinding PositionSamplerBinding{};
		PositionSamplerBinding.binding = 0;
		PositionSamplerBinding.descriptorCount = 1;
		PositionSamplerBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		PositionSamplerBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding NormalSamplerBinding{};
		NormalSamplerBinding.binding = 1;
		NormalSamplerBinding.descriptorCount = 1;
		NormalSamplerBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		NormalSamplerBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		std::array<vk::DescriptorSetLayoutBinding, 2> SSAOPassBinding = { PositionSamplerBinding ,NormalSamplerBinding };

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(SSAOPassBinding.size());
		layoutInfo.pBindings = SSAOPassBinding.data();

		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
	}
}

void SSA0_FullScreenQuad::createSSAOImage()
{


}

void SSA0_FullScreenQuad::createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, GBuffer Gbuffer)
{
	// create sets from the pool based on the layout
	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocinfo;
	allocinfo.descriptorPool = descriptorpool;
	allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocinfo.pSetLayouts = layouts.data();

	DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, DescriptorSets.data());

	////////////////////////////////////////////////////////////////////////////////////////////////
	//specifies what exactly to send
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		/////////////////////////////////////////////////////////////////////////////////////
		vk::DescriptorImageInfo PositionimageInfo{};
		PositionimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		PositionimageInfo.imageView = Gbuffer.Position.imageView;
		PositionimageInfo.sampler = Gbuffer.Position.imageSampler;

		vk::WriteDescriptorSet PositionSamplerdescriptorWrite{};
		PositionSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		PositionSamplerdescriptorWrite.dstBinding = 0;
		PositionSamplerdescriptorWrite.descriptorCount = 1;
		PositionSamplerdescriptorWrite.dstArrayElement = 0;
		PositionSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		PositionSamplerdescriptorWrite.pImageInfo = &PositionimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////
;
        vk::DescriptorImageInfo NormalimageInfo{};
        NormalimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        NormalimageInfo.imageView = Gbuffer.Normal.imageView;
        NormalimageInfo.sampler = Gbuffer.Normal.imageSampler;
        
        vk::WriteDescriptorSet NormalSamplerdescriptorWrite{};
        NormalSamplerdescriptorWrite.dstSet = DescriptorSets[i];
        NormalSamplerdescriptorWrite.dstBinding = 1;
        NormalSamplerdescriptorWrite.dstArrayElement = 0;
        NormalSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        NormalSamplerdescriptorWrite.descriptorCount = 1;
        NormalSamplerdescriptorWrite.pImageInfo = &NormalimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////

		std::array<vk::WriteDescriptorSet, 2> descriptorWrites = {
	                                                                PositionSamplerdescriptorWrite,        // binding 1
	                                                                NormalSamplerdescriptorWrite,          // binding 2
		                                                         };

		vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}

void SSA0_FullScreenQuad::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };
	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint16);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(quadIndices.size(), 1, 0, 0, 0);
}

void SSA0_FullScreenQuad::CleanUp()
{

	Drawable::Destructor();
}

