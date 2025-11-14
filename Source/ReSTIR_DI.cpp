#include "ReSTIR_DI.h"
#include "VulkanContext.h"
#include "Lighting_FullScreenQuad.h"
#include "SSGI.h"



ReSTIR_DI::ReSTIR_DI(VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger, Lighting_FullScreenQuad* rLightingPass, SSGI* rssgi)
{
	bufferManager = buffermanger;
	vulkanContext = vulkancontext;
	camera = rcamera;
	commandPool = commandpool;
	LightingPass = rLightingPass;
	ssgi = rssgi;
	createDescriptorSetLayout();
}

void ReSTIR_DI::CreateImage() {

	vk::Extent3D SampledImageExtent = vk::Extent3D(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, 1);

	ResevoirImage.ImageID = " Resevoir  Image";
	bufferManager->CreateImage(&ResevoirImage, SampledImageExtent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst); 
	ResevoirImage.imageView = bufferManager->CreateImageView(&ResevoirImage, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	ResevoirImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge, true);


	vk::CommandBuffer cmd = bufferManager->CreateSingleUseCommandBuffer(commandPool);

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
	toGeneral.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
	toGeneral.SourceOnThePipeline = vk::PipelineStageFlagBits::eTransfer;
	toGeneral.DestinationOnThePipeline = vk::PipelineStageFlagBits::eComputeShader;

	bufferManager->TransitionImage(cmd, &ResevoirImage, toClear);
	cmd.clearColorImage(ResevoirImage.image, vk::ImageLayout::eTransferDstOptimal, clearColor, range);
	bufferManager->TransitionImage(cmd, &ResevoirImage, toGeneral);

	bufferManager->SubmitAndDestoyCommandBuffer(commandPool, cmd, vulkanContext->graphicsQueue);
}


void ReSTIR_DI::DestroyImage() {

	if (ResevoirImage.image)
	{
		bufferManager->DestroyImage(ResevoirImage);
	}
}


void ReSTIR_DI::createDescriptorSetLayout()
{
	{
		vk::DescriptorSetLayoutBinding LightUniformBufferLayout{};
		LightUniformBufferLayout.binding = 0;
		LightUniformBufferLayout.descriptorCount = 1;
		LightUniformBufferLayout.descriptorType = vk::DescriptorType::eUniformBuffer;
		LightUniformBufferLayout.stageFlags = vk::ShaderStageFlagBits::eCompute;

		vk::DescriptorSetLayoutBinding ResevoirStorageImageLayout{};
		ResevoirStorageImageLayout.binding = 1;
		ResevoirStorageImageLayout.descriptorCount = 1;
		ResevoirStorageImageLayout.descriptorType = vk::DescriptorType::eStorageImage;
		ResevoirStorageImageLayout.stageFlags = vk::ShaderStageFlagBits::eCompute;

		vk::DescriptorSetLayoutBinding WorldPositionImageLayout{};
		WorldPositionImageLayout.binding = 2;
		WorldPositionImageLayout.descriptorCount = 1;
		WorldPositionImageLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		WorldPositionImageLayout.stageFlags = vk::ShaderStageFlagBits::eCompute;

		vk::DescriptorSetLayoutBinding NormalImageLayout{};
		NormalImageLayout.binding = 3;
		NormalImageLayout.descriptorCount = 1;
		NormalImageLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		NormalImageLayout.stageFlags = vk::ShaderStageFlagBits::eCompute;


		vk::DescriptorSetLayoutBinding AlbedoImageLayout{};
		AlbedoImageLayout.binding = 4;
		AlbedoImageLayout.descriptorCount = 1;
		AlbedoImageLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		AlbedoImageLayout.stageFlags = vk::ShaderStageFlagBits::eCompute;


		vk::DescriptorSetLayoutBinding MaterialImageLayout{};
		MaterialImageLayout.binding = 5;
		MaterialImageLayout.descriptorCount = 1;
		MaterialImageLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		MaterialImageLayout.stageFlags = vk::ShaderStageFlagBits::eCompute;

		vk::DescriptorSetLayoutBinding BluenoiseImageLayout{};
		MaterialImageLayout.binding = 6;
		MaterialImageLayout.descriptorCount = ssgi->BlueNoiseTextures.size();
		MaterialImageLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		MaterialImageLayout.stageFlags = vk::ShaderStageFlagBits::eCompute;

		std::array<vk::DescriptorSetLayoutBinding, 6> bindings = {LightUniformBufferLayout,ResevoirStorageImageLayout,
			                                                      AlbedoImageLayout,NormalImageLayout,MaterialImageLayout,
			                                                      WorldPositionImageLayout };

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &RservoirSamplingDescriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
	}
}


