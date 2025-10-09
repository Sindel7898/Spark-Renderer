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
		fragmentUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		FragmentUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

		VkDeviceSize	RayGenuniformBufferSize = sizeof(SSGI_UniformBufferData);

		for (size_t i = 0; i < fragmentUniformBuffers.size(); i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "SSGI Uniform Buffer" + i;
			bufferManager->CreateBuffer(&bufferdata, RayGenuniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
			fragmentUniformBuffers[i] = bufferdata;

			FragmentUniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
		}
	}

}


void SSGI::CreateGIImage() {

	SSGI_ImageFullResolution   = vk::Extent3D(vulkanContext->swapchainExtent.width/2, vulkanContext->swapchainExtent.height/2, 1);
	SSGI_ImageHalfResolution   = vk::Extent3D(SSGI_ImageFullResolution.width /2, SSGI_ImageFullResolution.height/ 2, 1);
	SSGI_ImageQuaterResolution = vk::Extent3D(SSGI_ImageHalfResolution.width / 2, SSGI_ImageHalfResolution.height / 2, 1);

	vk::Extent3D Swapchainextent_Full_Res = vk::Extent3D(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, 1);

	SSGIPassImage.ImageID = "SSGI Pass Image";
	bufferManager->CreateImage(&SSGIPassImage, SSGI_ImageFullResolution, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage,false);
	SSGIPassImage.imageView = bufferManager->CreateImageView(&SSGIPassImage, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	SSGIPassImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	SSGIPassLastFrameImage.ImageID = "Last SSGI Accumilation Image";
	bufferManager->CreateImage(&SSGIPassLastFrameImage, SSGI_ImageFullResolution, vulkanContext->swapchainformat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled| vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst);
	SSGIPassLastFrameImage.imageView = bufferManager->CreateImageView(&SSGIPassLastFrameImage, vulkanContext->swapchainformat, vk::ImageAspectFlagBits::eColor);
	SSGIPassLastFrameImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	SSGIAccumilationImage.ImageID = "SSGI Accumilation Image";
	bufferManager->CreateImage(&SSGIAccumilationImage, SSGI_ImageFullResolution, vulkanContext->swapchainformat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, false);
	SSGIAccumilationImage.imageView = bufferManager->CreateImageView(&SSGIAccumilationImage, vulkanContext->swapchainformat, vk::ImageAspectFlagBits::eColor);
	SSGIAccumilationImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);
	///////////////////////////////////////////////
	BlurPing_DownSampleHalfRes.ImageID = " BlurPing_DownSampleHalfRes  Image";
	bufferManager->CreateImage(&BlurPing_DownSampleHalfRes, SSGI_ImageHalfResolution, vulkanContext->swapchainformat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, false);
	BlurPing_DownSampleHalfRes.imageView = bufferManager->CreateImageView(&BlurPing_DownSampleHalfRes, vulkanContext->swapchainformat, vk::ImageAspectFlagBits::eColor);
	BlurPing_DownSampleHalfRes.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge,false);

	BlurPong_DownSampleHalfRes.ImageID = "BlurPong_DownSampleHalfRes  Image";
	bufferManager->CreateImage(&BlurPong_DownSampleHalfRes, SSGI_ImageHalfResolution, vulkanContext->swapchainformat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, false);
	BlurPong_DownSampleHalfRes.imageView = bufferManager->CreateImageView(&BlurPong_DownSampleHalfRes, vulkanContext->swapchainformat, vk::ImageAspectFlagBits::eColor);
	BlurPong_DownSampleHalfRes.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge, false);
	////////////////////////////////////////////////
	BlurPing_DownSampleQuaterRes.ImageID = " BlurPing_DownSampleQuaterRes Image";
	bufferManager->CreateImage(&BlurPing_DownSampleQuaterRes, SSGI_ImageQuaterResolution, vulkanContext->swapchainformat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, false);
	BlurPing_DownSampleQuaterRes.imageView = bufferManager->CreateImageView(&BlurPing_DownSampleQuaterRes, vulkanContext->swapchainformat, vk::ImageAspectFlagBits::eColor);
	BlurPing_DownSampleQuaterRes.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge, false);

	BlurPong_DownSampleQuaterRes.ImageID = "BlurPong_DownSampleQuaterRes Image";
	bufferManager->CreateImage(&BlurPong_DownSampleQuaterRes, SSGI_ImageQuaterResolution, vulkanContext->swapchainformat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, false);
	BlurPong_DownSampleQuaterRes.imageView = bufferManager->CreateImageView(&BlurPong_DownSampleQuaterRes, vulkanContext->swapchainformat, vk::ImageAspectFlagBits::eColor);
	BlurPong_DownSampleQuaterRes.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge, false);
	////////////////////////////////////////////////

	BlurPing_UPSampleHalfRes.ImageID = " BlurPing_UPSampleHalfRes Image";
	bufferManager->CreateImage(&BlurPing_UPSampleHalfRes, SSGI_ImageHalfResolution, vulkanContext->swapchainformat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, false);
	BlurPing_UPSampleHalfRes.imageView = bufferManager->CreateImageView(&BlurPing_UPSampleHalfRes, vulkanContext->swapchainformat, vk::ImageAspectFlagBits::eColor);
	BlurPing_UPSampleHalfRes.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge, false);

	BlurPong_UPSampleHalfRes.ImageID = " BlurPong_UPSampleHalfRes Image";
	bufferManager->CreateImage(&BlurPong_UPSampleHalfRes, SSGI_ImageHalfResolution, vulkanContext->swapchainformat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, false);
	BlurPong_UPSampleHalfRes.imageView = bufferManager->CreateImageView(&BlurPong_UPSampleHalfRes, vulkanContext->swapchainformat, vk::ImageAspectFlagBits::eColor);
	BlurPong_UPSampleHalfRes.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge, false);
	/////////////////////////////////////////////////////////////////////////

	BlurPing_UPSampleFullRes.ImageID = " BlurPing_UPSampleFullRes Image";
	bufferManager->CreateImage(&BlurPing_UPSampleFullRes, SSGI_ImageFullResolution, vulkanContext->swapchainformat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, false);
	BlurPing_UPSampleFullRes.imageView = bufferManager->CreateImageView(&BlurPing_UPSampleFullRes, vulkanContext->swapchainformat, vk::ImageAspectFlagBits::eColor);
	BlurPing_UPSampleFullRes.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge, false);

	BlurPong_UPSampleFullRes.ImageID = " BlurPing_UPSampleHalfRes Image";
	bufferManager->CreateImage(&BlurPong_UPSampleFullRes, SSGI_ImageFullResolution, vulkanContext->swapchainformat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, false);
	BlurPong_UPSampleFullRes.imageView = bufferManager->CreateImageView(&BlurPong_UPSampleFullRes, vulkanContext->swapchainformat, vk::ImageAspectFlagBits::eColor);
	BlurPong_UPSampleFullRes.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge,true);
	/////////////////////////////////////////////////////////////////////////


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
	bufferManager->DestroyImage(SSGIAccumilationImage);
	bufferManager->DestroyImage(SSGIPassLastFrameImage);
	bufferManager->DestroyImage(BlurPing_DownSampleHalfRes);
	bufferManager->DestroyImage(BlurPong_DownSampleHalfRes);
	bufferManager->DestroyImage(BlurPing_DownSampleQuaterRes);
	bufferManager->DestroyImage(BlurPong_DownSampleQuaterRes);
	bufferManager->DestroyImage(BlurPing_UPSampleHalfRes);
	bufferManager->DestroyImage(BlurPong_UPSampleHalfRes);
	bufferManager->DestroyImage(BlurPing_UPSampleFullRes);
	bufferManager->DestroyImage(BlurPong_UPSampleFullRes);
}


