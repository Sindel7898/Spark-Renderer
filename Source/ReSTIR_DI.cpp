#include "ReSTIR_DI.h"
#include "VulkanContext.h"
#include "Lighting_RTX.h"
#include "SSGI.h"
#include "DynamicDiffuse_RTGI.h"



ReSTIR_DI::ReSTIR_DI(VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger, Lighting_RTX* rLightingPass, SSGI* rssgi, DynamicDiffuse_RTGI* DDGIr)
{
	bufferManager = buffermanger;
	vulkanContext = vulkancontext;
	camera = rcamera;
	commandPool = commandpool;
	LightingPass = rLightingPass;
	ssgi = rssgi;
	DDGIRef = DDGIr;
	createDescriptorSetLayout();
}

ReSTIR_DI::~ReSTIR_DI()
{
	CleanUp();
}

void ReSTIR_DI::CreateImage() {

	vk::Extent3D SampledImageExtent = vk::Extent3D(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, 1);

	ResevoirImage.ImageID = " Resevoir  Image";
	bufferManager->CreateImage(&ResevoirImage, SampledImageExtent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eColorAttachment);
	ResevoirImage.imageView = bufferManager->CreateImageView(&ResevoirImage, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	ResevoirImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge, true);

	PrevResevoirImage.ImageID = " Prev Resevoir  Image";
	bufferManager->CreateImage(&PrevResevoirImage, SampledImageExtent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment);
	PrevResevoirImage.imageView = bufferManager->CreateImageView(&PrevResevoirImage, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	PrevResevoirImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge, true);

	ReSTIRDI_Results.ImageID = " Prev ReSTIRDI  Image";
	bufferManager->CreateImage(&ReSTIRDI_Results, SampledImageExtent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment);
	ReSTIRDI_Results.imageView = bufferManager->CreateImageView(&ReSTIRDI_Results, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	ReSTIRDI_Results.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge, true);

	ReSTIRDI_Denoised_Results.ImageID = " Prev ReSTIR DI Denoised Image";
	bufferManager->CreateImage(&ReSTIRDI_Denoised_Results, SampledImageExtent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment);
	ReSTIRDI_Denoised_Results.imageView = bufferManager->CreateImageView(&ReSTIRDI_Denoised_Results, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	ReSTIRDI_Denoised_Results.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge, true);



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

	if (PrevResevoirImage.image)
	{
		bufferManager->DestroyImage(PrevResevoirImage);
	}

	if (ReSTIRDI_Results.image)
	{
		bufferManager->DestroyImage(ReSTIRDI_Results);
	}

	if (ReSTIRDI_Denoised_Results.image)
	{
		bufferManager->DestroyImage(ReSTIRDI_Denoised_Results);
	}
}


