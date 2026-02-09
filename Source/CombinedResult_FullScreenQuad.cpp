#include "CombinedResult_FullScreenQuad.h"
#include <stdexcept>
#include <chrono>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include "Light.h"
#include "Camera.h"
#include "Lighting_RTX.h"

#include <random>

CombinedResult_FullScreenQuad::CombinedResult_FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext,Camera* cameraref, vk::CommandPool commandpool, Lighting_RTX* lighting): Drawable()
{
	camera = cameraref;
	bufferManager = buffermanager;
	vulkanContext = vulkancontext;
	commandPool   = commandpool;
	lightingref = lighting;
	CreateUniformBuffer();
	createDescriptorSetLayout();

}

CombinedResult_FullScreenQuad::~CombinedResult_FullScreenQuad()
{
	if (Gamma_Correction_descriptorSetLayout) {
		vulkanContext->LogicalDevice.destroyDescriptorSetLayout(Gamma_Correction_descriptorSetLayout);
	}

	Drawable::Destructor();
}

void CombinedResult_FullScreenQuad::CreateImage(vk::Extent3D imageExtent)
{
	Combined_Lighting_Image.ImageID = "Combined Lighting  Image  Texture";
	bufferManager->CreateImage(&Combined_Lighting_Image, imageExtent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	Combined_Lighting_Image.imageView = bufferManager->CreateImageView(&Combined_Lighting_Image, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	Combined_Lighting_Image.imageSampler = bufferManager->CreateImageSampler();

	IMGUI_PRESENT_IMAGE.ImageID = "IMGUI PRESENT  Image  Texture";
	bufferManager->CreateImage(&IMGUI_PRESENT_IMAGE, imageExtent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	IMGUI_PRESENT_IMAGE.imageView = bufferManager->CreateImageView(&IMGUI_PRESENT_IMAGE, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	IMGUI_PRESENT_IMAGE.imageSampler = bufferManager->CreateImageSampler();

	IMGUI_PRESENT_IMAGE_GAMMA_CORRECTED.ImageID = "IMGUI_PRESENT IMAGE GAMMA CORRECTED  Image  Texture";
	bufferManager->CreateImage(&IMGUI_PRESENT_IMAGE_GAMMA_CORRECTED, imageExtent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	IMGUI_PRESENT_IMAGE_GAMMA_CORRECTED.imageView = bufferManager->CreateImageView(&IMGUI_PRESENT_IMAGE_GAMMA_CORRECTED, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	IMGUI_PRESENT_IMAGE_GAMMA_CORRECTED.imageSampler = bufferManager->CreateImageSampler();

	Final_Denoised_Image.ImageID = "Denoised Image Texture";
	bufferManager->CreateImage(&Final_Denoised_Image, imageExtent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage);
	Final_Denoised_Image.imageView = bufferManager->CreateImageView(&Final_Denoised_Image, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	Final_Denoised_Image.imageSampler = bufferManager->CreateImageSampler();

}

void CombinedResult_FullScreenQuad::DestroyImage()
{

	bufferManager->DestroyImage(Combined_Lighting_Image);
	bufferManager->DestroyImage(IMGUI_PRESENT_IMAGE);
	bufferManager->DestroyImage(IMGUI_PRESENT_IMAGE_GAMMA_CORRECTED);
	bufferManager->DestroyImage(Final_Denoised_Image);

}


void CombinedResult_FullScreenQuad::createDescriptorSetLayout()
{
	{
		vk::DescriptorSetLayoutBinding LightingResultDescriptorBinding{};
		LightingResultDescriptorBinding.binding = 0;
		LightingResultDescriptorBinding.descriptorCount = 1;
		LightingResultDescriptorBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		LightingResultDescriptorBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding SSGIDescriptorBinding{};
		SSGIDescriptorBinding.binding = 1;
		SSGIDescriptorBinding.descriptorCount = 1;
		SSGIDescriptorBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		SSGIDescriptorBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding SSAODescriptorBinding{};
		SSAODescriptorBinding.binding = 2;
		SSAODescriptorBinding.descriptorCount = 1;
		SSAODescriptorBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		SSAODescriptorBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding MaterialsDescriptorBinding{};
		MaterialsDescriptorBinding.binding = 3;
		MaterialsDescriptorBinding.descriptorCount = 1;
		MaterialsDescriptorBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		MaterialsDescriptorBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding AlbedoDescriptorBinding{};
		AlbedoDescriptorBinding.binding = 4;
		AlbedoDescriptorBinding.descriptorCount = 1;
		AlbedoDescriptorBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		AlbedoDescriptorBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding DDGIDescriptorBinding{};
		DDGIDescriptorBinding.binding = 5;
		DDGIDescriptorBinding.descriptorCount = 1;
		DDGIDescriptorBinding.descriptorType = vk::DescriptorType::eStorageImage;
		DDGIDescriptorBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding PTGI_DescriptorBinding{};
		PTGI_DescriptorBinding.binding = 6;
		PTGI_DescriptorBinding.descriptorCount = 1;
		PTGI_DescriptorBinding.descriptorType = vk::DescriptorType::eStorageImage;
		PTGI_DescriptorBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding Last_Frame_PTGI_DescriptorBinding{};
		Last_Frame_PTGI_DescriptorBinding.binding = 7;
		Last_Frame_PTGI_DescriptorBinding.descriptorCount = 1;
		Last_Frame_PTGI_DescriptorBinding.descriptorType = vk::DescriptorType::eStorageImage;
		Last_Frame_PTGI_DescriptorBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding MotionVectorDescriptorBinding{};
		MotionVectorDescriptorBinding.binding = 8;
		MotionVectorDescriptorBinding.descriptorCount = 1;
		MotionVectorDescriptorBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		MotionVectorDescriptorBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		std::array<vk::DescriptorSetLayoutBinding, 9> ImageResultPassBinding = { LightingResultDescriptorBinding,
			                                                                     SSGIDescriptorBinding,SSAODescriptorBinding,
			                                                                     MaterialsDescriptorBinding,AlbedoDescriptorBinding,
			                                                                     DDGIDescriptorBinding,PTGI_DescriptorBinding,Last_Frame_PTGI_DescriptorBinding,
			                                                                     MotionVectorDescriptorBinding };

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(ImageResultPassBinding.size());
		layoutInfo.pBindings = ImageResultPassBinding.data();

		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
	}

	{
		vk::DescriptorSetLayoutBinding CombinedResulttDescriptorBinding{};
		CombinedResulttDescriptorBinding.binding = 0;
		CombinedResulttDescriptorBinding.descriptorCount = 1;
		CombinedResulttDescriptorBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		CombinedResulttDescriptorBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		std::array<vk::DescriptorSetLayoutBinding, 1> GammaBindings = { CombinedResulttDescriptorBinding };

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(GammaBindings.size());
		layoutInfo.pBindings = GammaBindings.data();

		vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &Gamma_Correction_descriptorSetLayout);
	}

}


void CombinedResult_FullScreenQuad::UpdataeUniformBufferData()
{
	vulkanContext->AccumilationCount++;
}


void CombinedResult_FullScreenQuad::createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, ImageData LightingResultImage, ImageData SSGIImage, ImageData SSAOIImage, ImageData MaterialImage, ImageData AlbedoImage,ImageData DDGIImaGE, ImageData PTGiIMAGE, ImageData TA_PTGiIMAGE, ImageData MotionVector)
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
		vk::DescriptorImageInfo LightingResultimageInfo{};
		LightingResultimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		LightingResultimageInfo.imageView   = LightingResultImage.imageView;
		LightingResultimageInfo.sampler    = LightingResultImage.imageSampler;

		vk::WriteDescriptorSet LightingResultSamplerdescriptorWrite{};
		LightingResultSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		LightingResultSamplerdescriptorWrite.dstBinding = 0;
		LightingResultSamplerdescriptorWrite.descriptorCount = 1;
		LightingResultSamplerdescriptorWrite.dstArrayElement = 0;
		LightingResultSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		LightingResultSamplerdescriptorWrite.pImageInfo = &LightingResultimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////
;
        vk::DescriptorImageInfo SSGIImageResultimageInfo{};
		SSGIImageResultimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		SSGIImageResultimageInfo.imageView   = SSGIImage.imageView;
		SSGIImageResultimageInfo.sampler     = SSGIImage.imageSampler;
        
        vk::WriteDescriptorSet SSGISamplerdescriptorWrite{};
		SSGISamplerdescriptorWrite.dstSet = DescriptorSets[i];
		SSGISamplerdescriptorWrite.dstBinding = 1;
		SSGISamplerdescriptorWrite.descriptorCount = 1;
		SSGISamplerdescriptorWrite.dstArrayElement = 0;
		SSGISamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		SSGISamplerdescriptorWrite.pImageInfo = &SSGIImageResultimageInfo;
        /////////////////////////////////////////////////////////////////////////////////////


		vk::DescriptorImageInfo SSAOImageResultimageInfo{};
		SSAOImageResultimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		SSAOImageResultimageInfo.imageView = SSAOIImage.imageView;
		SSAOImageResultimageInfo.sampler = SSAOIImage.imageSampler;

		vk::WriteDescriptorSet SSAOSamplerdescriptorWrite{};
		SSAOSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		SSAOSamplerdescriptorWrite.dstBinding = 2;
		SSAOSamplerdescriptorWrite.descriptorCount = 1;
		SSAOSamplerdescriptorWrite.dstArrayElement = 0;
		SSAOSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		SSAOSamplerdescriptorWrite.pImageInfo = &SSAOImageResultimageInfo;


		vk::DescriptorImageInfo MaterialsImageResultimageInfo{};
		MaterialsImageResultimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		MaterialsImageResultimageInfo.imageView = MaterialImage.imageView;
		MaterialsImageResultimageInfo.sampler = MaterialImage.imageSampler;

		vk::WriteDescriptorSet MaterialsSamplerdescriptorWrite{};
		MaterialsSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		MaterialsSamplerdescriptorWrite.dstBinding = 3;
		MaterialsSamplerdescriptorWrite.descriptorCount = 1;
		MaterialsSamplerdescriptorWrite.dstArrayElement = 0;
		MaterialsSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		MaterialsSamplerdescriptorWrite.pImageInfo = &MaterialsImageResultimageInfo;

		vk::DescriptorImageInfo AlbedoImageResultimageInfo{};
		AlbedoImageResultimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		AlbedoImageResultimageInfo.imageView = AlbedoImage.imageView;
		AlbedoImageResultimageInfo.sampler = AlbedoImage.imageSampler;

		vk::WriteDescriptorSet AlbedoSamplerdescriptorWrite{};
		AlbedoSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		AlbedoSamplerdescriptorWrite.dstBinding = 4;
		AlbedoSamplerdescriptorWrite.descriptorCount = 1;
		AlbedoSamplerdescriptorWrite.dstArrayElement = 0;
		AlbedoSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		AlbedoSamplerdescriptorWrite.pImageInfo = &AlbedoImageResultimageInfo;

		vk::DescriptorImageInfo DDGIImageResultimageInfo{};
		DDGIImageResultimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		DDGIImageResultimageInfo.imageView = DDGIImaGE.imageView;
		DDGIImageResultimageInfo.sampler = DDGIImaGE.imageSampler;

		vk::WriteDescriptorSet DDGISamplerdescriptorWrite{};
		DDGISamplerdescriptorWrite.dstSet = DescriptorSets[i];
		DDGISamplerdescriptorWrite.dstBinding = 5;
		DDGISamplerdescriptorWrite.descriptorCount = 1;
		DDGISamplerdescriptorWrite.dstArrayElement = 0;
		DDGISamplerdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
		DDGISamplerdescriptorWrite.pImageInfo = &DDGIImageResultimageInfo;

		vk::DescriptorImageInfo  PTGIImageResultimageInfo{};
		PTGIImageResultimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		PTGIImageResultimageInfo.imageView = PTGiIMAGE.imageView;
		PTGIImageResultimageInfo.sampler = PTGiIMAGE.imageSampler;

		vk::WriteDescriptorSet  PTGI_SamplerdescriptorWrite{};
		PTGI_SamplerdescriptorWrite.dstSet = DescriptorSets[i];
		PTGI_SamplerdescriptorWrite.dstBinding = 6;
		PTGI_SamplerdescriptorWrite.descriptorCount = 1;
		PTGI_SamplerdescriptorWrite.dstArrayElement = 0;
		PTGI_SamplerdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
		PTGI_SamplerdescriptorWrite.pImageInfo = &PTGIImageResultimageInfo;

		vk::DescriptorImageInfo  TA_PTGIImageResultimageInfo{};
		TA_PTGIImageResultimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		TA_PTGIImageResultimageInfo.imageView = TA_PTGiIMAGE.imageView;
		TA_PTGIImageResultimageInfo.sampler = TA_PTGiIMAGE.imageSampler;

		vk::WriteDescriptorSet  Last_Frame_PTGI_SamplerdescriptorWrite{};
		Last_Frame_PTGI_SamplerdescriptorWrite.dstSet = DescriptorSets[i];
		Last_Frame_PTGI_SamplerdescriptorWrite.dstBinding = 7;
		Last_Frame_PTGI_SamplerdescriptorWrite.descriptorCount = 1;
		Last_Frame_PTGI_SamplerdescriptorWrite.dstArrayElement = 0;
		Last_Frame_PTGI_SamplerdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
		Last_Frame_PTGI_SamplerdescriptorWrite.pImageInfo = &TA_PTGIImageResultimageInfo;

		vk::DescriptorImageInfo MotionVectorImageResultimageInfo{};
		MotionVectorImageResultimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		MotionVectorImageResultimageInfo.imageView = MotionVector.imageView;
		MotionVectorImageResultimageInfo.sampler = MotionVector.imageSampler;

		vk::WriteDescriptorSet MotionVectorSamplerdescriptorWrite{};
		MotionVectorSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		MotionVectorSamplerdescriptorWrite.dstBinding = 8;
		MotionVectorSamplerdescriptorWrite.descriptorCount = 1;
		MotionVectorSamplerdescriptorWrite.dstArrayElement = 0;
		MotionVectorSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		MotionVectorSamplerdescriptorWrite.pImageInfo = &MotionVectorImageResultimageInfo;

		std::array<vk::WriteDescriptorSet, 9> descriptorWrites = { LightingResultSamplerdescriptorWrite,SSGISamplerdescriptorWrite,
			                                                       SSAOSamplerdescriptorWrite,MaterialsSamplerdescriptorWrite,
			                                                       AlbedoSamplerdescriptorWrite,DDGISamplerdescriptorWrite,
																   PTGI_SamplerdescriptorWrite,Last_Frame_PTGI_SamplerdescriptorWrite,MotionVectorSamplerdescriptorWrite };

		vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}


	{
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, Gamma_Correction_descriptorSetLayout);

		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = layouts.data();

		Gamma_Correction_DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, Gamma_Correction_DescriptorSets.data());


		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			/////////////////////////////////////////////////////////////////////////////////////
			vk::DescriptorImageInfo LightingResultimageInfo{};
			LightingResultimageInfo.imageLayout = vk::ImageLayout::eGeneral;
			LightingResultimageInfo.imageView = IMGUI_PRESENT_IMAGE.imageView;
			LightingResultimageInfo.sampler = IMGUI_PRESENT_IMAGE.imageSampler;

			vk::WriteDescriptorSet LightingResultSamplerdescriptorWrite{};
			LightingResultSamplerdescriptorWrite.dstSet = Gamma_Correction_DescriptorSets[i];
			LightingResultSamplerdescriptorWrite.dstBinding = 0;
			LightingResultSamplerdescriptorWrite.descriptorCount = 1;
			LightingResultSamplerdescriptorWrite.dstArrayElement = 0;
			LightingResultSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			LightingResultSamplerdescriptorWrite.pImageInfo = &LightingResultimageInfo;

			std::array<vk::WriteDescriptorSet, 1> descriptorWrites = { LightingResultSamplerdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}
}


void CombinedResult_FullScreenQuad::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { 	bufferManager->FullScreenQuadVertexBufferData.buffer };

	PostProcessSettings PPS;
	PPS.Brightness_Saturation_Concentration_GIboost = glm::vec4(Brightness, Saturation, Concentration, GIBoost);
	PPS.MaxGamma_MinGamma_Padding = glm::vec4(MaxGamma, MinGamma, lightingref->GISolutionIndex, (float)vulkanContext->AccumilationCount);

	commandbuffer.pushConstants(pipelinelayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(PostProcessSettings), &PPS);

	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(bufferManager->FullScreenQuadIndexBufferData.buffer, 0, vk::IndexType::eUint16);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(bufferManager->quadIndices.size(), 1, 0, 0, 0);
}

void CombinedResult_FullScreenQuad::DrawGammaCorrection(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { bufferManager->FullScreenQuadVertexBufferData.buffer };
	vulkanContext->vkCmdSetPolygonModeEXT(commandbuffer, VkPolygonMode::VK_POLYGON_MODE_FILL);
	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(bufferManager->FullScreenQuadIndexBufferData.buffer, 0, vk::IndexType::eUint16);

	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &Gamma_Correction_DescriptorSets[imageIndex], 0, nullptr);

	commandbuffer.drawIndexed(bufferManager->quadIndices.size(), 1, 0, 0, 0);
}