void SSGI::GenerateMipMaps(vk::CommandBuffer commandbuffer) {

	vk::Extent3D swapchainextent = vk::Extent3D(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, 1);

	//bufferManager->GenerateMipMaps(&HalfRes_SSGIPassImage, &commandbuffer, swapchainextent.width, swapchainextent.height, vulkanContext->graphicsQueue, 1);

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


	vk::DescriptorSetLayoutBinding AlbedoPassSamplerLayout{};
	AlbedoPassSamplerLayout.binding = 3;
	AlbedoPassSamplerLayout.descriptorCount = 1;
	AlbedoPassSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	AlbedoPassSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;


	vk::DescriptorSetLayoutBinding LightingPassSamplerLayout{};
	LightingPassSamplerLayout.binding = 4;
	LightingPassSamplerLayout.descriptorCount = 1;
	LightingPassSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	LightingPassSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutBinding BlueNoiseSamplerLayout{};
	BlueNoiseSamplerLayout.binding = 5;
	BlueNoiseSamplerLayout.descriptorCount = BlueNoiseTextures.size();
	BlueNoiseSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	BlueNoiseSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutBinding  SSGIUniformBufferLayout{};
	SSGIUniformBufferLayout.binding = 6;
	SSGIUniformBufferLayout.descriptorCount = 1;
	SSGIUniformBufferLayout.descriptorType = vk::DescriptorType::eUniformBuffer;
	SSGIUniformBufferLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;


	std::array<vk::DescriptorSetLayoutBinding, 7> bindings = { NormalsSamplerLayout,ViewSpacePositionsSamplerLayout,
												                DepthTextureSamplerLayout, AlbedoPassSamplerLayout, LightingPassSamplerLayout,
		                                                        BlueNoiseSamplerLayout,  SSGIUniformBufferLayout };

	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();


	if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess)
	{
		throw std::runtime_error("Failed to create descriptorset layout!");
	}

	{
		vk::DescriptorSetLayoutBinding NoiseSSGISamplerLayout{};
		NoiseSSGISamplerLayout.binding = 0;
		NoiseSSGISamplerLayout.descriptorCount = 1;
		NoiseSSGISamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		NoiseSSGISamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding LastFrameSSGISamplerLayout{};
		LastFrameSSGISamplerLayout.binding = 1;
		LastFrameSSGISamplerLayout.descriptorCount = 1;
		LastFrameSSGISamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		LastFrameSSGISamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { NoiseSSGISamplerLayout,LastFrameSSGISamplerLayout };

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &TemporalAccumilationDescriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
	}


	{
		vk::DescriptorSetLayoutBinding NoiseTASSGISamplerLayout{};
		NoiseTASSGISamplerLayout.binding = 0;
		NoiseTASSGISamplerLayout.descriptorCount = 1;
		NoiseTASSGISamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		NoiseTASSGISamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding ViewSpacePositionSamplerLayout{};
		ViewSpacePositionSamplerLayout.binding = 1;
		ViewSpacePositionSamplerLayout.descriptorCount = 1;
		ViewSpacePositionSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		ViewSpacePositionSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { NoiseTASSGISamplerLayout,ViewSpacePositionSamplerLayout };

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &Blured_TemporalAccumilationDescriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
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
			fragmentuniformbufferInfo.buffer = fragmentUniformBuffers[i].buffer;
			fragmentuniformbufferInfo.offset = 0;
			fragmentuniformbufferInfo.range = sizeof(SSGI_UniformBufferData);

			vk::WriteDescriptorSet fragmentUniformdescriptorWrite{};
			fragmentUniformdescriptorWrite.dstSet = DescriptorSets[i];
			fragmentUniformdescriptorWrite.dstBinding = 6;
			fragmentUniformdescriptorWrite.dstArrayElement = 0;
			fragmentUniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
			fragmentUniformdescriptorWrite.descriptorCount = 1;
			fragmentUniformdescriptorWrite.pBufferInfo = &fragmentuniformbufferInfo;


			std::array<vk::WriteDescriptorSet, 7> descriptorWrites{ NormalSamplerdescriptorWrite,ViewSpacePositionSamplerdescriptorWrite,DepthimageSamplerdescriptorWrite,
																	AlbedoPassSamplerdescriptorWrite,LightingPassSamplerdescriptorWrite,BlueNoiseSamplerdescriptorWrite,
																	fragmentUniformdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}


	{
		// create sets from the pool based on the layout
		// 	     
		std::vector<vk::DescriptorSetLayout> BIlateriallayouts(MAX_FRAMES_IN_FLIGHT, TemporalAccumilationDescriptorSetLayout);


		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = BIlateriallayouts.data();

		TemporalAccumilationFullDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, TemporalAccumilationFullDescriptorSets.data());

		////////////////////////////////////////////////////////////////////////////////////////////////
		//specifies what exactly to send
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			/////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorImageInfo NoisyGIimageInfo{};
			NoisyGIimageInfo.imageLayout = vk::ImageLayout::eGeneral;
			NoisyGIimageInfo.imageView =SSGIPassImage.imageView;
			NoisyGIimageInfo.sampler = SSGIPassImage.imageSampler;

			vk::WriteDescriptorSet NoisyGISamplerdescriptorWrite{};
			NoisyGISamplerdescriptorWrite.dstSet = TemporalAccumilationFullDescriptorSets[i];
			NoisyGISamplerdescriptorWrite.dstBinding = 0;
			NoisyGISamplerdescriptorWrite.dstArrayElement = 0;
			NoisyGISamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			NoisyGISamplerdescriptorWrite.descriptorCount = 1;
			NoisyGISamplerdescriptorWrite.pImageInfo = &NoisyGIimageInfo;


			vk::DescriptorImageInfo LastFrameGIimageInfo{};
			LastFrameGIimageInfo.imageLayout = vk::ImageLayout::eGeneral;
			LastFrameGIimageInfo.imageView = SSGIPassLastFrameImage.imageView;
			LastFrameGIimageInfo.sampler = SSGIPassLastFrameImage.imageSampler;

			vk::WriteDescriptorSet LastFrameGISamplerdescriptorWrite{};
			LastFrameGISamplerdescriptorWrite.dstSet = TemporalAccumilationFullDescriptorSets[i];
			LastFrameGISamplerdescriptorWrite.dstBinding = 1;
			LastFrameGISamplerdescriptorWrite.dstArrayElement = 0;
			LastFrameGISamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			LastFrameGISamplerdescriptorWrite.descriptorCount = 1;
			LastFrameGISamplerdescriptorWrite.pImageInfo = &LastFrameGIimageInfo;



			std::array<vk::WriteDescriptorSet, 2> TAGIdescriptorWrites{ NoisyGISamplerdescriptorWrite ,LastFrameGISamplerdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(TAGIdescriptorWrites.size(), TAGIdescriptorWrites.data(), 0, nullptr);
		}
	}



	{   
		std::vector<vk::DescriptorSetLayout> BIlateriallayouts(MAX_FRAMES_IN_FLIGHT, Blured_TemporalAccumilationDescriptorSetLayout);


		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = BIlateriallayouts.data();

		DownSampleHalfRes_PING_SampleDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, DownSampleHalfRes_PING_SampleDescriptorSets.data());

		////////////////////////////////////////////////////////////////////////////////////////////////
		//specifies what exactly to send
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			/////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorImageInfo NoisyTAimageInfo{};
			NoisyTAimageInfo.imageLayout = vk::ImageLayout::eGeneral;
			NoisyTAimageInfo.imageView   = SSGIAccumilationImage.imageView;
			NoisyTAimageInfo.sampler     = SSGIAccumilationImage.imageSampler;

			vk::WriteDescriptorSet NoisyTASamplerdescriptorWrite{};
			NoisyTASamplerdescriptorWrite.dstSet = DownSampleHalfRes_PING_SampleDescriptorSets[i];
			NoisyTASamplerdescriptorWrite.dstBinding = 0;
			NoisyTASamplerdescriptorWrite.dstArrayElement = 0;
			NoisyTASamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			NoisyTASamplerdescriptorWrite.descriptorCount = 1;
			NoisyTASamplerdescriptorWrite.pImageInfo = &NoisyTAimageInfo;

			vk::DescriptorImageInfo ViewSpacePositionImage{};
			ViewSpacePositionImage.imageLayout = vk::ImageLayout::eGeneral;
			ViewSpacePositionImage.imageView   = gbuffer.ViewSpacePosition.imageView;
			ViewSpacePositionImage.sampler     = gbuffer.ViewSpacePosition.imageSampler;

			vk::WriteDescriptorSet ViewSpacePositionImageSamplerdescriptorWrite{};
			ViewSpacePositionImageSamplerdescriptorWrite.dstSet = DownSampleHalfRes_PING_SampleDescriptorSets[i];
			ViewSpacePositionImageSamplerdescriptorWrite.dstBinding = 1;
			ViewSpacePositionImageSamplerdescriptorWrite.dstArrayElement = 0;
			ViewSpacePositionImageSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			ViewSpacePositionImageSamplerdescriptorWrite.descriptorCount = 1;
			ViewSpacePositionImageSamplerdescriptorWrite.pImageInfo = &ViewSpacePositionImage;

			std::array<vk::WriteDescriptorSet, 2> TAGIdescriptorWrites{ NoisyTASamplerdescriptorWrite,ViewSpacePositionImageSamplerdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(TAGIdescriptorWrites.size(), TAGIdescriptorWrites.data(), 0, nullptr);
		}
	}



	{
		std::vector<vk::DescriptorSetLayout> BIlateriallayouts(MAX_FRAMES_IN_FLIGHT, Blured_TemporalAccumilationDescriptorSetLayout);


		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = BIlateriallayouts.data();

		DownSampleHalfRes_PONG_SampleDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, DownSampleHalfRes_PONG_SampleDescriptorSets.data());

		////////////////////////////////////////////////////////////////////////////////////////////////
		//specifies what exactly to send
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			/////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorImageInfo HorizontalBluredimageInfo{};
			HorizontalBluredimageInfo.imageLayout = vk::ImageLayout::eGeneral;
			HorizontalBluredimageInfo.imageView = BlurPing_DownSampleHalfRes.imageView;
			HorizontalBluredimageInfo.sampler   = BlurPing_DownSampleHalfRes.imageSampler;

			vk::WriteDescriptorSet HorizontalBluredSamplerdescriptorWrite{};
			HorizontalBluredSamplerdescriptorWrite.dstSet = DownSampleHalfRes_PONG_SampleDescriptorSets[i];
			HorizontalBluredSamplerdescriptorWrite.dstBinding = 0;
			HorizontalBluredSamplerdescriptorWrite.dstArrayElement = 0;
			HorizontalBluredSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			HorizontalBluredSamplerdescriptorWrite.descriptorCount = 1;
			HorizontalBluredSamplerdescriptorWrite.pImageInfo = &HorizontalBluredimageInfo;

			vk::DescriptorImageInfo ViewSpacePositionImage{};
			ViewSpacePositionImage.imageLayout = vk::ImageLayout::eGeneral;
			ViewSpacePositionImage.imageView = gbuffer.ViewSpacePosition.imageView;
			ViewSpacePositionImage.sampler = gbuffer.ViewSpacePosition.imageSampler;

			vk::WriteDescriptorSet ViewSpacePositionImageSamplerdescriptorWrite{};
			ViewSpacePositionImageSamplerdescriptorWrite.dstSet = DownSampleHalfRes_PONG_SampleDescriptorSets[i];
			ViewSpacePositionImageSamplerdescriptorWrite.dstBinding = 1;
			ViewSpacePositionImageSamplerdescriptorWrite.dstArrayElement = 0;
			ViewSpacePositionImageSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			ViewSpacePositionImageSamplerdescriptorWrite.descriptorCount = 1;
			ViewSpacePositionImageSamplerdescriptorWrite.pImageInfo = &ViewSpacePositionImage;

			std::array<vk::WriteDescriptorSet, 2> BlureddescriptorWrites{ HorizontalBluredSamplerdescriptorWrite,ViewSpacePositionImageSamplerdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(BlureddescriptorWrites.size(), BlureddescriptorWrites.data(), 0, nullptr);
		}
	}


	{
		std::vector<vk::DescriptorSetLayout> BIlateriallayouts(MAX_FRAMES_IN_FLIGHT, Blured_TemporalAccumilationDescriptorSetLayout);


		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = BIlateriallayouts.data();

		DownSampleQuaterRes_PING_SampleDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, DownSampleQuaterRes_PING_SampleDescriptorSets.data());

		////////////////////////////////////////////////////////////////////////////////////////////////
		//specifies what exactly to send
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			/////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorImageInfo HorizontalBluredimageInfo{};
			HorizontalBluredimageInfo.imageLayout = vk::ImageLayout::eGeneral;
			HorizontalBluredimageInfo.imageView   = BlurPong_DownSampleHalfRes.imageView;
			HorizontalBluredimageInfo.sampler     = BlurPong_DownSampleHalfRes.imageSampler;

			vk::WriteDescriptorSet HorizontalBluredSamplerdescriptorWrite{};
			HorizontalBluredSamplerdescriptorWrite.dstSet = DownSampleQuaterRes_PING_SampleDescriptorSets[i];
			HorizontalBluredSamplerdescriptorWrite.dstBinding = 0;
			HorizontalBluredSamplerdescriptorWrite.dstArrayElement = 0;
			HorizontalBluredSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			HorizontalBluredSamplerdescriptorWrite.descriptorCount = 1;
			HorizontalBluredSamplerdescriptorWrite.pImageInfo = &HorizontalBluredimageInfo;

			vk::DescriptorImageInfo ViewSpacePositionImage{};
			ViewSpacePositionImage.imageLayout = vk::ImageLayout::eGeneral;
			ViewSpacePositionImage.imageView = gbuffer.ViewSpacePosition.imageView;
			ViewSpacePositionImage.sampler = gbuffer.ViewSpacePosition.imageSampler;

			vk::WriteDescriptorSet ViewSpacePositionImageSamplerdescriptorWrite{};
			ViewSpacePositionImageSamplerdescriptorWrite.dstSet = DownSampleQuaterRes_PING_SampleDescriptorSets[i];
			ViewSpacePositionImageSamplerdescriptorWrite.dstBinding = 1;
			ViewSpacePositionImageSamplerdescriptorWrite.dstArrayElement = 0;
			ViewSpacePositionImageSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			ViewSpacePositionImageSamplerdescriptorWrite.descriptorCount = 1;
			ViewSpacePositionImageSamplerdescriptorWrite.pImageInfo = &ViewSpacePositionImage;

			std::array<vk::WriteDescriptorSet, 2> BlureddescriptorWrites{ HorizontalBluredSamplerdescriptorWrite,ViewSpacePositionImageSamplerdescriptorWrite };
			vulkanContext->LogicalDevice.updateDescriptorSets(BlureddescriptorWrites.size(), BlureddescriptorWrites.data(), 0, nullptr);
		}
	}


	{
		std::vector<vk::DescriptorSetLayout> BIlateriallayouts(MAX_FRAMES_IN_FLIGHT, Blured_TemporalAccumilationDescriptorSetLayout);


		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = BIlateriallayouts.data();

		DownSampleQuaterRes_PONG_SampleDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, DownSampleQuaterRes_PONG_SampleDescriptorSets.data());

		////////////////////////////////////////////////////////////////////////////////////////////////
		//specifies what exactly to send
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			/////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorImageInfo HorizontalBluredimageInfo{};
			HorizontalBluredimageInfo.imageLayout = vk::ImageLayout::eGeneral;
			HorizontalBluredimageInfo.imageView = BlurPing_DownSampleQuaterRes.imageView;
			HorizontalBluredimageInfo.sampler   = BlurPing_DownSampleQuaterRes.imageSampler;

			vk::WriteDescriptorSet HorizontalBluredSamplerdescriptorWrite{};
			HorizontalBluredSamplerdescriptorWrite.dstSet = DownSampleQuaterRes_PONG_SampleDescriptorSets[i];
			HorizontalBluredSamplerdescriptorWrite.dstBinding = 0;
			HorizontalBluredSamplerdescriptorWrite.dstArrayElement = 0;
			HorizontalBluredSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			HorizontalBluredSamplerdescriptorWrite.descriptorCount = 1;
			HorizontalBluredSamplerdescriptorWrite.pImageInfo = &HorizontalBluredimageInfo;


			vk::DescriptorImageInfo ViewSpacePositionImage{};
			ViewSpacePositionImage.imageLayout = vk::ImageLayout::eGeneral;
			ViewSpacePositionImage.imageView = gbuffer.ViewSpacePosition.imageView;
			ViewSpacePositionImage.sampler = gbuffer.ViewSpacePosition.imageSampler;

			vk::WriteDescriptorSet ViewSpacePositionImageSamplerdescriptorWrite{};
			ViewSpacePositionImageSamplerdescriptorWrite.dstSet = DownSampleQuaterRes_PONG_SampleDescriptorSets[i];
			ViewSpacePositionImageSamplerdescriptorWrite.dstBinding = 1;
			ViewSpacePositionImageSamplerdescriptorWrite.dstArrayElement = 0;
			ViewSpacePositionImageSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			ViewSpacePositionImageSamplerdescriptorWrite.descriptorCount = 1;
			ViewSpacePositionImageSamplerdescriptorWrite.pImageInfo = &ViewSpacePositionImage;

			std::array<vk::WriteDescriptorSet, 2> BlureddescriptorWrites{ HorizontalBluredSamplerdescriptorWrite,ViewSpacePositionImageSamplerdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(BlureddescriptorWrites.size(), BlureddescriptorWrites.data(), 0, nullptr);
		}
	}


	{
		std::vector<vk::DescriptorSetLayout> BIlateriallayouts(MAX_FRAMES_IN_FLIGHT, Blured_TemporalAccumilationDescriptorSetLayout);


		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = BIlateriallayouts.data();

		UPSampleHalfRes_PING_SampleDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, UPSampleHalfRes_PING_SampleDescriptorSets.data());

		////////////////////////////////////////////////////////////////////////////////////////////////
		//specifies what exactly to send
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			/////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorImageInfo HorizontalBluredimageInfo{};
			HorizontalBluredimageInfo.imageLayout = vk::ImageLayout::eGeneral;
			HorizontalBluredimageInfo.imageView = BlurPong_DownSampleQuaterRes.imageView;
			HorizontalBluredimageInfo.sampler   = BlurPong_DownSampleQuaterRes.imageSampler;

			vk::WriteDescriptorSet HorizontalBluredSamplerdescriptorWrite{};
			HorizontalBluredSamplerdescriptorWrite.dstSet = UPSampleHalfRes_PING_SampleDescriptorSets[i];
			HorizontalBluredSamplerdescriptorWrite.dstBinding = 0;
			HorizontalBluredSamplerdescriptorWrite.dstArrayElement = 0;
			HorizontalBluredSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			HorizontalBluredSamplerdescriptorWrite.descriptorCount = 1;
			HorizontalBluredSamplerdescriptorWrite.pImageInfo = &HorizontalBluredimageInfo;


			vk::DescriptorImageInfo ViewSpacePositionImage{};
			ViewSpacePositionImage.imageLayout = vk::ImageLayout::eGeneral;
			ViewSpacePositionImage.imageView = gbuffer.ViewSpacePosition.imageView;
			ViewSpacePositionImage.sampler = gbuffer.ViewSpacePosition.imageSampler;

			vk::WriteDescriptorSet ViewSpacePositionImageSamplerdescriptorWrite{};
			ViewSpacePositionImageSamplerdescriptorWrite.dstSet = UPSampleHalfRes_PING_SampleDescriptorSets[i];
			ViewSpacePositionImageSamplerdescriptorWrite.dstBinding = 1;
			ViewSpacePositionImageSamplerdescriptorWrite.dstArrayElement = 0;
			ViewSpacePositionImageSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			ViewSpacePositionImageSamplerdescriptorWrite.descriptorCount = 1;
			ViewSpacePositionImageSamplerdescriptorWrite.pImageInfo = &ViewSpacePositionImage;

			std::array<vk::WriteDescriptorSet, 2> BlureddescriptorWrites{ HorizontalBluredSamplerdescriptorWrite,ViewSpacePositionImageSamplerdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(BlureddescriptorWrites.size(), BlureddescriptorWrites.data(), 0, nullptr);
		}
	}


	{
		std::vector<vk::DescriptorSetLayout> BIlateriallayouts(MAX_FRAMES_IN_FLIGHT, Blured_TemporalAccumilationDescriptorSetLayout);


		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = BIlateriallayouts.data();

		UPSampleHalfRes_PONG_SampleDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, UPSampleHalfRes_PONG_SampleDescriptorSets.data());

		////////////////////////////////////////////////////////////////////////////////////////////////
		//specifies what exactly to send
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			/////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorImageInfo HorizontalBluredimageInfo{};
			HorizontalBluredimageInfo.imageLayout = vk::ImageLayout::eGeneral;
			HorizontalBluredimageInfo.imageView = BlurPing_UPSampleHalfRes.imageView;
			HorizontalBluredimageInfo.sampler   = BlurPing_UPSampleHalfRes.imageSampler;

			vk::WriteDescriptorSet HorizontalBluredSamplerdescriptorWrite{};
			HorizontalBluredSamplerdescriptorWrite.dstSet = UPSampleHalfRes_PONG_SampleDescriptorSets[i];
			HorizontalBluredSamplerdescriptorWrite.dstBinding = 0;
			HorizontalBluredSamplerdescriptorWrite.dstArrayElement = 0;
			HorizontalBluredSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			HorizontalBluredSamplerdescriptorWrite.descriptorCount = 1;
			HorizontalBluredSamplerdescriptorWrite.pImageInfo = &HorizontalBluredimageInfo;


			vk::DescriptorImageInfo ViewSpacePositionImage{};
			ViewSpacePositionImage.imageLayout = vk::ImageLayout::eGeneral;
			ViewSpacePositionImage.imageView = gbuffer.ViewSpacePosition.imageView;
			ViewSpacePositionImage.sampler = gbuffer.ViewSpacePosition.imageSampler;

			vk::WriteDescriptorSet ViewSpacePositionImageSamplerdescriptorWrite{};
			ViewSpacePositionImageSamplerdescriptorWrite.dstSet = UPSampleHalfRes_PONG_SampleDescriptorSets[i];
			ViewSpacePositionImageSamplerdescriptorWrite.dstBinding = 1;
			ViewSpacePositionImageSamplerdescriptorWrite.dstArrayElement = 0;
			ViewSpacePositionImageSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			ViewSpacePositionImageSamplerdescriptorWrite.descriptorCount = 1;
			ViewSpacePositionImageSamplerdescriptorWrite.pImageInfo = &ViewSpacePositionImage;

			std::array<vk::WriteDescriptorSet, 2> BlureddescriptorWrites{ HorizontalBluredSamplerdescriptorWrite,ViewSpacePositionImageSamplerdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(BlureddescriptorWrites.size(), BlureddescriptorWrites.data(), 0, nullptr);
		}
	}


	{
		std::vector<vk::DescriptorSetLayout> BIlateriallayouts(MAX_FRAMES_IN_FLIGHT, Blured_TemporalAccumilationDescriptorSetLayout);


		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = BIlateriallayouts.data();

		UPSampleFullRes_PING_SampleDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, UPSampleFullRes_PING_SampleDescriptorSets.data());

		////////////////////////////////////////////////////////////////////////////////////////////////
		//specifies what exactly to send
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			/////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorImageInfo HorizontalBluredimageInfo{};
			HorizontalBluredimageInfo.imageLayout = vk::ImageLayout::eGeneral;
			HorizontalBluredimageInfo.imageView = BlurPong_UPSampleHalfRes.imageView;
			HorizontalBluredimageInfo.sampler = BlurPong_UPSampleHalfRes.imageSampler;

			vk::WriteDescriptorSet HorizontalBluredSamplerdescriptorWrite{};
			HorizontalBluredSamplerdescriptorWrite.dstSet = UPSampleFullRes_PING_SampleDescriptorSets[i];
			HorizontalBluredSamplerdescriptorWrite.dstBinding = 0;
			HorizontalBluredSamplerdescriptorWrite.dstArrayElement = 0;
			HorizontalBluredSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			HorizontalBluredSamplerdescriptorWrite.descriptorCount = 1;
			HorizontalBluredSamplerdescriptorWrite.pImageInfo = &HorizontalBluredimageInfo;


			vk::DescriptorImageInfo ViewSpacePositionImage{};
			ViewSpacePositionImage.imageLayout = vk::ImageLayout::eGeneral;
			ViewSpacePositionImage.imageView = gbuffer.ViewSpacePosition.imageView;
			ViewSpacePositionImage.sampler = gbuffer.ViewSpacePosition.imageSampler;

			vk::WriteDescriptorSet ViewSpacePositionImageSamplerdescriptorWrite{};
			ViewSpacePositionImageSamplerdescriptorWrite.dstSet = UPSampleFullRes_PING_SampleDescriptorSets[i];
			ViewSpacePositionImageSamplerdescriptorWrite.dstBinding = 1;
			ViewSpacePositionImageSamplerdescriptorWrite.dstArrayElement = 0;
			ViewSpacePositionImageSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			ViewSpacePositionImageSamplerdescriptorWrite.descriptorCount = 1;
			ViewSpacePositionImageSamplerdescriptorWrite.pImageInfo = &ViewSpacePositionImage;

			std::array<vk::WriteDescriptorSet, 2> BlureddescriptorWrites{ HorizontalBluredSamplerdescriptorWrite,ViewSpacePositionImageSamplerdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(BlureddescriptorWrites.size(), BlureddescriptorWrites.data(), 0, nullptr);
		}
	}


	{
		std::vector<vk::DescriptorSetLayout> BIlateriallayouts(MAX_FRAMES_IN_FLIGHT, Blured_TemporalAccumilationDescriptorSetLayout);


		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = BIlateriallayouts.data();

		UPSampleFullRes_PONG_SampleDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, UPSampleFullRes_PONG_SampleDescriptorSets.data());

		////////////////////////////////////////////////////////////////////////////////////////////////
		//specifies what exactly to send
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			/////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorImageInfo HorizontalBluredimageInfo{};
			HorizontalBluredimageInfo.imageLayout = vk::ImageLayout::eGeneral;
			HorizontalBluredimageInfo.imageView = BlurPing_UPSampleFullRes.imageView;
			HorizontalBluredimageInfo.sampler = BlurPing_UPSampleFullRes.imageSampler;

			vk::WriteDescriptorSet HorizontalBluredSamplerdescriptorWrite{};
			HorizontalBluredSamplerdescriptorWrite.dstSet = UPSampleFullRes_PONG_SampleDescriptorSets[i];
			HorizontalBluredSamplerdescriptorWrite.dstBinding = 0;
			HorizontalBluredSamplerdescriptorWrite.dstArrayElement = 0;
			HorizontalBluredSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			HorizontalBluredSamplerdescriptorWrite.descriptorCount = 1;
			HorizontalBluredSamplerdescriptorWrite.pImageInfo = &HorizontalBluredimageInfo;


			vk::DescriptorImageInfo ViewSpacePositionImage{};
			ViewSpacePositionImage.imageLayout = vk::ImageLayout::eGeneral;
			ViewSpacePositionImage.imageView = gbuffer.ViewSpacePosition.imageView;
			ViewSpacePositionImage.sampler = gbuffer.ViewSpacePosition.imageSampler;

			vk::WriteDescriptorSet ViewSpacePositionImageSamplerdescriptorWrite{};
			ViewSpacePositionImageSamplerdescriptorWrite.dstSet = UPSampleFullRes_PONG_SampleDescriptorSets[i];
			ViewSpacePositionImageSamplerdescriptorWrite.dstBinding = 1;
			ViewSpacePositionImageSamplerdescriptorWrite.dstArrayElement = 0;
			ViewSpacePositionImageSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			ViewSpacePositionImageSamplerdescriptorWrite.descriptorCount = 1;
			ViewSpacePositionImageSamplerdescriptorWrite.pImageInfo = &ViewSpacePositionImage;

			std::array<vk::WriteDescriptorSet, 2> BlureddescriptorWrites{ HorizontalBluredSamplerdescriptorWrite,ViewSpacePositionImageSamplerdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(BlureddescriptorWrites.size(), BlureddescriptorWrites.data(), 0, nullptr);
		}
	}

}

