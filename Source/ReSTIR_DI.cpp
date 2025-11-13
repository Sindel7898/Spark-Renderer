#include "ReSTIR_DI.h"
#include "VulkanContext.h"
#include "Lighting_FullScreenQuad.h"



ReSTIR_DI::ReSTIR_DI(VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger, Lighting_FullScreenQuad* rLightingPass)
{
	bufferManager = buffermanger;
	vulkanContext = vulkancontext;
	camera = rcamera;
	commandPool = commandpool;
	LightingPass = rLightingPass;

	createDescriptorSetLayout();
}

void ReSTIR_DI::CreateImage() {

	// Note: This extent is deliberately different from the one in CreateAtlasImages
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


void ReSTIR_DI::DestroyAtlasImages() {

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

		std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {LightUniformBufferLayout,ResevoirStorageImageLayout };

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

		std::array<vk::WriteDescriptorSet, 2> descriptorWrites = {LightUniformBufferDescriptorWrite,ResevoirImagedescriptorWrite };

		vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}

void ReSTIR_DI::DispatchResevoirCandidateCalcCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex)
{

	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, 1, &RservoirSamplingProbeDescriptorSets[imageIndex], 0, nullptr);

	uint32_t workGroupsX = (vulkanContext->swapchainExtent.width  + 31) / 32;
	uint32_t workGroupsY = (vulkanContext->swapchainExtent.height + 31) / 32;

	commandBuffer.dispatch(workGroupsX, workGroupsY, 1);
}