void ReSTIR_DI::createDescriptorSetLayout()
{

	{
		vk::DescriptorSetLayoutBinding LightUniformBufferLayout{};
		LightUniformBufferLayout.binding = 0;
		LightUniformBufferLayout.descriptorCount = 1;
		LightUniformBufferLayout.descriptorType = vk::DescriptorType::eUniformBuffer;
		LightUniformBufferLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding ResevoirStorageImageLayout{};
		ResevoirStorageImageLayout.binding = 1;
		ResevoirStorageImageLayout.descriptorCount = 1;
		ResevoirStorageImageLayout.descriptorType = vk::DescriptorType::eStorageImage;
		ResevoirStorageImageLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding WorldPositionImageLayout{};
		WorldPositionImageLayout.binding = 2;
		WorldPositionImageLayout.descriptorCount = 1;
		WorldPositionImageLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		WorldPositionImageLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding NormalImageLayout{};
		NormalImageLayout.binding = 3;
		NormalImageLayout.descriptorCount = 1;
		NormalImageLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		NormalImageLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding AlbedoImageLayout{};
		AlbedoImageLayout.binding = 4;
		AlbedoImageLayout.descriptorCount = 1;
		AlbedoImageLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		AlbedoImageLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding MaterialImageLayout{};
		MaterialImageLayout.binding = 5;
		MaterialImageLayout.descriptorCount = 1;
		MaterialImageLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		MaterialImageLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding BluenoiseImageLayout{};
		BluenoiseImageLayout.binding = 6;
		BluenoiseImageLayout.descriptorCount = ssgi->BlueNoiseTextures.size();
		BluenoiseImageLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		BluenoiseImageLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding TLAS{};
		TLAS.binding = 7;
		TLAS.descriptorCount = 1;
		TLAS.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
		TLAS.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding AlbedoAssetTexturesSamplerLayout{};
		AlbedoAssetTexturesSamplerLayout.binding = 8;
		AlbedoAssetTexturesSamplerLayout.descriptorCount = bufferManager->AllScene_Albedo_Images.size();
		AlbedoAssetTexturesSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		AlbedoAssetTexturesSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

		vk::DescriptorSetLayoutBinding NormalAssetTexturesSamplerLayout{};
		NormalAssetTexturesSamplerLayout.binding = 9;
		NormalAssetTexturesSamplerLayout.descriptorCount = bufferManager->AllScene_Normal_Images.size();
		NormalAssetTexturesSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		NormalAssetTexturesSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

		vk::DescriptorSetLayoutBinding MetalicRoughnessAssetTexturesSamplerLayout{};
		MetalicRoughnessAssetTexturesSamplerLayout.binding = 10;
		MetalicRoughnessAssetTexturesSamplerLayout.descriptorCount = bufferManager->AllScene_MetalicRoughness_Images.size();
		MetalicRoughnessAssetTexturesSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		MetalicRoughnessAssetTexturesSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

		vk::DescriptorSetLayoutBinding EmmisiveAssetTexturesSamplerLayout{};
		EmmisiveAssetTexturesSamplerLayout.binding = 11;
		EmmisiveAssetTexturesSamplerLayout.descriptorCount = bufferManager->AllScene_Emissive_Images.size();
		EmmisiveAssetTexturesSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		EmmisiveAssetTexturesSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

		vk::DescriptorSetLayoutBinding IndexStorageBuffersLayout{};
		IndexStorageBuffersLayout.binding = 12;
		IndexStorageBuffersLayout.descriptorCount = 1;
		IndexStorageBuffersLayout.descriptorType = vk::DescriptorType::eStorageBuffer;
		IndexStorageBuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

		vk::DescriptorSetLayoutBinding VertexStorageBuffersLayout{};
		VertexStorageBuffersLayout.binding = 13;
		VertexStorageBuffersLayout.descriptorCount = 1;
		VertexStorageBuffersLayout.descriptorType = vk::DescriptorType::eStorageBuffer;
		VertexStorageBuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

		vk::DescriptorSetLayoutBinding offsetStorageBuffersLayout{};
		offsetStorageBuffersLayout.binding = 14;
		offsetStorageBuffersLayout.descriptorCount = 1;
		offsetStorageBuffersLayout.descriptorType = vk::DescriptorType::eStorageBuffer;
		offsetStorageBuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

		vk::DescriptorSetLayoutBinding trasnformationUniformBuffersLayout{};
		trasnformationUniformBuffersLayout.binding = 15;
		trasnformationUniformBuffersLayout.descriptorCount = 1;
		trasnformationUniformBuffersLayout.descriptorType = vk::DescriptorType::eUniformBuffer;
		trasnformationUniformBuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

		vk::DescriptorSetLayoutBinding PrevResevoirStorageImageLayout{};
		PrevResevoirStorageImageLayout.binding = 16;
		PrevResevoirStorageImageLayout.descriptorCount = 1;
		PrevResevoirStorageImageLayout.descriptorType = vk::DescriptorType::eStorageImage;
		PrevResevoirStorageImageLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding MotionVectorImageLayout{};
		MotionVectorImageLayout.binding = 17;
		MotionVectorImageLayout.descriptorCount = 1;
		MotionVectorImageLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		MotionVectorImageLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding ReSTIRDIImageLayout{};
		ReSTIRDIImageLayout.binding = 18;
		ReSTIRDIImageLayout.descriptorCount = 1;
		ReSTIRDIImageLayout.descriptorType = vk::DescriptorType::eStorageImage;
		ReSTIRDIImageLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding PrevNormalImageLayout{};
		PrevNormalImageLayout.binding = 19;
		PrevNormalImageLayout.descriptorCount = 1;
		PrevNormalImageLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		PrevNormalImageLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding EmmiveImageLayout{};
		EmmiveImageLayout.binding = 20;
		EmmiveImageLayout.descriptorCount = 1;
		EmmiveImageLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		EmmiveImageLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		std::array<vk::DescriptorSetLayoutBinding, 21> bindings = {
					LightUniformBufferLayout, ResevoirStorageImageLayout,
					WorldPositionImageLayout, NormalImageLayout,
					AlbedoImageLayout, MaterialImageLayout,
					BluenoiseImageLayout,
					TLAS,AlbedoAssetTexturesSamplerLayout,NormalAssetTexturesSamplerLayout,
					MetalicRoughnessAssetTexturesSamplerLayout,EmmisiveAssetTexturesSamplerLayout,
					IndexStorageBuffersLayout,VertexStorageBuffersLayout,offsetStorageBuffersLayout,
					trasnformationUniformBuffersLayout,PrevResevoirStorageImageLayout,MotionVectorImageLayout,
					ReSTIRDIImageLayout,PrevNormalImageLayout,EmmiveImageLayout
		};

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &RayTracingDescriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
	}

	{
		vk::DescriptorSetLayoutBinding IrradianceAtlasImageLayout{};
		IrradianceAtlasImageLayout.binding = 0;
		IrradianceAtlasImageLayout.descriptorCount = 1;
		IrradianceAtlasImageLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		IrradianceAtlasImageLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		vk::DescriptorSetLayoutBinding VisibilityAtlasImageLayout{};
		VisibilityAtlasImageLayout.binding = 1;
		VisibilityAtlasImageLayout.descriptorCount = 1;
		VisibilityAtlasImageLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		VisibilityAtlasImageLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

		std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { IrradianceAtlasImageLayout ,VisibilityAtlasImageLayout };

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &DDGIATLASDescriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
	}
}

