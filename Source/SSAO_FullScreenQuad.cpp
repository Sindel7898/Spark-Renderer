#include "SSAO_FullScreenQuad.h"
#include <stdexcept>
#include <chrono>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include "Light.h"
#include "Camera.h"
#include <random>

SSA0_FullScreenQuad::SSA0_FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext,Camera* cameraref, vk::CommandPool commandpool): Drawable()
{
	camera = cameraref;
	bufferManager = buffermanager;
	vulkanContext = vulkancontext;
	commandPool   = commandpool;
	CreateVertexAndIndexBuffer();
	CreateUniformBuffer();
	CreateKernel();
	createDescriptorSetLayout();


	SSAOuniformbuffer.KernelSizeRadiusBiasAndBool = glm::vec4(KernelSize, Radius, Bias, bShouldSSAO);
}


void SSA0_FullScreenQuad::CreateVertexAndIndexBuffer()
{

	VkDeviceSize VertexBufferSize = sizeof(quad[0]) * quad.size();
	vertexBufferData.BufferID = "SSAO Vertex Buffer";
    bufferManager->CreateGPUOptimisedBuffer(&vertexBufferData,quad.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(uint16_t) * quadIndices.size();
	indexBufferData.BufferID = "SSAO Index Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&indexBufferData,quadIndices.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer, commandPool, vulkanContext->graphicsQueue);

}

void SSA0_FullScreenQuad::CreateUniformBuffer()
{
	fragmentUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	FragmentUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

	VkDeviceSize FragmentBufferSize = sizeof(SSAOUniformBuffer);

	for (size_t i = 0; i < fragmentUniformBuffers.size(); i++)
	{
		BufferData bufferdata;
		bufferdata.BufferID = "SSAO Vertex Uniform Buffer" + i;

	  bufferManager->CreateBuffer(&bufferdata,FragmentBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
	  fragmentUniformBuffers[i] = bufferdata;
	  FragmentUniformBuffersMappedMem[i] = bufferManager->MapMemory(fragmentUniformBuffers[i]);
	}

	/////////////////////////////////
	const int noiseSize = 4;

	std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
	std::default_random_engine generator;

	for (unsigned int i = 0; i <  noiseSize * noiseSize; i++)
	{
		glm::vec4 noise(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			0.0f, 0.0f);
		ssaoNoise.push_back(noise);
	}

	vk::DeviceSize imagesize = 4 * 4 * sizeof(glm::vec4);

	NoiseTexture = bufferManager->CreateTextureImage(ssaoNoise.data(), imagesize, 4, 4, vk::Format::eR32G32B32A32Sfloat, commandPool, vulkanContext->graphicsQueue);

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

		vk::DescriptorSetLayoutBinding NoiseSamplerBinding{};
		NoiseSamplerBinding.binding = 2;
		NoiseSamplerBinding.descriptorCount = 1;
		NoiseSamplerBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		NoiseSamplerBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;


		vk::DescriptorSetLayoutBinding SSAOUniformBufferBinding{};
		SSAOUniformBufferBinding.binding = 3;
		SSAOUniformBufferBinding.descriptorCount = 1;
		SSAOUniformBufferBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
		SSAOUniformBufferBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		std::array<vk::DescriptorSetLayoutBinding, 4> SSAOPassBinding = { PositionSamplerBinding ,NormalSamplerBinding,NoiseSamplerBinding,SSAOUniformBufferBinding };

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(SSAOPassBinding.size());
		layoutInfo.pBindings = SSAOPassBinding.data();

		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
	}
}

void SSA0_FullScreenQuad::CreateKernel()
{
	std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
	std::default_random_engine generator;

	for (unsigned int i = 0; i < KernelSize; i++)
	{
		glm::vec4 sample(randomFloats(generator) * 2.0 - 1.0,
			             randomFloats(generator) * 2.0 - 1.0,
			             randomFloats(generator) ,
			             0.0f);

		
		sample = glm::normalize(sample); // Normalize first
		sample *= randomFloats(generator); // Then scale

		float scale = (float)i / KernelSize;
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		SSAOuniformbuffer.ssaoKernel[i] = sample;
	}
}

void SSA0_FullScreenQuad::UpdataeUniformBufferData()
{
	SSAOuniformbuffer.CameraProjMatrix = camera->GetProjectionMatrix();
	SSAOuniformbuffer.CameraProjMatrix[1][1] *= -1;

	SSAOuniformbuffer.CameraViewMatrix = camera->GetViewMatrix();

	SSAOuniformbuffer.KernelSizeRadiusBiasAndBool = glm::vec4(KernelSize, Radius, Bias, bShouldSSAO);

	for (size_t i = 0; i < FragmentUniformBuffersMappedMem.size(); i++)
	{
		memcpy(FragmentUniformBuffersMappedMem[i], &SSAOuniformbuffer, sizeof(SSAOuniformbuffer));
	}
}



float SSA0_FullScreenQuad::lerp(float a, float b, float f)
{
	return a + f * (b - a);
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
		PositionimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		PositionimageInfo.imageView = Gbuffer.ViewSpacePosition.imageView;
		PositionimageInfo.sampler = Gbuffer.ViewSpacePosition.imageSampler;

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
        NormalimageInfo.imageLayout = vk::ImageLayout::eGeneral;
        NormalimageInfo.imageView = Gbuffer.ViewSpaceNormal.imageView;
        NormalimageInfo.sampler = Gbuffer.ViewSpaceNormal.imageSampler;
        
        vk::WriteDescriptorSet NormalSamplerdescriptorWrite{};
        NormalSamplerdescriptorWrite.dstSet = DescriptorSets[i];
        NormalSamplerdescriptorWrite.dstBinding = 1;
        NormalSamplerdescriptorWrite.dstArrayElement = 0;
        NormalSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        NormalSamplerdescriptorWrite.descriptorCount = 1;
        NormalSamplerdescriptorWrite.pImageInfo = &NormalimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////


		vk::DescriptorImageInfo NoiseTextureimageInfo{};
		NoiseTextureimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		NoiseTextureimageInfo.imageView = NoiseTexture.imageView;
		NoiseTextureimageInfo.sampler = NoiseTexture.imageSampler;

		vk::WriteDescriptorSet NoiseSamplerdescriptorWrite{};
		NoiseSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		NoiseSamplerdescriptorWrite.dstBinding = 2;
		NoiseSamplerdescriptorWrite.dstArrayElement = 0;
		NoiseSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		NoiseSamplerdescriptorWrite.descriptorCount = 1;
		NoiseSamplerdescriptorWrite.pImageInfo = &NoiseTextureimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////

		vk::DescriptorBufferInfo fragmentUniformBufferInfo{};
		fragmentUniformBufferInfo.buffer = fragmentUniformBuffers[i].buffer;
		fragmentUniformBufferInfo.offset = 0;
		fragmentUniformBufferInfo.range = sizeof(SSAOUniformBuffer);

		vk::WriteDescriptorSet fragmentUniformbBufferdescriptorWrite{};
		fragmentUniformbBufferdescriptorWrite.dstSet = DescriptorSets[i];
		fragmentUniformbBufferdescriptorWrite.dstBinding = 3;
		fragmentUniformbBufferdescriptorWrite.dstArrayElement = 0;
		fragmentUniformbBufferdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		fragmentUniformbBufferdescriptorWrite.descriptorCount = 1;
		fragmentUniformbBufferdescriptorWrite.pBufferInfo = &fragmentUniformBufferInfo;


		std::array<vk::WriteDescriptorSet, 4> descriptorWrites = {
	                                                                PositionSamplerdescriptorWrite,        // binding 1
	                                                                NormalSamplerdescriptorWrite,          // binding 2
																	NoiseSamplerdescriptorWrite,
																	fragmentUniformbBufferdescriptorWrite
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
	bufferManager->DestroyImage(NoiseTexture);
	Drawable::Destructor();
}