void SSGI::UpdateUniformBuffer(uint32_t currentImage, std::vector<std::shared_ptr<Light>>& lightref, float DeltaTime)
{
	 NoiseIndex = (NoiseIndex + 1)  % BlueNoiseTextures.size();

	SSGI_UniformBufferData SSGI_UniformBufferData;
 	SSGI_UniformBufferData.ProjectionMatrix = camera->GetProjectionMatrix();
	SSGI_UniformBufferData.ProjectionMatrix[1][1] *= -1;
	SSGI_UniformBufferData.BlueNoiseImageIndex_WithPadding = glm::vec4(NoiseIndex, DeltaTime,0,0);

	memcpy(FragmentUniformBuffersMappedMem[currentImage], &SSGI_UniformBufferData, sizeof(SSGI_UniformBufferData));

	if (LastCameraMatrix != camera->GetViewMatrix())
	{
		vulkanContext->ResetTemporalAccumilation();

		LastCameraMatrix = camera->GetViewMatrix();
	}

	vulkanContext->AccumilationCount++;
}


void SSGI::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };

	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint16);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(quadIndices.size(), 1, 0, 0, 0);

}

void SSGI::DrawTA(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
{

	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };

	commandbuffer.pushConstants(pipelinelayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(vulkanContext->AccumilationCount), &vulkanContext->AccumilationCount);

	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint16);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &TemporalAccumilationFullDescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(quadIndices.size(), 1, 0, 0, 0);
}