void ReSTIR_DI::createDescriptorDDGIATLAS(vk::DescriptorPool descriptorpool)
{
	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, DDGIATLASDescriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocinfo;
	allocinfo.descriptorPool = descriptorpool;
	allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocinfo.pSetLayouts = layouts.data();

	RaytracingDDGIDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, RaytracingDDGIDescriptorSets.data());

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		vk::DescriptorImageInfo IradianceImageInfo{};
		IradianceImageInfo.imageLayout = vk::ImageLayout::eGeneral;
		IradianceImageInfo.imageView = DDGIRef->IradianceImageAtlasImage.imageView;
		IradianceImageInfo.sampler = DDGIRef->IradianceImageAtlasImage.imageSampler;

		vk::WriteDescriptorSet IrradianceImagStoragedescriptorWrite{};
		IrradianceImagStoragedescriptorWrite.dstSet = RaytracingDDGIDescriptorSets[i];
		IrradianceImagStoragedescriptorWrite.dstBinding = 0;
		IrradianceImagStoragedescriptorWrite.dstArrayElement = 0;
		IrradianceImagStoragedescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		IrradianceImagStoragedescriptorWrite.descriptorCount = 1;
		IrradianceImagStoragedescriptorWrite.pImageInfo = &IradianceImageInfo;

		vk::DescriptorImageInfo VisibilityImageInfo{};
		VisibilityImageInfo.imageLayout = vk::ImageLayout::eGeneral;
		VisibilityImageInfo.imageView = DDGIRef->VisibilityImageAtlasImage.imageView;
		VisibilityImageInfo.sampler = DDGIRef->VisibilityImageAtlasImage.imageSampler;

		vk::WriteDescriptorSet VisibilityImagStoragedescriptorWrite{};
		VisibilityImagStoragedescriptorWrite.dstSet = RaytracingDDGIDescriptorSets[i];
		VisibilityImagStoragedescriptorWrite.dstBinding = 1;
		VisibilityImagStoragedescriptorWrite.dstArrayElement = 0;
		VisibilityImagStoragedescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		VisibilityImagStoragedescriptorWrite.descriptorCount = 1;
		VisibilityImagStoragedescriptorWrite.pImageInfo = &VisibilityImageInfo;


		std::array<vk::WriteDescriptorSet, 2> descriptorWrites = {IrradianceImagStoragedescriptorWrite,VisibilityImagStoragedescriptorWrite};

		vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}

}


void ReSTIR_DI::createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, vk::AccelerationStructureKHR* TLAS)
{
	TLASr = TLAS;
	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, RayTracingDescriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocinfo;
	allocinfo.descriptorPool = descriptorpool;
	allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocinfo.pSetLayouts = layouts.data();

	RaytracingDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, RaytracingDescriptorSets.data());

	UpdateDescrptorSets();
}

