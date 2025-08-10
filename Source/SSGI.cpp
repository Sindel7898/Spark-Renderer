#include "SSGI.h"
#include "VulkanContext.h"
#include "BufferManager.h"
#include "Camera.h"
#include "Light.h"

#include <stdexcept>

SSGI::SSGI(BufferManager* buffermanager, VulkanContext* vulkancontext, Camera* cameraref, vk::CommandPool commandpool) : Drawable()
{
	bufferManager = buffermanager;
	vulkanContext = vulkancontext;
	camera        = cameraref;
	commandPool   = commandpool;
	CreateVertexAndIndexBuffer();
	CreateUniformBuffer();
	createDescriptorSetLayout();
}

void SSGI::CreateVertexAndIndexBuffer()
{

	VkDeviceSize VertexBufferSize = sizeof(quad[0]) * quad.size();
	vertexBufferData.BufferID = "SSGI Vertex Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&vertexBufferData, quad.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(uint16_t) * quadIndices.size();
	indexBufferData.BufferID = "SSGI Index Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&indexBufferData, quadIndices.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer, commandPool, vulkanContext->graphicsQueue);

	//////// Load Blue Noise

	BlueNoise =  bufferManager->LoadTextureImage("../Textures/BlueNoise64Tiled.png",vk::Format::eR8G8B8A8Snorm,commandPool, vulkanContext->graphicsQueue);
}