void SSGI::DrawDownSampleHalfResFirstPass(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
{

	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };

	int Direction = true;
	commandbuffer.pushConstants(pipelinelayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(int), &Direction);

	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint16);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DownSampleHalfRes_PING_SampleDescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(quadIndices.size(), 1, 0, 0, 0);
}

void SSGI::DrawDownSampleHalfResSecondPass(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
{

	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };

	int Direction = false;
	commandbuffer.pushConstants(pipelinelayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(int), &Direction);

	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint16);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DownSampleHalfRes_PONG_SampleDescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(quadIndices.size(), 1, 0, 0, 0);
}

void SSGI::DrawDownSampleQuaterfResFirstPass(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
{

	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };

	int Direction = true;
	commandbuffer.pushConstants(pipelinelayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(int), &Direction);

	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint16);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DownSampleQuaterRes_PING_SampleDescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(quadIndices.size(), 1, 0, 0, 0);
}

void SSGI::DrawDownSampleQuaterfResSecondPass(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
{

	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };

	int Direction = false;
	commandbuffer.pushConstants(pipelinelayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(int), &Direction);

	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint16);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DownSampleQuaterRes_PONG_SampleDescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(quadIndices.size(), 1, 0, 0, 0);
}

void SSGI::DrawUPSampleHalfResFirstPass(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
{

	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };

	int Direction = true;
	commandbuffer.pushConstants(pipelinelayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(int), &Direction);

	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint16);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &UPSampleHalfRes_PING_SampleDescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(quadIndices.size(), 1, 0, 0, 0);
}

