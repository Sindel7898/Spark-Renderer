#include "SSR_FullScreenQuad.h"
#include <stdexcept>
#include <chrono>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include "Light.h"
#include "Camera.h"
#include <random>

SSR_FullScreenQuad::SSR_FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext,Camera* cameraref, vk::CommandPool commandpool): Drawable()
{
	camera = cameraref;
	bufferManager = buffermanager;
	vulkanContext = vulkancontext;
	commandPool   = commandpool;
	CreateVertexAndIndexBuffer();
	CreateUniformBuffer();
	createDescriptorSetLayout();
}


void SSR_FullScreenQuad::CreateVertexAndIndexBuffer()
{

	VkDeviceSize VertexBufferSize = sizeof(quad[0]) * quad.size();
	vertexBufferData.BufferID = "SSR Vertex Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&vertexBufferData,quad.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(uint16_t) * quadIndices.size();
	indexBufferData.BufferID = "SSR Index Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&indexBufferData,quadIndices.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer, commandPool, vulkanContext->graphicsQueue);

}

void SSR_FullScreenQuad::CreateImage(vk::Extent3D imageExtent)
{
	  SSRImage.ImageID = "Gbuffer SSRImage Texture";
	   bufferManager->CreateImage(&SSRImage,imageExtent, vulkanContext->swapchainformat,vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

	   SSRImage.imageView = bufferManager->CreateImageView(&SSRImage, vulkanContext->swapchainformat, vk::ImageAspectFlagBits::eColor);
	   SSRImage.imageSampler = bufferManager->CreateImageSampler();
	   vk::CommandBuffer commandBuffer = bufferManager->CreateSingleUseCommandBuffer(commandPool);

	   ImageTransitionData transitionInfo{};
	   transitionInfo.oldlayout = vk::ImageLayout::eUndefined;
	   transitionInfo.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	   transitionInfo.AspectFlag = vk::ImageAspectFlagBits::eColor;
	   transitionInfo.SourceAccessflag = vk::AccessFlagBits::eNone;
	   transitionInfo.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
	   transitionInfo.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	   transitionInfo.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

	   bufferManager->TransitionImage(commandBuffer, &SSRImage, transitionInfo);

	   bufferManager->SubmitAndDestoyCommandBuffer(commandPool, commandBuffer, vulkanContext->graphicsQueue);
}

void SSR_FullScreenQuad::DestroyImage()
{

	bufferManager->DestroyImage(SSRImage);
}

void SSR_FullScreenQuad::createDescriptorSetLayout()
{
	{
		//////// Create set for SSR Pass ////////////
		vk::DescriptorSetLayoutBinding LightingSamplerBinding{};
		LightingSamplerBinding.binding = 0;
		LightingSamplerBinding.descriptorCount = 1;
		LightingSamplerBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		LightingSamplerBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding NormalSamplerBinding{};
		NormalSamplerBinding.binding = 1;
		NormalSamplerBinding.descriptorCount = 1;
		NormalSamplerBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		NormalSamplerBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding ViewSpacePositionSamplerBinding{};
		ViewSpacePositionSamplerBinding.binding = 2;
		ViewSpacePositionSamplerBinding.descriptorCount = 1;
		ViewSpacePositionSamplerBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		ViewSpacePositionSamplerBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding DepthSamplerBinding{};
		DepthSamplerBinding.binding = 3;
		DepthSamplerBinding.descriptorCount = 1;
		DepthSamplerBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		DepthSamplerBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;



		std::array<vk::DescriptorSetLayoutBinding, 4> SSRPassBinding = { LightingSamplerBinding,NormalSamplerBinding,ViewSpacePositionSamplerBinding,DepthSamplerBinding };

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(SSRPassBinding.size());
		layoutInfo.pBindings = SSRPassBinding.data();

		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
	}
}


void SSR_FullScreenQuad::createDescriptorSets(vk::DescriptorPool descriptorpool, ImageData LightingPass, ImageData NormalPass, ImageData ViewSpacePositionPass, ImageData DepthPass)
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

		vk::DescriptorImageInfo LightingPassimageInfo{};
		LightingPassimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		LightingPassimageInfo.imageView   = LightingPass.imageView;
		LightingPassimageInfo.sampler     = LightingPass.imageSampler;

		vk::WriteDescriptorSet LightingPassSamplerdescriptorWrite{};
		LightingPassSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		LightingPassSamplerdescriptorWrite.dstBinding = 0;
		LightingPassSamplerdescriptorWrite.descriptorCount = 1;
		LightingPassSamplerdescriptorWrite.dstArrayElement = 0;
		LightingPassSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		LightingPassSamplerdescriptorWrite.pImageInfo = &LightingPassimageInfo;

		vk::DescriptorImageInfo NormalPassimageInfo{};
		NormalPassimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		NormalPassimageInfo.imageView   = NormalPass.imageView;
		NormalPassimageInfo.sampler     = NormalPass.imageSampler;

		vk::WriteDescriptorSet NormalPassSamplerdescriptorWrite{};
		NormalPassSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		NormalPassSamplerdescriptorWrite.dstBinding = 1;
		NormalPassSamplerdescriptorWrite.descriptorCount = 1;
		NormalPassSamplerdescriptorWrite.dstArrayElement = 0;
		NormalPassSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		NormalPassSamplerdescriptorWrite.pImageInfo = &NormalPassimageInfo;


		vk::DescriptorImageInfo ViewSpacePositionPassimageInfo{};
		ViewSpacePositionPassimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		ViewSpacePositionPassimageInfo.imageView = ViewSpacePositionPass.imageView;
		ViewSpacePositionPassimageInfo.sampler = ViewSpacePositionPass.imageSampler;

		vk::WriteDescriptorSet ViewSpacePositionPassSamplerdescriptorWrite{};
		ViewSpacePositionPassSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		ViewSpacePositionPassSamplerdescriptorWrite.dstBinding = 2;
		ViewSpacePositionPassSamplerdescriptorWrite.descriptorCount = 1;
		ViewSpacePositionPassSamplerdescriptorWrite.dstArrayElement = 0;
		ViewSpacePositionPassSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		ViewSpacePositionPassSamplerdescriptorWrite.pImageInfo = &ViewSpacePositionPassimageInfo;



		vk::DescriptorImageInfo DepthPassimageInfo{};
		DepthPassimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		DepthPassimageInfo.imageView   = DepthPass.imageView;
		DepthPassimageInfo.sampler     = DepthPass.imageSampler;

		vk::WriteDescriptorSet DepthPassSamplerdescriptorWrite{};
		DepthPassSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		DepthPassSamplerdescriptorWrite.dstBinding = 3;
		DepthPassSamplerdescriptorWrite.descriptorCount = 1;
		DepthPassSamplerdescriptorWrite.dstArrayElement = 0;
		DepthPassSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		DepthPassSamplerdescriptorWrite.pImageInfo = &DepthPassimageInfo;

		std::array<vk::WriteDescriptorSet, 4> descriptorWrites = {
																	LightingPassSamplerdescriptorWrite,  
																	NormalPassSamplerdescriptorWrite,
																	ViewSpacePositionPassSamplerdescriptorWrite,
																	DepthPassSamplerdescriptorWrite
		                                                         };

		vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}


void SSR_FullScreenQuad::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };
	
	commandbuffer.pushConstants(pipelinelayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(camera->GetProjectionMatrix()), &camera->GetProjectionMatrix());
	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint16);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(quadIndices.size(), 1, 0, 0, 0);
}

void SSR_FullScreenQuad::CleanUp()
{
	Drawable::Destructor();

}

