#include "SSGI.h"
#include "VulkanContext.h"
#include "BufferManager.h"
#include "Camera.h"
#include "Light.h"
#include "Lighting_RTX.h"

#include <stdexcept>

SSGI::SSGI(BufferManager* buffermanager, VulkanContext* vulkancontext, Camera* cameraref, vk::CommandPool commandpool, Lighting_RTX* lighting)
{
	bufferManager = buffermanager;
	vulkanContext = vulkancontext;
	camera        = cameraref;
	commandPool   = commandpool;
	lightingref   = lighting;
	CreateNoiseTextures();
	CreateUniformBuffer();
	createDescriptorSetLayout();
}

SSGI::~SSGI()
{
	if (bufferManager)
	{
		for (ImageData noise : BlueNoiseTextures)
		{
			bufferManager->DestroyImage(noise);
		}
		BlueNoiseTextures.clear();

		for (size_t i = 0; i < ComputeUniformBuffers.size(); i++)
		{
			if (ComputeUniformBuffers[i].buffer) {
				bufferManager->UnmapMemory(ComputeUniformBuffers[i]);
				bufferManager->DestroyBuffer(ComputeUniformBuffers[i]);
			}
		}

		ComputeUniformBuffers.clear();
		ComputeUniformBuffersMappedMem.clear();

		if (descriptorSetLayout) {
			vulkanContext->LogicalDevice.destroyDescriptorSetLayout(descriptorSetLayout);
		}
	}
}


void SSGI::CreateNoiseTextures()
{

	for (int i = 0; i < 63; i++)
	{
		ImageData Noise;
		std::string TextureType = ".png";
		std::string NoisePath = "../Textures/BlueNoise/stbn_unitvec3_cosine_2Dx1D_128x128x64_" + std::to_string(i) + TextureType;
		Noise = bufferManager->LoadTextureImage(NoisePath, vk::Format::eR8G8B8A8Snorm, commandPool, vulkanContext->graphicsQueue);
		BlueNoiseTextures.push_back(Noise);

	}
}

