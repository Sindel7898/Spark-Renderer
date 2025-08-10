#include "FXAA_FullScreenQuad.h"
#include <stdexcept>
#include <chrono>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include "Light.h"
#include "Camera.h"
#include <random>

FXAA_FullScreenQuad::FXAA_FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext,Camera* cameraref, vk::CommandPool commandpool): Drawable()
{
	camera = cameraref;
	bufferManager = buffermanager;
	vulkanContext = vulkancontext;
	commandPool   = commandpool;
	CreateVertexAndIndexBuffer();
	CreateUniformBuffer();
	createDescriptorSetLayout();
}


void FXAA_FullScreenQuad::CreateVertexAndIndexBuffer()
{

	VkDeviceSize VertexBufferSize = sizeof(quad[0]) * quad.size();
	vertexBufferData.BufferID = "FXAA Vertex Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&vertexBufferData,quad.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(uint16_t) * quadIndices.size();
	indexBufferData.BufferID = "FXAA Index Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&indexBufferData,quadIndices.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer, commandPool, vulkanContext->graphicsQueue);

}

void FXAA_FullScreenQuad::CreateImage(vk::Extent3D imageExtent)
{
	   FxaaImage.ImageID = "Gbuffer FxaaImage Texture";
	   bufferManager->CreateImage(&FxaaImage,imageExtent, vulkanContext->swapchainformat,vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

	   FxaaImage.imageView = bufferManager->CreateImageView(&FxaaImage, vulkanContext->swapchainformat, vk::ImageAspectFlagBits::eColor);
	   FxaaImage.imageSampler = bufferManager->CreateImageSampler();

	   vk::CommandBuffer commandBuffer = bufferManager->CreateSingleUseCommandBuffer(commandPool);

	   ImageTransitionData transitionInfo{};
	   transitionInfo.oldlayout = vk::ImageLayout::eUndefined;
	   transitionInfo.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	   transitionInfo.AspectFlag = vk::ImageAspectFlagBits::eColor;
	   transitionInfo.SourceAccessflag = vk::AccessFlagBits::eNone;
	   transitionInfo.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
	   transitionInfo.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	   transitionInfo.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

	   bufferManager->TransitionImage(commandBuffer, &FxaaImage, transitionInfo);

	   bufferManager->SubmitAndDestoyCommandBuffer(commandPool, commandBuffer, vulkanContext->graphicsQueue);
}

void FXAA_FullScreenQuad::DestroyImage()
{

	bufferManager->DestroyImage(FxaaImage);
}

void FXAA_FullScreenQuad::createDescriptorSetLayout()
{
	{
		//////// Create set for SSAO Pass ////////////
		vk::DescriptorSetLayoutBinding LightingSamplerBinding{};
		LightingSamplerBinding.binding = 0;
		LightingSamplerBinding.descriptorCount = 1;
		LightingSamplerBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		LightingSamplerBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		std::array<vk::DescriptorSetLayoutBinding, 1> FXAAPassBinding = { LightingSamplerBinding };

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(FXAAPassBinding.size());
		layoutInfo.pBindings = FXAAPassBinding.data();

		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
	}
}


void FXAA_FullScreenQuad::createDescriptorSets(vk::DescriptorPool descriptorpool, ImageData LightingPass)
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

		std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {
																	LightingPassSamplerdescriptorWrite,        // binding 1

		                                                         };

		vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}


void FXAA_FullScreenQuad::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };

	bFXAA_Padding = glm::vec4(bFXAA, 0.0f, 0.0f, 0.0f);

	commandbuffer.pushConstants(pipelinelayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(bFXAA_Padding), &bFXAA_Padding);
	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint16);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(quadIndices.size(), 1, 0, 0, 0);
}

void FXAA_FullScreenQuad::CleanUp()
{
	Drawable::Destructor();

}