void ReSTIR_DI::UpdateDescrptorSets()
{

	{
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			vk::DescriptorBufferInfo LightUniformBufferInfo;
			LightUniformBufferInfo.buffer = LightingPass->UniformBuffers[i].buffer;
			LightUniformBufferInfo.offset = 0;
			LightUniformBufferInfo.range = sizeof(LightUniformData) * MAX_LIGHT_COUNT;

			vk::WriteDescriptorSet LightUniformBufferDescriptorWrite{};
			LightUniformBufferDescriptorWrite.dstSet = RaytracingDescriptorSets[i];
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
			ResevoirImagedescriptorWrite.dstSet = RaytracingDescriptorSets[i];
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
			PositionSamplerdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
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
			NormalimageInfo.sampler = LightingPass->GbufferRef->Normal.imageSampler;

			vk::WriteDescriptorSet NormalSamplerdescriptorWrite{};
			NormalSamplerdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			NormalSamplerdescriptorWrite.dstBinding = 3;
			NormalSamplerdescriptorWrite.dstArrayElement = 0;
			NormalSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			NormalSamplerdescriptorWrite.descriptorCount = 1;
			NormalSamplerdescriptorWrite.pImageInfo = &NormalimageInfo;
			/////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorImageInfo AlbedoimageInfo{};
			AlbedoimageInfo.imageLayout = vk::ImageLayout::eGeneral;
			AlbedoimageInfo.imageView = LightingPass->GbufferRef->Albedo.imageView;
			AlbedoimageInfo.sampler = LightingPass->GbufferRef->Albedo.imageSampler;

			vk::WriteDescriptorSet  AlbedoSamplerdescriptorWrite{};
			AlbedoSamplerdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			AlbedoSamplerdescriptorWrite.dstBinding = 4;
			AlbedoSamplerdescriptorWrite.dstArrayElement = 0;
			AlbedoSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			AlbedoSamplerdescriptorWrite.descriptorCount = 1;
			AlbedoSamplerdescriptorWrite.pImageInfo = &AlbedoimageInfo;
			/////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorImageInfo MaterialsimageInfo{};
			MaterialsimageInfo.imageLayout = vk::ImageLayout::eGeneral;
			MaterialsimageInfo.imageView = LightingPass->GbufferRef->Materials.imageView;
			MaterialsimageInfo.sampler = LightingPass->GbufferRef->Materials.imageSampler;

			vk::WriteDescriptorSet MaterialsSamplerdescriptorWrite{};
			MaterialsSamplerdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
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
			BlueNoiseSamplerdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			BlueNoiseSamplerdescriptorWrite.dstBinding = 6;
			BlueNoiseSamplerdescriptorWrite.dstArrayElement = 0;
			BlueNoiseSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			BlueNoiseSamplerdescriptorWrite.descriptorCount = BlueNoiseImagesInfos.size();
			BlueNoiseSamplerdescriptorWrite.pImageInfo = BlueNoiseImagesInfos.data();


			vk::WriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
			descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
			descriptorAccelerationStructureInfo.pAccelerationStructures = TLASr;

			vk::WriteDescriptorSet TLAS_descriptorWrite{};
			TLAS_descriptorWrite.dstSet = RaytracingDescriptorSets[i];
			TLAS_descriptorWrite.dstBinding = 7;
			TLAS_descriptorWrite.dstArrayElement = 0;
			TLAS_descriptorWrite.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
			TLAS_descriptorWrite.descriptorCount = 1;
			TLAS_descriptorWrite.pNext = &descriptorAccelerationStructureInfo;



			std::vector<vk::DescriptorImageInfo>AssetImagesInfos;

			for (int j = 0; j < bufferManager->AllScene_Albedo_Images.size(); j++)
			{
				ImageData* imageData = bufferManager->AllScene_Albedo_Images[j];
				if (imageData) {

					vk::DescriptorImageInfo ASSETImageInfo{};
					ASSETImageInfo.imageLayout = vk::ImageLayout::eGeneral;
					ASSETImageInfo.imageView = imageData->imageView;
					ASSETImageInfo.sampler = imageData->imageSampler;

					AssetImagesInfos.push_back(ASSETImageInfo);
				};
			}

			vk::WriteDescriptorSet AssetImagSamplerdescriptorWrite{};
			AssetImagSamplerdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			AssetImagSamplerdescriptorWrite.dstBinding = 8;
			AssetImagSamplerdescriptorWrite.dstArrayElement = 0;
			AssetImagSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			AssetImagSamplerdescriptorWrite.descriptorCount = AssetImagesInfos.size();
			AssetImagSamplerdescriptorWrite.pImageInfo = AssetImagesInfos.data();



			std::vector<vk::DescriptorImageInfo> NormalImageAssetImagesInfos;

			for (int j = 0; j < bufferManager->AllScene_Normal_Images.size(); j++)
			{
				ImageData* imageData = bufferManager->AllScene_Normal_Images[j];
				if (imageData) {

					vk::DescriptorImageInfo NormalASSETImageInfo{};
					NormalASSETImageInfo.imageLayout = vk::ImageLayout::eGeneral;
					NormalASSETImageInfo.imageView = imageData->imageView;
					NormalASSETImageInfo.sampler = imageData->imageSampler;

					NormalImageAssetImagesInfos.push_back(NormalASSETImageInfo);
				};
			}

			vk::WriteDescriptorSet NormalAssetImagSamplerdescriptorWrite{};
			NormalAssetImagSamplerdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			NormalAssetImagSamplerdescriptorWrite.dstBinding = 9;
			NormalAssetImagSamplerdescriptorWrite.dstArrayElement = 0;
			NormalAssetImagSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			NormalAssetImagSamplerdescriptorWrite.descriptorCount = NormalImageAssetImagesInfos.size();
			NormalAssetImagSamplerdescriptorWrite.pImageInfo = NormalImageAssetImagesInfos.data();


			std::vector<vk::DescriptorImageInfo> MetalicRoughnessImageAssetImagesInfos;

			for (int j = 0; j < bufferManager->AllScene_MetalicRoughness_Images.size(); j++)
			{
				ImageData* imageData = bufferManager->AllScene_MetalicRoughness_Images[j];
				if (imageData) {

					vk::DescriptorImageInfo MetalicRoughnessASSETImageInfo{};
					MetalicRoughnessASSETImageInfo.imageLayout = vk::ImageLayout::eGeneral;
					MetalicRoughnessASSETImageInfo.imageView = imageData->imageView;
					MetalicRoughnessASSETImageInfo.sampler = imageData->imageSampler;

					MetalicRoughnessImageAssetImagesInfos.push_back(MetalicRoughnessASSETImageInfo);
				};
			}

			vk::WriteDescriptorSet MetalicRoughnessAssetImagSamplerdescriptorWrite{};
			MetalicRoughnessAssetImagSamplerdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			MetalicRoughnessAssetImagSamplerdescriptorWrite.dstBinding = 10;
			MetalicRoughnessAssetImagSamplerdescriptorWrite.dstArrayElement = 0;
			MetalicRoughnessAssetImagSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			MetalicRoughnessAssetImagSamplerdescriptorWrite.descriptorCount = MetalicRoughnessImageAssetImagesInfos.size();
			MetalicRoughnessAssetImagSamplerdescriptorWrite.pImageInfo = MetalicRoughnessImageAssetImagesInfos.data();


			std::vector<vk::DescriptorImageInfo> EmmisiveImageAssetImagesInfos;

			for (int j = 0; j < bufferManager->AllScene_Emissive_Images.size(); j++)
			{
				ImageData* imageData = bufferManager->AllScene_Emissive_Images[j];

				if (imageData) {

					vk::DescriptorImageInfo EmmisiveASSETImageInfo{};
					EmmisiveASSETImageInfo.imageLayout = vk::ImageLayout::eGeneral;
					EmmisiveASSETImageInfo.imageView = imageData->imageView;
					EmmisiveASSETImageInfo.sampler = imageData->imageSampler;

					EmmisiveImageAssetImagesInfos.push_back(EmmisiveASSETImageInfo);
				};
			}

			vk::WriteDescriptorSet EmmisiveAssetImagSamplerdescriptorWrite{};
			EmmisiveAssetImagSamplerdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			EmmisiveAssetImagSamplerdescriptorWrite.dstBinding = 11;
			EmmisiveAssetImagSamplerdescriptorWrite.dstArrayElement = 0;
			EmmisiveAssetImagSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			EmmisiveAssetImagSamplerdescriptorWrite.descriptorCount = EmmisiveImageAssetImagesInfos.size();
			EmmisiveAssetImagSamplerdescriptorWrite.pImageInfo = EmmisiveImageAssetImagesInfos.data();


			vk::DescriptorBufferInfo IndexStorageBuffersInfo{};
			IndexStorageBuffersInfo.buffer = bufferManager->AllScene_IndexStorageBuffers[0].buffer;
			IndexStorageBuffersInfo.offset = 0;
			IndexStorageBuffersInfo.range = sizeof(uint32_t) * bufferManager->AllScene_IndexGeometryData.size();;

			vk::WriteDescriptorSet IndexStorageBufferdescriptorWrite{};
			IndexStorageBufferdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			IndexStorageBufferdescriptorWrite.dstBinding = 12;
			IndexStorageBufferdescriptorWrite.dstArrayElement = 0;
			IndexStorageBufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
			IndexStorageBufferdescriptorWrite.descriptorCount = 1;
			IndexStorageBufferdescriptorWrite.pBufferInfo = &IndexStorageBuffersInfo;

			vk::DescriptorBufferInfo VertexStorageBuffersInfo{};
			VertexStorageBuffersInfo.buffer = bufferManager->AllScene_VertexStorageBuffers[0].buffer;
			VertexStorageBuffersInfo.offset = 0;
			VertexStorageBuffersInfo.range = sizeof(PaddedModelVertex) * bufferManager->AllScene_VertexGeometryData.size();;

			vk::WriteDescriptorSet VertexStorageBufferdescriptorWrite{};
			VertexStorageBufferdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			VertexStorageBufferdescriptorWrite.dstBinding = 13;
			VertexStorageBufferdescriptorWrite.dstArrayElement = 0;
			VertexStorageBufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
			VertexStorageBufferdescriptorWrite.descriptorCount = 1;
			VertexStorageBufferdescriptorWrite.pBufferInfo = &VertexStorageBuffersInfo;

			vk::DescriptorBufferInfo OffsetStorageBuffersInfo{};
			OffsetStorageBuffersInfo.buffer = bufferManager->AllScene_OffsetStorageBuffers[0].buffer;
			OffsetStorageBuffersInfo.offset = 0;
			OffsetStorageBuffersInfo.range = sizeof(VertexAndIndexOffsets) * bufferManager->AllScene_VertexAndIndexOffsets.size();;

			vk::WriteDescriptorSet OffsetStorageBufferdescriptorWrite{};
			OffsetStorageBufferdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			OffsetStorageBufferdescriptorWrite.dstBinding = 14;
			OffsetStorageBufferdescriptorWrite.dstArrayElement = 0;
			OffsetStorageBufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
			OffsetStorageBufferdescriptorWrite.descriptorCount = 1;
			OffsetStorageBufferdescriptorWrite.pBufferInfo = &OffsetStorageBuffersInfo;

			vk::DescriptorBufferInfo TransformUniformBuffersInfo{};
			TransformUniformBuffersInfo.buffer = bufferManager->AllScene_TransformationUniformBuffers[i].buffer;
			TransformUniformBuffersInfo.offset = 0;
			TransformUniformBuffersInfo.range = sizeof(GlobalTransformationMatrices) * 100;

			vk::WriteDescriptorSet TransformUniformBufferdescriptorWrite{};
			TransformUniformBufferdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			TransformUniformBufferdescriptorWrite.dstBinding = 15;
			TransformUniformBufferdescriptorWrite.dstArrayElement = 0;
			TransformUniformBufferdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
			TransformUniformBufferdescriptorWrite.descriptorCount = 1;
			TransformUniformBufferdescriptorWrite.pBufferInfo = &TransformUniformBuffersInfo;

			vk::DescriptorImageInfo PrevResevoirImageInfo{};
			PrevResevoirImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			PrevResevoirImageInfo.imageView = PrevResevoirImage.imageView;
			PrevResevoirImageInfo.sampler = PrevResevoirImage.imageSampler;

			vk::WriteDescriptorSet PrevResevoirImagedescriptorWrite{};
			PrevResevoirImagedescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			PrevResevoirImagedescriptorWrite.dstBinding = 16;
			PrevResevoirImagedescriptorWrite.dstArrayElement = 0;
			PrevResevoirImagedescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			PrevResevoirImagedescriptorWrite.descriptorCount = 1;
			PrevResevoirImagedescriptorWrite.pImageInfo = &PrevResevoirImageInfo;

			vk::DescriptorImageInfo MotionVectorImageInfo{};
			MotionVectorImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			MotionVectorImageInfo.imageView = LightingPass->GbufferRef->MotionVector.imageView;
			MotionVectorImageInfo.sampler = LightingPass->GbufferRef->MotionVector.imageSampler;

			vk::WriteDescriptorSet MotionVectorImagedescriptorWrite{};
			MotionVectorImagedescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			MotionVectorImagedescriptorWrite.dstBinding = 17;
			MotionVectorImagedescriptorWrite.dstArrayElement = 0;
			MotionVectorImagedescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			MotionVectorImagedescriptorWrite.descriptorCount = 1;
			MotionVectorImagedescriptorWrite.pImageInfo = &MotionVectorImageInfo;


			vk::DescriptorImageInfo ReSTIRImageInfo{};
			ReSTIRImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			ReSTIRImageInfo.imageView = ReSTIRDI_Results.imageView;
			ReSTIRImageInfo.sampler = ReSTIRDI_Results.imageSampler;

			vk::WriteDescriptorSet ReSTIRImagedescriptorWrite{};
			ReSTIRImagedescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			ReSTIRImagedescriptorWrite.dstBinding = 18;
			ReSTIRImagedescriptorWrite.dstArrayElement = 0;
			ReSTIRImagedescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			ReSTIRImagedescriptorWrite.descriptorCount = 1;
			ReSTIRImagedescriptorWrite.pImageInfo = &ReSTIRImageInfo;

			vk::DescriptorImageInfo PrevNormalImageInfo{};
			PrevNormalImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			PrevNormalImageInfo.imageView = LightingPass->GbufferRef->PrevNormal.imageView;
			PrevNormalImageInfo.sampler = LightingPass->GbufferRef->PrevNormal.imageSampler;

			vk::WriteDescriptorSet PrevNormalmagedescriptorWrite{};
			PrevNormalmagedescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			PrevNormalmagedescriptorWrite.dstBinding = 19;
			PrevNormalmagedescriptorWrite.dstArrayElement = 0;
			PrevNormalmagedescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			PrevNormalmagedescriptorWrite.descriptorCount = 1;
			PrevNormalmagedescriptorWrite.pImageInfo = &PrevNormalImageInfo;

			vk::DescriptorImageInfo EmmisiveimageInfo{};
			EmmisiveimageInfo.imageLayout = vk::ImageLayout::eGeneral;
			EmmisiveimageInfo.imageView = LightingPass->GbufferRef->Emissive.imageView;
			EmmisiveimageInfo.sampler = LightingPass->GbufferRef->Emissive.imageSampler;

			vk::WriteDescriptorSet EmmisiveSamplerdescriptorWrite{};
			EmmisiveSamplerdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			EmmisiveSamplerdescriptorWrite.dstBinding = 20;
			EmmisiveSamplerdescriptorWrite.dstArrayElement = 0;
			EmmisiveSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			EmmisiveSamplerdescriptorWrite.descriptorCount = 1;
			EmmisiveSamplerdescriptorWrite.pImageInfo = &EmmisiveimageInfo;



			std::array<vk::WriteDescriptorSet, 21> descriptorWrites = {
							LightUniformBufferDescriptorWrite,
							ResevoirImagedescriptorWrite,
							PositionSamplerdescriptorWrite,
							NormalSamplerdescriptorWrite,
							AlbedoSamplerdescriptorWrite,
							MaterialsSamplerdescriptorWrite,
							BlueNoiseSamplerdescriptorWrite,
							TLAS_descriptorWrite,AssetImagSamplerdescriptorWrite,NormalAssetImagSamplerdescriptorWrite,
							MetalicRoughnessAssetImagSamplerdescriptorWrite,EmmisiveAssetImagSamplerdescriptorWrite,
							IndexStorageBufferdescriptorWrite,VertexStorageBufferdescriptorWrite,
							OffsetStorageBufferdescriptorWrite,TransformUniformBufferdescriptorWrite,PrevResevoirImagedescriptorWrite,
							MotionVectorImagedescriptorWrite,ReSTIRImagedescriptorWrite,PrevNormalmagedescriptorWrite,EmmisiveSamplerdescriptorWrite

			};

			vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}
}