void SSGI::DrawUPSampleHalfResSecondPass(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
{

	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };

	int Direction = false;
	commandbuffer.pushConstants(pipelinelayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(int), &Direction);

	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint16);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &UPSampleHalfRes_PONG_SampleDescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(quadIndices.size(), 1, 0, 0, 0);
}

void SSGI::DrawUPSampleFullResFirstPass(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
{

	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };

	int Direction = true;
	commandbuffer.pushConstants(pipelinelayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(int), &Direction);

	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint16);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &UPSampleFullRes_PING_SampleDescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(quadIndices.size(), 1, 0, 0, 0);
}

void SSGI::DrawUPSampleFullResSecondPass(vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
{

	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };

	int Direction = false;
	commandbuffer.pushConstants(pipelinelayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(int), &Direction);

	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint16);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &UPSampleFullRes_PONG_SampleDescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(quadIndices.size(), 1, 0, 0, 0);
}


void SSGI::CleanUp()
{
	if (bufferManager)
	{

		for (ImageData noise : BlueNoiseTextures)
		{
			bufferManager->DestroyImage(noise);
		}
		
		BlueNoiseTextures.clear();

		vulkanContext->LogicalDevice.destroyDescriptorSetLayout(TemporalAccumilationDescriptorSetLayout);
		vulkanContext->LogicalDevice.destroyDescriptorSetLayout(Blured_TemporalAccumilationDescriptorSetLayout);

	}
	Drawable::Destructor();

}