void SSGI::CreateUniformBuffer() {

	{
		ComputeUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		ComputeUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

		VkDeviceSize	RayGenuniformBufferSize = sizeof(SSGI_UniformBufferData);

		for (size_t i = 0; i < ComputeUniformBuffers.size(); i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "SSGI Uniform Buffer" + i;
			bufferManager->CreateBuffer(&bufferdata, RayGenuniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
			ComputeUniformBuffers[i] = bufferdata;

			ComputeUniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
		}
	}

}


void SSGI::CreateGIImage() {

	SSGI_ImageFullResolution   = vk::Extent3D(vulkanContext->swapchainExtent.width/2, vulkanContext->swapchainExtent.height/2, 1);
	
	SSGIPassImage.ImageID = "SSGI Pass Image";
	bufferManager->CreateImage(&SSGIPassImage, SSGI_ImageFullResolution, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage,false);
	SSGIPassImage.imageView = bufferManager->CreateImageView(&SSGIPassImage, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
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

	{
		vk::ClearColorValue clearColor(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f});
		vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

		ImageTransitionData toClear{};
		toClear.oldlayout = vk::ImageLayout::eUndefined;
		toClear.newlayout = vk::ImageLayout::eTransferDstOptimal;
		toClear.AspectFlag = vk::ImageAspectFlagBits::eColor;
		toClear.SourceAccessflag = vk::AccessFlagBits::eNone;
		toClear.DestinationAccessflag = vk::AccessFlagBits::eTransferWrite;
		toClear.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
		toClear.DestinationOnThePipeline = vk::PipelineStageFlagBits::eTransfer;

		ImageTransitionData toGeneral{};
		toGeneral.oldlayout = vk::ImageLayout::eTransferDstOptimal;
		toGeneral.newlayout = vk::ImageLayout::eGeneral;
		toGeneral.AspectFlag = vk::ImageAspectFlagBits::eColor;
		toGeneral.SourceAccessflag = vk::AccessFlagBits::eTransferWrite;
		toGeneral.DestinationAccessflag = vk::AccessFlagBits::eTransferRead; 
		toGeneral.SourceOnThePipeline = vk::PipelineStageFlagBits::eTransfer;
		toGeneral.DestinationOnThePipeline = vk::PipelineStageFlagBits::eTransfer; 

		bufferManager->TransitionImage(commandBuffer, &SSGIPassImage, toClear);
		commandBuffer.clearColorImage(SSGIPassImage.image, vk::ImageLayout::eTransferDstOptimal, clearColor, range);
		bufferManager->TransitionImage(commandBuffer, &SSGIPassImage, toGeneral);
	}


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
	NormalsSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eCompute;


	vk::DescriptorSetLayoutBinding ViewSpacePositionsSamplerLayout{};
	ViewSpacePositionsSamplerLayout.binding = 1;
	ViewSpacePositionsSamplerLayout.descriptorCount = 1;
	ViewSpacePositionsSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	ViewSpacePositionsSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eCompute;


	vk::DescriptorSetLayoutBinding DepthTextureSamplerLayout{};
	DepthTextureSamplerLayout.binding = 2;
	DepthTextureSamplerLayout.descriptorCount = 1;
	DepthTextureSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	DepthTextureSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eCompute;


	vk::DescriptorSetLayoutBinding AlbedoPassSamplerLayout{};
	AlbedoPassSamplerLayout.binding = 3;
	AlbedoPassSamplerLayout.descriptorCount = 1;
	AlbedoPassSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	AlbedoPassSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eCompute;


	vk::DescriptorSetLayoutBinding LightingPassSamplerLayout{};
	LightingPassSamplerLayout.binding = 4;
	LightingPassSamplerLayout.descriptorCount = 1;
	LightingPassSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	LightingPassSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eCompute;

	vk::DescriptorSetLayoutBinding BlueNoiseSamplerLayout{};
	BlueNoiseSamplerLayout.binding = 5;
	BlueNoiseSamplerLayout.descriptorCount = BlueNoiseTextures.size();
	BlueNoiseSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	BlueNoiseSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eCompute;

	vk::DescriptorSetLayoutBinding  SSGIUniformBufferLayout{};
	SSGIUniformBufferLayout.binding = 6;
	SSGIUniformBufferLayout.descriptorCount = 1;
	SSGIUniformBufferLayout.descriptorType = vk::DescriptorType::eUniformBuffer;
	SSGIUniformBufferLayout.stageFlags = vk::ShaderStageFlagBits::eCompute;

	vk::DescriptorSetLayoutBinding SSGIImageLayout{};
	SSGIImageLayout.binding = 7;
	SSGIImageLayout.descriptorCount = 1;
	SSGIImageLayout.descriptorType = vk::DescriptorType::eStorageImage;
	SSGIImageLayout.stageFlags = vk::ShaderStageFlagBits::eCompute;

	std::array<vk::DescriptorSetLayoutBinding, 8> bindings = { NormalsSamplerLayout,ViewSpacePositionsSamplerLayout,
												                DepthTextureSamplerLayout, AlbedoPassSamplerLayout, LightingPassSamplerLayout,
		                                                        BlueNoiseSamplerLayout,  SSGIUniformBufferLayout,SSGIImageLayout };

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
			NormalimageInfo.imageLayout = vk::ImageLayout::eGeneral;
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
			ViewSpacePositionimageInfo.imageLayout = vk::ImageLayout::eGeneral;
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
			vk::DescriptorImageInfo AlbedoPassimageInfo{};
			AlbedoPassimageInfo.imageLayout = vk::ImageLayout::eGeneral;
			AlbedoPassimageInfo.imageView = gbuffer.Albedo.imageView;
			AlbedoPassimageInfo.sampler =  gbuffer.Albedo.imageSampler;

			vk::WriteDescriptorSet AlbedoPassSamplerdescriptorWrite{};
			AlbedoPassSamplerdescriptorWrite.dstSet = DescriptorSets[i];
			AlbedoPassSamplerdescriptorWrite.dstBinding = 3;
			AlbedoPassSamplerdescriptorWrite.dstArrayElement = 0;
			AlbedoPassSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			AlbedoPassSamplerdescriptorWrite.descriptorCount = 1;
			AlbedoPassSamplerdescriptorWrite.pImageInfo = &AlbedoPassimageInfo;


			vk::DescriptorImageInfo LightingPassimageInfo{};
			LightingPassimageInfo.imageLayout = vk::ImageLayout::eGeneral;
			LightingPassimageInfo.imageView = LightingPass.imageView;
			LightingPassimageInfo.sampler   = LightingPass.imageSampler;

			vk::WriteDescriptorSet LightingPassSamplerdescriptorWrite{};
			LightingPassSamplerdescriptorWrite.dstSet = DescriptorSets[i];
			LightingPassSamplerdescriptorWrite.dstBinding = 4;
			LightingPassSamplerdescriptorWrite.dstArrayElement = 0;
			LightingPassSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			LightingPassSamplerdescriptorWrite.descriptorCount = 1;
			LightingPassSamplerdescriptorWrite.pImageInfo = &LightingPassimageInfo;


			std::vector<vk::DescriptorImageInfo>BlueNoiseImagesInfos;

			for (int i = 0; i < BlueNoiseTextures.size(); i++)
			{
				vk::DescriptorImageInfo BlueNoiseimageInfo{};
				BlueNoiseimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				BlueNoiseimageInfo.imageView = BlueNoiseTextures[i].imageView;
				BlueNoiseimageInfo.sampler = BlueNoiseTextures[i].imageSampler;

				BlueNoiseImagesInfos.push_back(BlueNoiseimageInfo);
			};

			vk::WriteDescriptorSet BlueNoiseSamplerdescriptorWrite{};
			BlueNoiseSamplerdescriptorWrite.dstSet = DescriptorSets[i];
			BlueNoiseSamplerdescriptorWrite.dstBinding = 5;
			BlueNoiseSamplerdescriptorWrite.dstArrayElement = 0;
			BlueNoiseSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			BlueNoiseSamplerdescriptorWrite.descriptorCount = BlueNoiseImagesInfos.size();
			BlueNoiseSamplerdescriptorWrite.pImageInfo = BlueNoiseImagesInfos.data();


			/////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorBufferInfo  fragmentuniformbufferInfo{};
			fragmentuniformbufferInfo.buffer = ComputeUniformBuffers[i].buffer;
			fragmentuniformbufferInfo.offset = 0;
			fragmentuniformbufferInfo.range = sizeof(SSGI_UniformBufferData);

			vk::WriteDescriptorSet fragmentUniformdescriptorWrite{};
			fragmentUniformdescriptorWrite.dstSet = DescriptorSets[i];
			fragmentUniformdescriptorWrite.dstBinding = 6;
			fragmentUniformdescriptorWrite.dstArrayElement = 0;
			fragmentUniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
			fragmentUniformdescriptorWrite.descriptorCount = 1;
			fragmentUniformdescriptorWrite.pBufferInfo = &fragmentuniformbufferInfo;


			vk::DescriptorImageInfo SSGIStorgeimageInfo{};
			SSGIStorgeimageInfo.imageLayout = vk::ImageLayout::eGeneral;
			SSGIStorgeimageInfo.imageView = SSGIPassImage.imageView;
			SSGIStorgeimageInfo.sampler = SSGIPassImage.imageSampler;

			vk::WriteDescriptorSet SSGIStorgedescriptorWrite{};
			SSGIStorgedescriptorWrite.dstSet = DescriptorSets[i];
			SSGIStorgedescriptorWrite.dstBinding = 7;
			SSGIStorgedescriptorWrite.dstArrayElement = 0;
			SSGIStorgedescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			SSGIStorgedescriptorWrite.descriptorCount = 1;
			SSGIStorgedescriptorWrite.pImageInfo = &SSGIStorgeimageInfo;

			std::array<vk::WriteDescriptorSet, 8> descriptorWrites{ NormalSamplerdescriptorWrite,ViewSpacePositionSamplerdescriptorWrite,DepthimageSamplerdescriptorWrite,
																	AlbedoPassSamplerdescriptorWrite,LightingPassSamplerdescriptorWrite,BlueNoiseSamplerdescriptorWrite,
																	fragmentUniformdescriptorWrite,SSGIStorgedescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}
}

void SSGI::UpdateUniformBuffer(uint32_t currentImage, float DeltaTime)
{
	 NoiseIndex = (NoiseIndex + 1)  % BlueNoiseTextures.size();

	SSGI_UniformBufferData SSGI_UniformBufferData;
 	SSGI_UniformBufferData.ProjectionMatrix = camera->GetProjectionMatrix();
	SSGI_UniformBufferData.ProjectionMatrix[1][1] *= -1;
	SSGI_UniformBufferData.BlueNoiseImageIndex_WithPadding = glm::vec4(NoiseIndex, DeltaTime, vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height);

	memcpy(ComputeUniformBuffersMappedMem[currentImage], &SSGI_UniformBufferData, sizeof(SSGI_UniformBufferData));

	if (LastCameraMatrix != camera->GetViewMatrix())
	{
		vulkanContext->ResetTemporalAccumilation();

		LastCameraMatrix = camera->GetViewMatrix();
	}

	vulkanContext->AccumilationCount++;
}

void SSGI::ComputeSSGI(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
{

	if (lightingref->GISolutionIndex == 1 || lightingref->GISolutionIndex == 2)
	{
		uint32_t workGroupsX = (SSGI_ImageFullResolution.width + 31) / 32;
		uint32_t workGroupsY = (SSGI_ImageFullResolution.height + 31) / 32;

		commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
		commandbuffer.dispatch(workGroupsX, workGroupsY, 1);
	}
}