void ReSTIR_DI::DispatchResevoirCandidateCalcCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex)
{
	//PushConstant pushConstant;
	//pushConstant.CameraPosition = glm::vec4(camera->GetPosition(),1);
	//pushConstant.ScreenSize     = glm::vec4(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, ssgi->NoiseIndex,1);
	//
	//commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(PushConstant), &pushConstant);
	//commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, 1, &RservoirSamplingProbeDescriptorSets[imageIndex], 0, nullptr);
	//
	//uint32_t workGroupsX = (vulkanContext->swapchainExtent.width  + 31) / 32;
	//uint32_t workGroupsY = (vulkanContext->swapchainExtent.height + 31) / 32;
	//
	//commandBuffer.dispatch(workGroupsX, workGroupsY, 1);
}


uint32_t ReSTIR_DI::alignedSize(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

void ReSTIR_DI::Draw(BufferData RayGenBuffer, BufferData RayHitBuffer, BufferData RayMisBuffer, vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
{
	vk::BufferDeviceAddressInfo raygenShaderBindingTableDeviceAdressesInfo;
	raygenShaderBindingTableDeviceAdressesInfo.buffer = RayGenBuffer.buffer;

	vk::BufferDeviceAddressInfo missShaderBindingTableDeviceAdressesInfo;
	missShaderBindingTableDeviceAdressesInfo.buffer = RayMisBuffer.buffer;

	vk::BufferDeviceAddressInfo hitShaderBindingTableDeviceAdressesInfo;
	hitShaderBindingTableDeviceAdressesInfo.buffer = RayHitBuffer.buffer;

	auto raygenShaderBindingTableAdress = vulkanContext->LogicalDevice.getBufferAddress(raygenShaderBindingTableDeviceAdressesInfo);
	auto missShaderBindingTableAdress   = vulkanContext->LogicalDevice.getBufferAddress(missShaderBindingTableDeviceAdressesInfo);
	auto hitShaderBindingTableAdress     = vulkanContext->LogicalDevice.getBufferAddress(hitShaderBindingTableDeviceAdressesInfo);


	const uint32_t handleSizeAligned = alignedSize(
		vulkanContext->RayTracingPipelineProperties.shaderGroupHandleSize,
		vulkanContext->RayTracingPipelineProperties.shaderGroupHandleAlignment);

	vk::StridedDeviceAddressRegionKHR    raygenShaderSbtEntry{};
	raygenShaderSbtEntry.deviceAddress = raygenShaderBindingTableAdress;
	raygenShaderSbtEntry.stride = handleSizeAligned;
	raygenShaderSbtEntry.size = handleSizeAligned;


	vk::StridedDeviceAddressRegionKHR  missShaderSbtEntry{};
	missShaderSbtEntry.deviceAddress = missShaderBindingTableAdress;
	missShaderSbtEntry.stride = handleSizeAligned;
	missShaderSbtEntry.size = handleSizeAligned;

	vk::StridedDeviceAddressRegionKHR hitShaderSbtEntry{};
	hitShaderSbtEntry.deviceAddress = hitShaderBindingTableAdress;
	hitShaderSbtEntry.stride = handleSizeAligned;
	hitShaderSbtEntry.size = handleSizeAligned;

	//vk::StridedDeviceAddressRegionKHR hitShaderSbtEntry{};

	vk::StridedDeviceAddressRegionKHR callableShaderSbtEntry{};

	VkStridedDeviceAddressRegionKHR  TEMP_raygenShaderSbtEntry   = static_cast<VkStridedDeviceAddressRegionKHR>(raygenShaderSbtEntry);
	VkStridedDeviceAddressRegionKHR  TEMP_missShaderSbtEntry     = static_cast<VkStridedDeviceAddressRegionKHR>(missShaderSbtEntry);;
	VkStridedDeviceAddressRegionKHR  TEMP_hitShaderSbtEntry      = static_cast<VkStridedDeviceAddressRegionKHR>(hitShaderSbtEntry);;
	VkStridedDeviceAddressRegionKHR  TEMP_callableShaderSbtEntry = static_cast<VkStridedDeviceAddressRegionKHR>(callableShaderSbtEntry);;

	
	int depth = 1;

	PushConstant pushConstant;
    pushConstant.CameraPosition = glm::vec4(camera->GetPosition(),1);
    pushConstant.ScreenSize     = glm::vec4(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, ssgi->NoiseIndex, LightingPass->LightCount);
	pushConstant.sampleGridInfo.GridBaseLocation_ScreenSizeWidth = glm::vec4(DDGIRef->GridLocation.x, DDGIRef->GridLocation.y, DDGIRef->GridLocation.z, vulkanContext->swapchainExtent.width);
	pushConstant.sampleGridInfo.ProbeSpacing_ScreenSizeHeight = glm::vec4(DDGIRef->ProbeOffset.x, DDGIRef->ProbeOffset.y, DDGIRef->ProbeOffset.z, vulkanContext->swapchainExtent.height);
	pushConstant.sampleGridInfo.ProbeCount = glm::vec4(DDGIRef->NumOfProbesX, DDGIRef->NumOfProbesY, DDGIRef->NumOfProbesZ, 0);
	pushConstant.sampleGridInfo.generalAtlasInfo.AtlasWidthSize = DDGIRef->IradianceImageExtent.width;
	pushConstant.sampleGridInfo.generalAtlasInfo.ProbeSideLength = DDGIRef->ProbeSideLength;
	pushConstant.sampleGridInfo.generalAtlasInfo.GutterSize = DDGIRef->GutterSize;
	pushConstant.sampleGridInfo.generalAtlasInfo.RaysPerProbe = DDGIRef->RaysPerProbe;
	pushConstant.Temporal_Spatial_Reuse_EnableDDGI_DDGI_Vertex_Flags = glm::vec4(bTemporalReuse, bSpatialReuse,bDDGI, DDGIRef->DDGIVertex);

	glm::mat  ProjectionMatrix = camera->GetPrevProjectionMatrix();
	ProjectionMatrix[1][1] *= -1;

	pushConstant.ProjectionViewMatrix = ProjectionMatrix * camera->GetPrevViewMatrix();

	commandbuffer.pushConstants(pipelinelayout, vk::ShaderStageFlagBits::eRaygenKHR, 0, sizeof(PushConstant), &pushConstant);

	vk::DescriptorSet sets[] = {
	RaytracingDescriptorSets[imageIndex],
	RaytracingDDGIDescriptorSets[imageIndex]
	};

	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, pipelinelayout, 0, 2, sets, 0, nullptr);

	vulkanContext->vkCmdTraceRaysKHR(
		commandbuffer,
		&TEMP_raygenShaderSbtEntry,
		&TEMP_missShaderSbtEntry,
		&TEMP_hitShaderSbtEntry,
		&TEMP_callableShaderSbtEntry,
		vulkanContext->swapchainExtent.width,
		vulkanContext->swapchainExtent.height,
		depth);

}


void ReSTIR_DI::CleanUp()
{
	vulkanContext->LogicalDevice.destroyDescriptorSetLayout(RayTracingDescriptorSetLayout);
	vulkanContext->LogicalDevice.destroyDescriptorSetLayout(DDGIATLASDescriptorSetLayout);

}