void SSGI::CreateUniformBuffer() {

	{
		SSGI_UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		SSGI_UniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

		VkDeviceSize	RayGenuniformBufferSize = sizeof(SSGI_UniformBufferData);

		for (size_t i = 0; i < SSGI_UniformBuffers.size(); i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "RayGen Uniform Buffer" + i;
			bufferManager->CreateBuffer(&bufferdata, RayGenuniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
			SSGI_UniformBuffers[i] = bufferdata;

			SSGI_UniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
		}
	}

}


void SSGI::CreateGIImage() {

	vk::Extent3D swapchainextent = vk::Extent3D(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, 1);

	SSGIPassImage.ImageID = "RT Shadow Pass Image";
	bufferManager->CreateImage(&SSGIPassImage, swapchainextent, vulkanContext->swapchainformat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	SSGIPassImage.imageView = bufferManager->CreateImageView(&SSGIPassImage, vulkanContext->swapchainformat, vk::ImageAspectFlagBits::eColor);
	SSGIPassImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	vk::CommandBuffer commandBuffer = bufferManager->CreateSingleUseCommandBuffer(commandPool);

	ImageTransitionData transitionInfo{};
	transitionInfo.oldlayout = vk::ImageLayout::eUndefined;
	transitionInfo.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	transitionInfo.AspectFlag = vk::ImageAspectFlagBits::eColor;
	transitionInfo.SourceAccessflag = vk::AccessFlagBits::eNone;
	transitionInfo.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
	transitionInfo.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	transitionInfo.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

	bufferManager->TransitionImage(commandBuffer, &SSGIPassImage, transitionInfo);

	bufferManager->SubmitAndDestoyCommandBuffer(commandPool, commandBuffer, vulkanContext->graphicsQueue);


}
void SSGI::DestroyImage() {

	bufferManager->DestroyImage(SSGIPassImage);


}

void SSGI::createDescriptorSetLayout(){


	vk::DescriptorSetLayoutBinding NormalsSamplerLayout{};
	NormalsSamplerLayout.binding = 0;
	NormalsSamplerLayout.descriptorCount = 1;
	NormalsSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	NormalsSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;


	vk::DescriptorSetLayoutBinding ViewSpacePositionsSamplerLayout{};
	ViewSpacePositionsSamplerLayout.binding = 1;
	ViewSpacePositionsSamplerLayout.descriptorCount = 1;
	ViewSpacePositionsSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	ViewSpacePositionsSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;


	vk::DescriptorSetLayoutBinding DepthTextureSamplerLayout{};
	DepthTextureSamplerLayout.binding = 2;
	DepthTextureSamplerLayout.descriptorCount = 1;
	DepthTextureSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	DepthTextureSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutBinding LightingPassSamplerLayout{};
	LightingPassSamplerLayout.binding = 3;
	LightingPassSamplerLayout.descriptorCount = 1;
	LightingPassSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	LightingPassSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutBinding BlueNoiseSamplerLayout{};
	BlueNoiseSamplerLayout.binding = 4;
	BlueNoiseSamplerLayout.descriptorCount = 1;
	BlueNoiseSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	BlueNoiseSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutBinding  SSGIUniformBufferLayout{};
	SSGIUniformBufferLayout.binding = 5;
	SSGIUniformBufferLayout.descriptorCount = 1;
	SSGIUniformBufferLayout.descriptorType = vk::DescriptorType::eUniformBuffer;
	SSGIUniformBufferLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;


	std::array<vk::DescriptorSetLayoutBinding, 6> bindings = { NormalsSamplerLayout,ViewSpacePositionsSamplerLayout,
												                DepthTextureSamplerLayout,  LightingPassSamplerLayout,
		                                                        BlueNoiseSamplerLayout,  SSGIUniformBufferLayout };

	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();


	if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess)
	{
		throw std::runtime_error("Failed to create descriptorset layout!");
	}

}

void SSGI::createDescriptorSets(vk::DescriptorPool descriptorpool,GBuffer gbuffer,ImageData LightingPass, ImageData DepthImage)
{
	{
		// create sets from the pool based on the layout
		// 	     
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

			vk::DescriptorImageInfo NormalimageInfo{};
			NormalimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			NormalimageInfo.imageView   = gbuffer.ViewSpaceNormal.imageView;
			NormalimageInfo.sampler     = gbuffer.ViewSpaceNormal.imageSampler;

			vk::WriteDescriptorSet NormalSamplerdescriptorWrite{};
			NormalSamplerdescriptorWrite.dstSet = DescriptorSets[i];
			NormalSamplerdescriptorWrite.dstBinding = 0;
			NormalSamplerdescriptorWrite.dstArrayElement = 0;
			NormalSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			NormalSamplerdescriptorWrite.descriptorCount = 1;
			NormalSamplerdescriptorWrite.pImageInfo = &NormalimageInfo;

			vk::DescriptorImageInfo ViewSpacePositionimageInfo{};
			ViewSpacePositionimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			ViewSpacePositionimageInfo.imageView = gbuffer.ViewSpacePosition.imageView;
			ViewSpacePositionimageInfo.sampler = gbuffer.ViewSpacePosition.imageSampler;

			vk::WriteDescriptorSet ViewSpacePositionSamplerdescriptorWrite{};
			ViewSpacePositionSamplerdescriptorWrite.dstSet = DescriptorSets[i];
			ViewSpacePositionSamplerdescriptorWrite.dstBinding = 1;
			ViewSpacePositionSamplerdescriptorWrite.dstArrayElement = 0;
			ViewSpacePositionSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			ViewSpacePositionSamplerdescriptorWrite.descriptorCount = 1;
			ViewSpacePositionSamplerdescriptorWrite.pImageInfo = &ViewSpacePositionimageInfo;


			vk::DescriptorImageInfo  DepthimageInfo{};
			DepthimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			DepthimageInfo.imageView = DepthImage.imageView;
			DepthimageInfo.sampler = DepthImage.imageSampler;

			vk::WriteDescriptorSet DepthimageSamplerdescriptorWrite{};
			DepthimageSamplerdescriptorWrite.dstSet = DescriptorSets[i];
			DepthimageSamplerdescriptorWrite.dstBinding = 2;
			DepthimageSamplerdescriptorWrite.dstArrayElement = 0;
			DepthimageSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			DepthimageSamplerdescriptorWrite.descriptorCount = 1;
			DepthimageSamplerdescriptorWrite.pImageInfo = &DepthimageInfo;


			/////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorImageInfo LightingPassimageInfo{};
			LightingPassimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			LightingPassimageInfo.imageView = LightingPass.imageView;
			LightingPassimageInfo.sampler   = LightingPass.imageSampler;

			vk::WriteDescriptorSet LightingPassSamplerdescriptorWrite{};
			LightingPassSamplerdescriptorWrite.dstSet = DescriptorSets[i];
			LightingPassSamplerdescriptorWrite.dstBinding = 3;
			LightingPassSamplerdescriptorWrite.dstArrayElement = 0;
			LightingPassSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			LightingPassSamplerdescriptorWrite.descriptorCount = 1;
			LightingPassSamplerdescriptorWrite.pImageInfo = &LightingPassimageInfo;


			vk::DescriptorImageInfo BlueNoiseimageInfo{};
			BlueNoiseimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			BlueNoiseimageInfo.imageView = BlueNoise.imageView;
			BlueNoiseimageInfo.sampler = BlueNoise.imageSampler;

			vk::WriteDescriptorSet BlueNoiseSamplerdescriptorWrite{};
			BlueNoiseSamplerdescriptorWrite.dstSet = DescriptorSets[i];
			BlueNoiseSamplerdescriptorWrite.dstBinding = 4;
			BlueNoiseSamplerdescriptorWrite.dstArrayElement = 0;
			BlueNoiseSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			BlueNoiseSamplerdescriptorWrite.descriptorCount = 1;
			BlueNoiseSamplerdescriptorWrite.pImageInfo = &BlueNoiseimageInfo;


			/////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorBufferInfo  fragmentuniformbufferInfo{};
			fragmentuniformbufferInfo.buffer = SSGI_UniformBuffers[i].buffer;
			fragmentuniformbufferInfo.offset = 0;
			fragmentuniformbufferInfo.range = sizeof(SSGI_UniformBufferData);

			vk::WriteDescriptorSet fragmentUniformdescriptorWrite{};
			fragmentUniformdescriptorWrite.dstSet = DescriptorSets[i];
			fragmentUniformdescriptorWrite.dstBinding = 5;
			fragmentUniformdescriptorWrite.dstArrayElement = 0;
			fragmentUniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
			fragmentUniformdescriptorWrite.descriptorCount = 1;
			fragmentUniformdescriptorWrite.pBufferInfo = &fragmentuniformbufferInfo;


			std::array<vk::WriteDescriptorSet, 6> descriptorWrites{ NormalSamplerdescriptorWrite,ViewSpacePositionSamplerdescriptorWrite,DepthimageSamplerdescriptorWrite,
																	LightingPassSamplerdescriptorWrite,BlueNoiseSamplerdescriptorWrite,
																	fragmentUniformdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}
}

void SSGI::UpdateUniformBuffer(uint32_t currentImage, std::vector<std::shared_ptr<Light>>& lightref)
{
	SSGI_UniformBufferData SSGI_UniformBufferData;
 	SSGI_UniformBufferData.ProjectionMatrix = camera->GetProjectionMatrix();
	SSGI_UniformBufferData.ProjectionMatrix[1][1] *= -1;

	memcpy(SSGI_UniformBuffersMappedMem[currentImage], &SSGI_UniformBufferData, sizeof(SSGI_UniformBufferData));
}


void SSGI::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };

	commandbuffer.pushConstants(pipelinelayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(camera->GetProjectionMatrix()), &camera->GetProjectionMatrix());
	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint16);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(quadIndices.size(), 1, 0, 0, 0);
}

void SSGI::CleanUp()
{
	if (bufferManager)
	{
		bufferManager->DestroyImage(SSGIPassImage);

	}
}