void ReSTIR_DI::createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool)
{
	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, RservoirSamplingDescriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocinfo;
	allocinfo.descriptorPool = descriptorpool;
	allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocinfo.pSetLayouts = layouts.data();

	RservoirSamplingProbeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, RservoirSamplingProbeDescriptorSets.data());

	UpdateDescrptorSets();
}

void ReSTIR_DI::UpdateDescrptorSets()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		vk::DescriptorBufferInfo LightUniformBufferInfo;
		LightUniformBufferInfo.buffer = LightingPass->fragmentUniformBuffers[i].buffer;
		LightUniformBufferInfo.offset = 0;
		LightUniformBufferInfo.range = sizeof(LightUniformData) * 100;

		vk::WriteDescriptorSet LightUniformBufferDescriptorWrite{};
		LightUniformBufferDescriptorWrite.dstSet = RservoirSamplingProbeDescriptorSets[i];
		LightUniformBufferDescriptorWrite.dstBinding = 0;
		LightUniformBufferDescriptorWrite.dstArrayElement = 0;
		LightUniformBufferDescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		LightUniformBufferDescriptorWrite.descriptorCount = 1;
		LightUniformBufferDescriptorWrite.pBufferInfo = &LightUniformBufferInfo;

		vk::DescriptorImageInfo ResevoirImageInfo{};
		ResevoirImageInfo.imageLayout = vk::ImageLayout::eGeneral;
		ResevoirImageInfo.imageView = ResevoirImage.imageView;
		ResevoirImageInfo.sampler = ResevoirImage.imageSampler;

		vk::WriteDescriptorSet ResevoirImagedescriptorWrite{};
		ResevoirImagedescriptorWrite.dstSet = RservoirSamplingProbeDescriptorSets[i];
		ResevoirImagedescriptorWrite.dstBinding = 1;
		ResevoirImagedescriptorWrite.dstArrayElement = 0;
		ResevoirImagedescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
		ResevoirImagedescriptorWrite.descriptorCount = 1;
		ResevoirImagedescriptorWrite.pImageInfo = &ResevoirImageInfo;


		vk::DescriptorImageInfo PositionimageInfo{};
		PositionimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		PositionimageInfo.imageView   = LightingPass->GbufferRef->Position.imageView;
		PositionimageInfo.sampler     = LightingPass->GbufferRef->Position.imageSampler;

		vk::WriteDescriptorSet PositionSamplerdescriptorWrite{};
		PositionSamplerdescriptorWrite.dstSet = RservoirSamplingProbeDescriptorSets[i];
		PositionSamplerdescriptorWrite.dstBinding = 2;
		PositionSamplerdescriptorWrite.dstArrayElement = 0;
		PositionSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		PositionSamplerdescriptorWrite.descriptorCount = 1;
		PositionSamplerdescriptorWrite.pImageInfo = &PositionimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////
		;
		vk::DescriptorImageInfo NormalimageInfo{};
		NormalimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		NormalimageInfo.imageView = LightingPass->GbufferRef->Normal.imageView;
		NormalimageInfo.sampler =   LightingPass->GbufferRef->Normal.imageSampler;

		vk::WriteDescriptorSet NormalSamplerdescriptorWrite{};
		NormalSamplerdescriptorWrite.dstSet = RservoirSamplingProbeDescriptorSets[i];
		NormalSamplerdescriptorWrite.dstBinding = 3;
		NormalSamplerdescriptorWrite.dstArrayElement = 0;
		NormalSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		NormalSamplerdescriptorWrite.descriptorCount = 1;
		NormalSamplerdescriptorWrite.pImageInfo = &NormalimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////

		vk::DescriptorImageInfo AlbedoimageInfo{};
		AlbedoimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		AlbedoimageInfo.imageView = LightingPass->GbufferRef->Albedo.imageView;
		AlbedoimageInfo.sampler =   LightingPass->GbufferRef->Albedo.imageSampler;

		vk::WriteDescriptorSet  AlbedoSamplerdescriptorWrite{};
		AlbedoSamplerdescriptorWrite.dstSet = RservoirSamplingProbeDescriptorSets[i];
		AlbedoSamplerdescriptorWrite.dstBinding = 4;
		AlbedoSamplerdescriptorWrite.dstArrayElement = 0;
		AlbedoSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		AlbedoSamplerdescriptorWrite.descriptorCount = 1;
		AlbedoSamplerdescriptorWrite.pImageInfo = &AlbedoimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////

		vk::DescriptorImageInfo MaterialsimageInfo{};
		MaterialsimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		MaterialsimageInfo.imageView = LightingPass->GbufferRef->Materials.imageView;
		MaterialsimageInfo.sampler =   LightingPass->GbufferRef->Materials.imageSampler;

		vk::WriteDescriptorSet MaterialsSamplerdescriptorWrite{};
		MaterialsSamplerdescriptorWrite.dstSet = RservoirSamplingProbeDescriptorSets[i];
		MaterialsSamplerdescriptorWrite.dstBinding = 5;
		MaterialsSamplerdescriptorWrite.dstArrayElement = 0;
		MaterialsSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		MaterialsSamplerdescriptorWrite.descriptorCount = 1;
		MaterialsSamplerdescriptorWrite.pImageInfo = &MaterialsimageInfo;

		std::vector<vk::DescriptorImageInfo>BlueNoiseImagesInfos;

		for (int i = 0; i < ssgi->BlueNoiseTextures.size(); i++)
		{
			vk::DescriptorImageInfo BlueNoiseimageInfo{};
			BlueNoiseimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			BlueNoiseimageInfo.imageView = ssgi->BlueNoiseTextures[i].imageView;
			BlueNoiseimageInfo.sampler = ssgi->BlueNoiseTextures[i].imageSampler;

			BlueNoiseImagesInfos.push_back(BlueNoiseimageInfo);
		};

		vk::WriteDescriptorSet BlueNoiseSamplerdescriptorWrite{};
		BlueNoiseSamplerdescriptorWrite.dstSet = RservoirSamplingProbeDescriptorSets[i];
		BlueNoiseSamplerdescriptorWrite.dstBinding = 6;
		BlueNoiseSamplerdescriptorWrite.dstArrayElement = 0;
		BlueNoiseSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		BlueNoiseSamplerdescriptorWrite.descriptorCount = BlueNoiseImagesInfos.size();
		BlueNoiseSamplerdescriptorWrite.pImageInfo = BlueNoiseImagesInfos.data();


		std::array<vk::WriteDescriptorSet, 7> descriptorWrites = {
					LightUniformBufferDescriptorWrite,
					ResevoirImagedescriptorWrite,
					PositionSamplerdescriptorWrite,   
					NormalSamplerdescriptorWrite,     
					AlbedoSamplerdescriptorWrite,     
					MaterialsSamplerdescriptorWrite,BlueNoiseSamplerdescriptorWrite
		};
		vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}

void ReSTIR_DI::DispatchResevoirCandidateCalcCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex)
{
	PushConstant pushConstant;
	pushConstant.CameraPosition = glm::vec4(camera->GetPosition(),1);
	pushConstant.ScreenSize     = glm::vec4(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, ssgi->NoiseIndex,1);

	commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(PushConstant), &pushConstant);
	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, 1, &RservoirSamplingProbeDescriptorSets[imageIndex], 0, nullptr);

	uint32_t workGroupsX = (vulkanContext->swapchainExtent.width  + 31) / 32;
	uint32_t workGroupsY = (vulkanContext->swapchainExtent.height + 31) / 32;

	commandBuffer.dispatch(workGroupsX, workGroupsY, 1);
}


void ReSTIR_DI::CleanUp()
{
	vulkanContext->LogicalDevice.destroyDescriptorSetLayout(RservoirSamplingDescriptorSetLayout);
}

