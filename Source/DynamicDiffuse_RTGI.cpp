#include "DynamicDiffuse_RTGI.h"
#include "VulkanContext.h"
#include "BufferManager.h"
#include "Camera.h"
#include "SkyBox.h"
#include <stdexcept>
#include "AssetManager.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

DynamicDiffuse_RTGI::DynamicDiffuse_RTGI(const std::string filepath, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger, SkyBox* skybox)
{
	bufferManager = buffermanger;
	vulkanContext = vulkancontext;
	camera        = rcamera;
	commandPool   = commandpool;
	FilePath = filepath;
	skyboxRef = skybox;

	CreateVertexAndIndexBuffer();
	CreateStorageBuffer();
	createRayTracingDescriptorSetLayout();
	CreateAtlasImages();
}
void DynamicDiffuse_RTGI::CreateVertexAndIndexBuffer() {

	storedModelData = &AssetManager::GetInstance().GetStoredModelData(FilePath);

	VkDeviceSize VertexBufferSize = sizeof(storedModelData->VertexData[0]) * storedModelData->VertexData.size();
	vertexBufferData.BufferID = "Model Vertex Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&vertexBufferData, storedModelData->VertexData.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(uint32_t) * storedModelData->IndexData.size();
	indexBufferData.BufferID = "Model Index Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&indexBufferData, storedModelData->IndexData.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer, commandPool, vulkanContext->graphicsQueue);
}

void DynamicDiffuse_RTGI::CreateStorageBuffer()
{
	{
		ProbeDataStorageBuffers.resize(1);

		VkDeviceSize ComputeStorageBufferSize = sizeof(ProbeInformation) * 2000;

		for (size_t i = 0; i < 1; i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "Probe Data Buffer" + i;
			bufferManager->CreateGPU_Only_Buffer(&bufferdata, ComputeStorageBufferSize, vk::BufferUsageFlagBits::eStorageBuffer, commandPool, vulkanContext->graphicsQueue);
			ProbeDataStorageBuffers[i] = bufferdata;
		}
	}

	{
		ProbeFibonacciDirectionsStorageBuffers.resize(1);

		VkDeviceSize ComputeStorageBufferSize = sizeof(glm::vec4) * 300;

		for (size_t i = 0; i < 1; i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "Probe Fibonacci Buffer" + i;
			bufferManager->CreateGPU_Only_Buffer(&bufferdata, ComputeStorageBufferSize, vk::BufferUsageFlagBits::eStorageBuffer, commandPool, vulkanContext->graphicsQueue);
			ProbeFibonacciDirectionsStorageBuffers[i] = bufferdata;
		}
	}
}

void DynamicDiffuse_RTGI::CreateAtlasImages() {

	int ProbeNum = NumOfProbesX * NumOfProbesY * NumOfProbesZ;
	RadianceImageExtent = vk::Extent3D(RaysPerProbe, ProbeNum, 1);

	RadianceImageAtlasImage.ImageID = " DDGI Radiance Atlas Image"; 
	bufferManager->CreateImage(&RadianceImageAtlasImage, RadianceImageExtent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc);
	RadianceImageAtlasImage.imageView = bufferManager->CreateImageView(&RadianceImageAtlasImage, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	RadianceImageAtlasImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge, false);

	////////////////////////////////////////////////////////////
	int IrradianceSize = (ProbeSideLength + GutterSize);
	int probesPerRow = static_cast<int>(ceil(sqrt(ProbeNum))); // find square root and round up
	int AtlasSize = IrradianceSize * probesPerRow;

	IradianceImageExtent = vk::Extent3D(AtlasSize, AtlasSize, 1);

	IradianceImageAtlasImage.ImageID = " DDGI Irradiance Atlas Image";
	bufferManager->CreateImage(&IradianceImageAtlasImage, IradianceImageExtent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc);
	IradianceImageAtlasImage.imageView = bufferManager->CreateImageView(&IradianceImageAtlasImage, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	IradianceImageAtlasImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge, true);

	VisibilityImageAtlasImage.ImageID = " DDGI Visibility Atlas Image";
	bufferManager->CreateImage(&VisibilityImageAtlasImage, IradianceImageExtent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc);
	VisibilityImageAtlasImage.imageView = bufferManager->CreateImageView(&VisibilityImageAtlasImage, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	VisibilityImageAtlasImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge, true);

	Prev_IradianceImageAtlasImage.ImageID = " DDGI Prev Irradiance Atlas Image";
	bufferManager->CreateImage(&Prev_IradianceImageAtlasImage, IradianceImageExtent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc);
	Prev_IradianceImageAtlasImage.imageView = bufferManager->CreateImageView(&Prev_IradianceImageAtlasImage, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	Prev_IradianceImageAtlasImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge, true);

	Prev_VisibilityImageAtlasImage.ImageID = " DDGI Visibility Atlas Image";
	bufferManager->CreateImage(&Prev_VisibilityImageAtlasImage, IradianceImageExtent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc);
	Prev_VisibilityImageAtlasImage.imageView = bufferManager->CreateImageView(&Prev_VisibilityImageAtlasImage, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	Prev_VisibilityImageAtlasImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge, true);

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

	bufferManager->TransitionImage(cmd, &RadianceImageAtlasImage, toClear);
	cmd.clearColorImage(RadianceImageAtlasImage.image, vk::ImageLayout::eTransferDstOptimal, clearColor, range);
	bufferManager->TransitionImage(cmd, &RadianceImageAtlasImage, toGeneral);

	bufferManager->TransitionImage(cmd, &Prev_IradianceImageAtlasImage, toClear);
	cmd.clearColorImage(Prev_IradianceImageAtlasImage.image, vk::ImageLayout::eTransferDstOptimal, clearColor, range);
	bufferManager->TransitionImage(cmd, &Prev_IradianceImageAtlasImage, toGeneral);

	bufferManager->TransitionImage(cmd, &IradianceImageAtlasImage, toClear);
	cmd.clearColorImage(IradianceImageAtlasImage.image, vk::ImageLayout::eTransferDstOptimal, clearColor, range);
	bufferManager->TransitionImage(cmd, &IradianceImageAtlasImage, toGeneral);

	bufferManager->TransitionImage(cmd, &VisibilityImageAtlasImage, toClear);
	cmd.clearColorImage(VisibilityImageAtlasImage.image, vk::ImageLayout::eTransferDstOptimal, clearColor, range);
	bufferManager->TransitionImage(cmd, &VisibilityImageAtlasImage, toGeneral);


	bufferManager->TransitionImage(cmd, &Prev_VisibilityImageAtlasImage, toClear);
	cmd.clearColorImage(Prev_VisibilityImageAtlasImage.image, vk::ImageLayout::eTransferDstOptimal, clearColor, range);
	bufferManager->TransitionImage(cmd, &Prev_VisibilityImageAtlasImage, toGeneral);

	bufferManager->SubmitAndDestoyCommandBuffer(commandPool, cmd, vulkanContext->graphicsQueue);

}

void DynamicDiffuse_RTGI::CreateSampledGIImage() {

	// Note: This extent is deliberately different from the one in CreateAtlasImages
	vk::Extent3D SampledImageExtent = vk::Extent3D(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, 1);

	Probe_Sampled_GI_Image.ImageID = " Sampled DDGI Image";
	bufferManager->CreateImage(&Probe_Sampled_GI_Image, SampledImageExtent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);	Probe_Sampled_GI_Image.imageView = bufferManager->CreateImageView(&Probe_Sampled_GI_Image, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	Probe_Sampled_GI_Image.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge, true);


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

	bufferManager->TransitionImage(cmd, &Probe_Sampled_GI_Image, toClear);
	cmd.clearColorImage(Probe_Sampled_GI_Image.image, vk::ImageLayout::eTransferDstOptimal, clearColor, range);
	bufferManager->TransitionImage(cmd, &Probe_Sampled_GI_Image, toGeneral);

	bufferManager->SubmitAndDestoyCommandBuffer(commandPool, cmd, vulkanContext->graphicsQueue);
}


void DynamicDiffuse_RTGI::DestroyAtlasImages() {

	if (RadianceImageAtlasImage.image)
	{
		bufferManager->DestroyImage(RadianceImageAtlasImage);
	}

	if (IradianceImageAtlasImage.image)
	{
		bufferManager->DestroyImage(IradianceImageAtlasImage);
	}

	if (VisibilityImageAtlasImage.image)
	{
		bufferManager->DestroyImage(VisibilityImageAtlasImage);
	}

	if (Prev_IradianceImageAtlasImage.image)
	{
		bufferManager->DestroyImage(Prev_IradianceImageAtlasImage);
	}

	if (Prev_VisibilityImageAtlasImage.image)
	{
		bufferManager->DestroyImage(Prev_VisibilityImageAtlasImage);
	}
}

void DynamicDiffuse_RTGI::DestroySampledGIImage() {

	if (Probe_Sampled_GI_Image.image)
	{
		bufferManager->DestroyImage(Probe_Sampled_GI_Image);
	}
}


void DynamicDiffuse_RTGI::createRayTracingDescriptorSetLayout(){

	{
		vk::DescriptorSetLayoutBinding IrradianceAtlasSamplerLayout{};
		IrradianceAtlasSamplerLayout.binding = 0;
		IrradianceAtlasSamplerLayout.descriptorCount = 1;
		IrradianceAtlasSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		IrradianceAtlasSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding  ProbeLocationUniformBuffer{};
		ProbeLocationUniformBuffer.binding = 1;
		ProbeLocationUniformBuffer.descriptorCount = 1;
		ProbeLocationUniformBuffer.descriptorType = vk::DescriptorType::eStorageBuffer;
		ProbeLocationUniformBuffer.stageFlags = vk::ShaderStageFlagBits::eVertex;

		std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
																   IrradianceAtlasSamplerLayout,
																   ProbeLocationUniformBuffer
		};

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &ProbeDescriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
	}


	{
		vk::DescriptorSetLayoutBinding ProbePositionStorageBufffer{};
		ProbePositionStorageBufffer.binding = 0;
		ProbePositionStorageBufffer.descriptorCount = 1;
		ProbePositionStorageBufffer.descriptorType = vk::DescriptorType::eStorageBuffer;
		ProbePositionStorageBufffer.stageFlags = vk::ShaderStageFlagBits::eCompute;

		vk::DescriptorSetLayoutBinding FibonacciDirectionsStorageBufffer{};
		FibonacciDirectionsStorageBufffer.binding = 1;
		FibonacciDirectionsStorageBufffer.descriptorCount = 1;
		FibonacciDirectionsStorageBufffer.descriptorType = vk::DescriptorType::eStorageBuffer;
		FibonacciDirectionsStorageBufffer.stageFlags = vk::ShaderStageFlagBits::eCompute;

		std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { ProbePositionStorageBufffer,FibonacciDirectionsStorageBufffer };

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &GridDescriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
	}

	{
		vk::DescriptorSetLayoutBinding FibonacciDirectionsStorageBufffer{};
		FibonacciDirectionsStorageBufffer.binding = 0;
		FibonacciDirectionsStorageBufffer.descriptorCount = 1;
		FibonacciDirectionsStorageBufffer.descriptorType = vk::DescriptorType::eStorageBuffer;
		FibonacciDirectionsStorageBufffer.stageFlags = vk::ShaderStageFlagBits::eCompute;

		vk::DescriptorSetLayoutBinding RadianceStorageImage{};
		RadianceStorageImage.binding = 1;
		RadianceStorageImage.descriptorCount = 1;
		RadianceStorageImage.descriptorType = vk::DescriptorType::eStorageImage;
		RadianceStorageImage.stageFlags = vk::ShaderStageFlagBits::eCompute;

		vk::DescriptorSetLayoutBinding IrradianceStorageImage{};
		IrradianceStorageImage.binding = 2;
		IrradianceStorageImage.descriptorCount = 1;
		IrradianceStorageImage.descriptorType = vk::DescriptorType::eStorageImage;
		IrradianceStorageImage.stageFlags = vk::ShaderStageFlagBits::eCompute;

		vk::DescriptorSetLayoutBinding VisibilityStorageImage{};
		VisibilityStorageImage.binding = 3;
		VisibilityStorageImage.descriptorCount = 1;
		VisibilityStorageImage.descriptorType = vk::DescriptorType::eStorageImage;
		VisibilityStorageImage.stageFlags = vk::ShaderStageFlagBits::eCompute;

		vk::DescriptorSetLayoutBinding PrevIrradianceStorageImage{};
		PrevIrradianceStorageImage.binding = 4;
		PrevIrradianceStorageImage.descriptorCount = 1;
		PrevIrradianceStorageImage.descriptorType = vk::DescriptorType::eStorageImage;
		PrevIrradianceStorageImage.stageFlags = vk::ShaderStageFlagBits::eCompute;

		vk::DescriptorSetLayoutBinding PrevVisibilityStorageImage{};
		PrevVisibilityStorageImage.binding = 5;
		PrevVisibilityStorageImage.descriptorCount = 1;
		PrevVisibilityStorageImage.descriptorType = vk::DescriptorType::eStorageImage;
		PrevVisibilityStorageImage.stageFlags = vk::ShaderStageFlagBits::eCompute;

		vk::DescriptorSetLayoutBinding ProbeData{};
		ProbeData.binding = 6;
		ProbeData.descriptorCount = 1;
		ProbeData.descriptorType = vk::DescriptorType::eStorageBuffer;
		ProbeData.stageFlags = vk::ShaderStageFlagBits::eCompute;

		std::array<vk::DescriptorSetLayoutBinding, 7> bindings = { FibonacciDirectionsStorageBufffer,RadianceStorageImage,
			                                                       IrradianceStorageImage,VisibilityStorageImage,
		                                                           PrevIrradianceStorageImage,PrevVisibilityStorageImage,ProbeData };

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &ConstructProbeDataDescriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
	}


	{

		vk::DescriptorSetLayoutBinding ProbeInfoStorageImage{};
		ProbeInfoStorageImage.binding = 0;
		ProbeInfoStorageImage.descriptorCount = 1;
		ProbeInfoStorageImage.descriptorType = vk::DescriptorType::eStorageBuffer;
		ProbeInfoStorageImage.stageFlags = vk::ShaderStageFlagBits::eCompute;

		vk::DescriptorSetLayoutBinding IrradianceStorageImage{};
		IrradianceStorageImage.binding = 1;
		IrradianceStorageImage.descriptorCount = 1;
		IrradianceStorageImage.descriptorType = vk::DescriptorType::eStorageImage;
		IrradianceStorageImage.stageFlags = vk::ShaderStageFlagBits::eCompute;

		vk::DescriptorSetLayoutBinding VisibilityStorageImage{};
		VisibilityStorageImage.binding = 2;
		VisibilityStorageImage.descriptorCount = 1;
		VisibilityStorageImage.descriptorType = vk::DescriptorType::eStorageImage;
		VisibilityStorageImage.stageFlags = vk::ShaderStageFlagBits::eCompute;

		vk::DescriptorSetLayoutBinding Radiance_VisibilityStorageImage{};
		Radiance_VisibilityStorageImage.binding = 3;
		Radiance_VisibilityStorageImage.descriptorCount = 1;
		Radiance_VisibilityStorageImage.descriptorType = vk::DescriptorType::eStorageImage;
		Radiance_VisibilityStorageImage.stageFlags = vk::ShaderStageFlagBits::eCompute;

		std::array<vk::DescriptorSetLayoutBinding, 4> bindings = { ProbeInfoStorageImage,IrradianceStorageImage,VisibilityStorageImage,Radiance_VisibilityStorageImage };

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &ProbeStatusDescriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
	}

  {
	vk::DescriptorSetLayoutBinding TLASLayout{};
	TLASLayout.binding = 0;
	TLASLayout.descriptorCount = 1;
	TLASLayout.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
	TLASLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR;

	vk::DescriptorSetLayoutBinding AlbedoAssetTexturesSamplerLayout{};
	AlbedoAssetTexturesSamplerLayout.binding = 1;
	AlbedoAssetTexturesSamplerLayout.descriptorCount = bufferManager->AllScene_Albedo_Images.size();
	AlbedoAssetTexturesSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	AlbedoAssetTexturesSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

	vk::DescriptorSetLayoutBinding NormalAssetTexturesSamplerLayout{};
	NormalAssetTexturesSamplerLayout.binding = 2;
	NormalAssetTexturesSamplerLayout.descriptorCount = bufferManager->AllScene_Normal_Images.size();
	NormalAssetTexturesSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	NormalAssetTexturesSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

	vk::DescriptorSetLayoutBinding MetalicRoughnessAssetTexturesSamplerLayout{};
	MetalicRoughnessAssetTexturesSamplerLayout.binding = 3;
	MetalicRoughnessAssetTexturesSamplerLayout.descriptorCount = bufferManager->AllScene_MetalicRoughness_Images.size();
	MetalicRoughnessAssetTexturesSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	MetalicRoughnessAssetTexturesSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

	vk::DescriptorSetLayoutBinding IndexStorageBuffersLayout{};
	IndexStorageBuffersLayout.binding = 4;
	IndexStorageBuffersLayout.descriptorCount = 1;
	IndexStorageBuffersLayout.descriptorType = vk::DescriptorType::eStorageBuffer;
	IndexStorageBuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

	vk::DescriptorSetLayoutBinding VertexStorageBuffersLayout{};
	VertexStorageBuffersLayout.binding = 5;
	VertexStorageBuffersLayout.descriptorCount = 1;
	VertexStorageBuffersLayout.descriptorType = vk::DescriptorType::eStorageBuffer;
	VertexStorageBuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

	vk::DescriptorSetLayoutBinding offsetStorageBuffersLayout{};
	offsetStorageBuffersLayout.binding = 6;
	offsetStorageBuffersLayout.descriptorCount = 1;
	offsetStorageBuffersLayout.descriptorType = vk::DescriptorType::eStorageBuffer;
	offsetStorageBuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

	vk::DescriptorSetLayoutBinding trasnformationUniformBuffersLayout{};
	trasnformationUniformBuffersLayout.binding = 7;
	trasnformationUniformBuffersLayout.descriptorCount = 1;
	trasnformationUniformBuffersLayout.descriptorType = vk::DescriptorType::eUniformBuffer;
	trasnformationUniformBuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

	vk::DescriptorSetLayoutBinding IrradianceAtlasStorageLayout{};
	IrradianceAtlasStorageLayout.binding = 8;
	IrradianceAtlasStorageLayout.descriptorCount = 1;
	IrradianceAtlasStorageLayout.descriptorType = vk::DescriptorType::eStorageImage;
	IrradianceAtlasStorageLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

	vk::DescriptorSetLayoutBinding ProbeDataBuffersLayout{};
	ProbeDataBuffersLayout.binding = 9;
	ProbeDataBuffersLayout.descriptorCount = 1;
	ProbeDataBuffersLayout.descriptorType = vk::DescriptorType::eStorageBuffer;
	ProbeDataBuffersLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

	vk::DescriptorSetLayoutBinding FibonacciDirectionsBuffersLayout{};
	FibonacciDirectionsBuffersLayout.binding = 10;
	FibonacciDirectionsBuffersLayout.descriptorCount = 1;
	FibonacciDirectionsBuffersLayout.descriptorType = vk::DescriptorType::eStorageBuffer;
	FibonacciDirectionsBuffersLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

	vk::DescriptorSetLayoutBinding LightInformationUniformBuffersLayout{};
	LightInformationUniformBuffersLayout.binding = 11;
	LightInformationUniformBuffersLayout.descriptorCount = 1;
	LightInformationUniformBuffersLayout.descriptorType = vk::DescriptorType::eUniformBuffer;
	LightInformationUniformBuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

	vk::DescriptorSetLayoutBinding SkyBoxImageSamplersLayout{};
	SkyBoxImageSamplersLayout.binding = 12;
	SkyBoxImageSamplersLayout.descriptorCount = skyboxRef->SkyBoxImages.size();
	SkyBoxImageSamplersLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	SkyBoxImageSamplersLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

	vk::DescriptorSetLayoutBinding IrradianceImageStorageLayout{};
	IrradianceImageStorageLayout.binding = 13;
	IrradianceImageStorageLayout.descriptorCount = 1;
	IrradianceImageStorageLayout.descriptorType = vk::DescriptorType::eStorageImage;
	IrradianceImageStorageLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

	vk::DescriptorSetLayoutBinding VisibilityImageStorageLayout{};
	VisibilityImageStorageLayout.binding = 14;
	VisibilityImageStorageLayout.descriptorCount = 1;
	VisibilityImageStorageLayout.descriptorType = vk::DescriptorType::eStorageImage;
	VisibilityImageStorageLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

	vk::DescriptorSetLayoutBinding EmmisiveAssetTexturesSamplerLayout{};
	EmmisiveAssetTexturesSamplerLayout.binding = 15;
	EmmisiveAssetTexturesSamplerLayout.descriptorCount = bufferManager->AllScene_Emissive_Images.size();
	EmmisiveAssetTexturesSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	EmmisiveAssetTexturesSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;


	std::array<vk::DescriptorSetLayoutBinding, 16> bindings = {
															   TLASLayout,
															   AlbedoAssetTexturesSamplerLayout,
															   NormalAssetTexturesSamplerLayout,
															   MetalicRoughnessAssetTexturesSamplerLayout,
															   IndexStorageBuffersLayout,
															   VertexStorageBuffersLayout,
															   offsetStorageBuffersLayout,
															   trasnformationUniformBuffersLayout,
															   IrradianceAtlasStorageLayout,
															   ProbeDataBuffersLayout,FibonacciDirectionsBuffersLayout,
															   LightInformationUniformBuffersLayout,SkyBoxImageSamplersLayout
															   ,IrradianceImageStorageLayout,VisibilityImageStorageLayout,EmmisiveAssetTexturesSamplerLayout
	};



	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();


	if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &RaytracingDescriptorSetLayout) != vk::Result::eSuccess)
	{
		throw std::runtime_error("Failed to create descriptorset layout!");
	}

  }


  {
	  vk::DescriptorSetLayoutBinding PositionImage{};
	  PositionImage.binding = 0;
	  PositionImage.descriptorCount = 1;
	  PositionImage.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	  PositionImage.stageFlags = vk::ShaderStageFlagBits::eCompute;

	  vk::DescriptorSetLayoutBinding NormalImage{};
	  NormalImage.binding = 1;
	  NormalImage.descriptorCount = 1;
	  NormalImage.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	  NormalImage.stageFlags = vk::ShaderStageFlagBits::eCompute;

	  vk::DescriptorSetLayoutBinding IrradianceStorageImage{};
	  IrradianceStorageImage.binding = 2;
	  IrradianceStorageImage.descriptorCount = 1;
	  IrradianceStorageImage.descriptorType = vk::DescriptorType::eStorageImage;
	  IrradianceStorageImage.stageFlags = vk::ShaderStageFlagBits::eCompute;

	  vk::DescriptorSetLayoutBinding SampledGIStorageImage{};
	  SampledGIStorageImage.binding = 3;
	  SampledGIStorageImage.descriptorCount = 1;
	  SampledGIStorageImage.descriptorType = vk::DescriptorType::eStorageImage;
	  SampledGIStorageImage.stageFlags = vk::ShaderStageFlagBits::eCompute;

	  vk::DescriptorSetLayoutBinding VisibilityStorageImage{};
	  VisibilityStorageImage.binding = 4;
	  VisibilityStorageImage.descriptorCount = 1;
	  VisibilityStorageImage.descriptorType = vk::DescriptorType::eStorageImage;
	  VisibilityStorageImage.stageFlags = vk::ShaderStageFlagBits::eCompute;

	  std::array<vk::DescriptorSetLayoutBinding, 5> bindings = { PositionImage,NormalImage,
		                                                         IrradianceStorageImage,SampledGIStorageImage,
		                                                         VisibilityStorageImage };

	  vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	  layoutInfo.pBindings = bindings.data();

	  if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &DDGISamplingDescriptorSetLayout) != vk::Result::eSuccess)
	  {
		  throw std::runtime_error("Failed to create descriptorset layout!");
	  }
	}




}

void DynamicDiffuse_RTGI::createDescriptorSets(vk::DescriptorPool descriptorpool, GBuffer gbuffer)
{
	{
		if (!DDGISamplingDescriptorSets.empty())
		{
			vulkanContext->LogicalDevice.freeDescriptorSets(descriptorpool, DDGISamplingDescriptorSets);
			DDGISamplingDescriptorSets.clear();
		}

		// create sets from the pool based on the layout
		// 	     
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, DDGISamplingDescriptorSetLayout);


		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = layouts.data();

		DDGISamplingDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, DDGISamplingDescriptorSets.data());

		////////////////////////////////////////////////////////////////////////////////////////////////
		//specifies what exactly to send
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			vk::DescriptorImageInfo PositionImageInfo{};
			PositionImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			PositionImageInfo.imageView   = gbuffer.Position.imageView;
			PositionImageInfo.sampler     = gbuffer.Position.imageSampler;

			vk::WriteDescriptorSet PositionImageSamplerdescriptorWrite{};
			PositionImageSamplerdescriptorWrite.dstSet = DDGISamplingDescriptorSets[i];
			PositionImageSamplerdescriptorWrite.dstBinding = 0;
			PositionImageSamplerdescriptorWrite.dstArrayElement = 0;
			PositionImageSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			PositionImageSamplerdescriptorWrite.descriptorCount = 1;
			PositionImageSamplerdescriptorWrite.pImageInfo = &PositionImageInfo;


			vk::DescriptorImageInfo NormalImageInfo{};
			NormalImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			NormalImageInfo.imageView = gbuffer.Normal.imageView;
			NormalImageInfo.sampler = gbuffer.Normal.imageSampler;

			vk::WriteDescriptorSet NormalImageSamplerdescriptorWrite{};
			NormalImageSamplerdescriptorWrite.dstSet = DDGISamplingDescriptorSets[i];
			NormalImageSamplerdescriptorWrite.dstBinding = 1;
			NormalImageSamplerdescriptorWrite.dstArrayElement = 0;
			NormalImageSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			NormalImageSamplerdescriptorWrite.descriptorCount = 1;
			NormalImageSamplerdescriptorWrite.pImageInfo = &NormalImageInfo;


			vk::DescriptorImageInfo IradianceImageAtlasImageInfo{};
			IradianceImageAtlasImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			IradianceImageAtlasImageInfo.imageView = IradianceImageAtlasImage.imageView;
			IradianceImageAtlasImageInfo.sampler = IradianceImageAtlasImage.imageSampler;

			vk::WriteDescriptorSet IradianceImageAtlasSamplerdescriptorWrite{};
			IradianceImageAtlasSamplerdescriptorWrite.dstSet = DDGISamplingDescriptorSets[i];
			IradianceImageAtlasSamplerdescriptorWrite.dstBinding = 2;
			IradianceImageAtlasSamplerdescriptorWrite.dstArrayElement = 0;
			IradianceImageAtlasSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			IradianceImageAtlasSamplerdescriptorWrite.descriptorCount = 1;
			IradianceImageAtlasSamplerdescriptorWrite.pImageInfo = &IradianceImageAtlasImageInfo;

			vk::DescriptorImageInfo Probe_Sampled_GIImageInfo{};
			Probe_Sampled_GIImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			Probe_Sampled_GIImageInfo.imageView = Probe_Sampled_GI_Image.imageView;
			Probe_Sampled_GIImageInfo.sampler = Probe_Sampled_GI_Image.imageSampler;

			vk::WriteDescriptorSet Probe_Sampled_GISamplerdescriptorWrite{};
			Probe_Sampled_GISamplerdescriptorWrite.dstSet = DDGISamplingDescriptorSets[i];
			Probe_Sampled_GISamplerdescriptorWrite.dstBinding = 3;
			Probe_Sampled_GISamplerdescriptorWrite.dstArrayElement = 0;
			Probe_Sampled_GISamplerdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			Probe_Sampled_GISamplerdescriptorWrite.descriptorCount = 1;
			Probe_Sampled_GISamplerdescriptorWrite.pImageInfo = &Probe_Sampled_GIImageInfo;

			vk::DescriptorImageInfo VisibilityImageAtlasImageInfo{};
			VisibilityImageAtlasImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			VisibilityImageAtlasImageInfo.imageView = VisibilityImageAtlasImage.imageView;
			VisibilityImageAtlasImageInfo.sampler = VisibilityImageAtlasImage.imageSampler;

			vk::WriteDescriptorSet VisibilityAtlasSamplerdescriptorWrite{};
			VisibilityAtlasSamplerdescriptorWrite.dstSet = DDGISamplingDescriptorSets[i];
			VisibilityAtlasSamplerdescriptorWrite.dstBinding = 4;
			VisibilityAtlasSamplerdescriptorWrite.dstArrayElement = 0;
			VisibilityAtlasSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			VisibilityAtlasSamplerdescriptorWrite.descriptorCount = 1;
			VisibilityAtlasSamplerdescriptorWrite.pImageInfo = &VisibilityImageAtlasImageInfo;


			std::array<vk::WriteDescriptorSet, 5> descriptorWrites{ PositionImageSamplerdescriptorWrite,
																	NormalImageSamplerdescriptorWrite,
				                                                    IradianceImageAtlasSamplerdescriptorWrite,Probe_Sampled_GISamplerdescriptorWrite,
			                                                        VisibilityAtlasSamplerdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}

}


void DynamicDiffuse_RTGI::createRaytracedDescriptorSets(vk::DescriptorPool descriptorpool, vk::AccelerationStructureKHR TLAS, std::vector<BufferData>& fragmentUniformBuffers)
{

	if (!ProbeDescriptorSets.empty())
	{
		vulkanContext->LogicalDevice.freeDescriptorSets(descriptorpool, ProbeDescriptorSets);
		ProbeDescriptorSets.clear();
	}

	if (!GridDescriptorSets.empty())
	{
		vulkanContext->LogicalDevice.freeDescriptorSets(descriptorpool, GridDescriptorSets);
		GridDescriptorSets.clear();
	}

	if (!RaytracingDescriptorSets.empty())
	{
		vulkanContext->LogicalDevice.freeDescriptorSets(descriptorpool, RaytracingDescriptorSets);
		RaytracingDescriptorSets.clear();
	}

	if (!ConstructProbeDataDescriptorSets.empty())
	{
		vulkanContext->LogicalDevice.freeDescriptorSets(descriptorpool, ConstructProbeDataDescriptorSets);
		ConstructProbeDataDescriptorSets.clear();
	}

	if (!ProbeStatusDescriptorSets.empty())
	{
		vulkanContext->LogicalDevice.freeDescriptorSets(descriptorpool, ProbeStatusDescriptorSets);
		ProbeStatusDescriptorSets.clear();
	}

	{
		// create sets from the pool based on the layout
		// 	     
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, ProbeDescriptorSetLayout);


		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = layouts.data();

		ProbeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, ProbeDescriptorSets.data());

		////////////////////////////////////////////////////////////////////////////////////////////////
		//specifies what exactly to send
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			vk::DescriptorImageInfo IrradiancAtlasImageInfo{};
			IrradiancAtlasImageInfo.imageLayout =  vk::ImageLayout::eGeneral;
			IrradiancAtlasImageInfo.imageView   = IradianceImageAtlasImage.imageView;
			IrradiancAtlasImageInfo.sampler     = IradianceImageAtlasImage.imageSampler;
		

			vk::WriteDescriptorSet IrradianceAtlasImageSamplerdescriptorWrite{};
			IrradianceAtlasImageSamplerdescriptorWrite.dstSet          = ProbeDescriptorSets[i];
			IrradianceAtlasImageSamplerdescriptorWrite.dstBinding      = 0;
			IrradianceAtlasImageSamplerdescriptorWrite.dstArrayElement = 0;
			IrradianceAtlasImageSamplerdescriptorWrite.descriptorType  = vk::DescriptorType::eCombinedImageSampler;
			IrradianceAtlasImageSamplerdescriptorWrite.descriptorCount = 1;
			IrradianceAtlasImageSamplerdescriptorWrite.pImageInfo      = &IrradiancAtlasImageInfo;

			vk::DescriptorBufferInfo ProbeLocationbufferInfo{};
			ProbeLocationbufferInfo.buffer = ProbeDataStorageBuffers[0].buffer;
			ProbeLocationbufferInfo.offset = 0;
			ProbeLocationbufferInfo.range = sizeof(ProbeInformation) * 1000;

			vk::WriteDescriptorSet ProbeLocationbufferdescriptorWrite{};
			ProbeLocationbufferdescriptorWrite.dstSet = ProbeDescriptorSets[i];
			ProbeLocationbufferdescriptorWrite.dstBinding = 1;
			ProbeLocationbufferdescriptorWrite.dstArrayElement = 0;
			ProbeLocationbufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
			ProbeLocationbufferdescriptorWrite.descriptorCount = 1;
			ProbeLocationbufferdescriptorWrite.pBufferInfo = &ProbeLocationbufferInfo;


			std::array<vk::WriteDescriptorSet, 2> descriptorWrites{ IrradianceAtlasImageSamplerdescriptorWrite,
																	ProbeLocationbufferdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}


	{
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, GridDescriptorSetLayout);


		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = layouts.data();

		GridDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, GridDescriptorSets.data());

		////////////////////////////////////////////////////////////////////////////////////////////////
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			vk::DescriptorBufferInfo ProbeLocationbufferInfo{};
			ProbeLocationbufferInfo.buffer = ProbeDataStorageBuffers[0].buffer;
			ProbeLocationbufferInfo.offset = 0;
			ProbeLocationbufferInfo.range = sizeof(ProbeInformation) * 2000;

			vk::WriteDescriptorSet ProbeLocationbufferdescriptorWrite{};
			ProbeLocationbufferdescriptorWrite.dstSet = GridDescriptorSets[i];
			ProbeLocationbufferdescriptorWrite.dstBinding = 0;
			ProbeLocationbufferdescriptorWrite.dstArrayElement = 0;
			ProbeLocationbufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
			ProbeLocationbufferdescriptorWrite.descriptorCount = 1;
			ProbeLocationbufferdescriptorWrite.pBufferInfo = &ProbeLocationbufferInfo;

			vk::DescriptorBufferInfo FibonacciDirectionsbufferInfo{};
			FibonacciDirectionsbufferInfo.buffer = ProbeFibonacciDirectionsStorageBuffers[0].buffer;
			FibonacciDirectionsbufferInfo.offset = 0;
			FibonacciDirectionsbufferInfo.range = sizeof(glm::vec4) * 300;

			vk::WriteDescriptorSet FibonacciDirectionsbufferdescriptorWrite{};
			FibonacciDirectionsbufferdescriptorWrite.dstSet = GridDescriptorSets[i];
			FibonacciDirectionsbufferdescriptorWrite.dstBinding = 1;
			FibonacciDirectionsbufferdescriptorWrite.dstArrayElement = 0;
			FibonacciDirectionsbufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
			FibonacciDirectionsbufferdescriptorWrite.descriptorCount = 1;
			FibonacciDirectionsbufferdescriptorWrite.pBufferInfo = &FibonacciDirectionsbufferInfo;


			std::array<vk::WriteDescriptorSet, 2> descriptorWrites{ProbeLocationbufferdescriptorWrite,FibonacciDirectionsbufferdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}


	{
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, ConstructProbeDataDescriptorSetLayout);


		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = layouts.data();

		ConstructProbeDataDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, ConstructProbeDataDescriptorSets.data());

		////////////////////////////////////////////////////////////////////////////////////////////////
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			vk::DescriptorBufferInfo FibonacciDirectionsbufferInfo{};
			FibonacciDirectionsbufferInfo.buffer = ProbeFibonacciDirectionsStorageBuffers[0].buffer;
			FibonacciDirectionsbufferInfo.offset = 0;
			FibonacciDirectionsbufferInfo.range = sizeof(glm::vec4) * 300;

			vk::WriteDescriptorSet FibonacciDirectionsbufferdescriptorWrite{};
			FibonacciDirectionsbufferdescriptorWrite.dstSet = ConstructProbeDataDescriptorSets[i];
			FibonacciDirectionsbufferdescriptorWrite.dstBinding = 0;
			FibonacciDirectionsbufferdescriptorWrite.dstArrayElement = 0;
			FibonacciDirectionsbufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
			FibonacciDirectionsbufferdescriptorWrite.descriptorCount = 1;
			FibonacciDirectionsbufferdescriptorWrite.pBufferInfo = &FibonacciDirectionsbufferInfo;

			vk::DescriptorImageInfo radiancAtlasImageInfo{};
			radiancAtlasImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			radiancAtlasImageInfo.imageView = RadianceImageAtlasImage.imageView;
			radiancAtlasImageInfo.sampler = RadianceImageAtlasImage.imageSampler;

			vk::WriteDescriptorSet radianceAtlasImageSamplerdescriptorWrite{};
			radianceAtlasImageSamplerdescriptorWrite.dstSet = ConstructProbeDataDescriptorSets[i];
			radianceAtlasImageSamplerdescriptorWrite.dstBinding = 1;
			radianceAtlasImageSamplerdescriptorWrite.dstArrayElement = 0;
			radianceAtlasImageSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			radianceAtlasImageSamplerdescriptorWrite.descriptorCount = 1;
			radianceAtlasImageSamplerdescriptorWrite.pImageInfo = &radiancAtlasImageInfo;

			vk::DescriptorImageInfo IradiancAtlasImageInfo{};
			IradiancAtlasImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			IradiancAtlasImageInfo.imageView = IradianceImageAtlasImage.imageView;
			IradiancAtlasImageInfo.sampler = IradianceImageAtlasImage.imageSampler;

			vk::WriteDescriptorSet IradianceAtlasImageSamplerdescriptorWrite{};
			IradianceAtlasImageSamplerdescriptorWrite.dstSet = ConstructProbeDataDescriptorSets[i];
			IradianceAtlasImageSamplerdescriptorWrite.dstBinding = 2;
			IradianceAtlasImageSamplerdescriptorWrite.dstArrayElement = 0;
			IradianceAtlasImageSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			IradianceAtlasImageSamplerdescriptorWrite.descriptorCount = 1;
			IradianceAtlasImageSamplerdescriptorWrite.pImageInfo = &IradiancAtlasImageInfo;

			vk::DescriptorImageInfo VisibilityAtlasImageInfo{};
			VisibilityAtlasImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			VisibilityAtlasImageInfo.imageView   = VisibilityImageAtlasImage.imageView;
			VisibilityAtlasImageInfo.sampler     = VisibilityImageAtlasImage.imageSampler;

			vk::WriteDescriptorSet VisibilityAtlasImageSamplerdescriptorWrite{};
			VisibilityAtlasImageSamplerdescriptorWrite.dstSet = ConstructProbeDataDescriptorSets[i];
			VisibilityAtlasImageSamplerdescriptorWrite.dstBinding = 3;
			VisibilityAtlasImageSamplerdescriptorWrite.dstArrayElement = 0;
			VisibilityAtlasImageSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			VisibilityAtlasImageSamplerdescriptorWrite.descriptorCount = 1;
			VisibilityAtlasImageSamplerdescriptorWrite.pImageInfo = &VisibilityAtlasImageInfo;

			vk::DescriptorImageInfo Prev_IradiancAtlasImageInfo{};
			Prev_IradiancAtlasImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			Prev_IradiancAtlasImageInfo.imageView = Prev_IradianceImageAtlasImage.imageView;
			Prev_IradiancAtlasImageInfo.sampler = Prev_IradianceImageAtlasImage.imageSampler;

			vk::WriteDescriptorSet Prev_IradianceAtlasImageSamplerdescriptorWrite{};
			Prev_IradianceAtlasImageSamplerdescriptorWrite.dstSet = ConstructProbeDataDescriptorSets[i];
			Prev_IradianceAtlasImageSamplerdescriptorWrite.dstBinding = 4;
			Prev_IradianceAtlasImageSamplerdescriptorWrite.dstArrayElement = 0;
			Prev_IradianceAtlasImageSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			Prev_IradianceAtlasImageSamplerdescriptorWrite.descriptorCount = 1;
			Prev_IradianceAtlasImageSamplerdescriptorWrite.pImageInfo = &Prev_IradiancAtlasImageInfo;

			vk::DescriptorImageInfo Prev_VisibilityAtlasImageInfo{};
			Prev_VisibilityAtlasImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			Prev_VisibilityAtlasImageInfo.imageView   = Prev_VisibilityImageAtlasImage.imageView;
			Prev_VisibilityAtlasImageInfo.sampler     = Prev_VisibilityImageAtlasImage.imageSampler;

			vk::WriteDescriptorSet Prev_VisibilityAtlasImageSamplerdescriptorWrite{};
			Prev_VisibilityAtlasImageSamplerdescriptorWrite.dstSet = ConstructProbeDataDescriptorSets[i];
			Prev_VisibilityAtlasImageSamplerdescriptorWrite.dstBinding = 5;
			Prev_VisibilityAtlasImageSamplerdescriptorWrite.dstArrayElement = 0;
			Prev_VisibilityAtlasImageSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			Prev_VisibilityAtlasImageSamplerdescriptorWrite.descriptorCount = 1;
			Prev_VisibilityAtlasImageSamplerdescriptorWrite.pImageInfo = &Prev_VisibilityAtlasImageInfo;

			vk::DescriptorBufferInfo ProbeLocationbufferInfo{};
			ProbeLocationbufferInfo.buffer = ProbeDataStorageBuffers[0].buffer;
			ProbeLocationbufferInfo.offset = 0;
			ProbeLocationbufferInfo.range = sizeof(ProbeInformation) * 2000;

			vk::WriteDescriptorSet ProbeLocationbufferdescriptorWrite{};
			ProbeLocationbufferdescriptorWrite.dstSet = ConstructProbeDataDescriptorSets[i];
			ProbeLocationbufferdescriptorWrite.dstBinding = 6;
			ProbeLocationbufferdescriptorWrite.dstArrayElement = 0;
			ProbeLocationbufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
			ProbeLocationbufferdescriptorWrite.descriptorCount = 1;
			ProbeLocationbufferdescriptorWrite.pBufferInfo = &ProbeLocationbufferInfo;


			std::array<vk::WriteDescriptorSet, 7> descriptorWrites{ FibonacciDirectionsbufferdescriptorWrite,
				                                                    radianceAtlasImageSamplerdescriptorWrite,
			                                                        IradianceAtlasImageSamplerdescriptorWrite,
				                                                    VisibilityAtlasImageSamplerdescriptorWrite,
				                                                    Prev_IradianceAtlasImageSamplerdescriptorWrite,
				                                                    Prev_VisibilityAtlasImageSamplerdescriptorWrite,
				                                                    ProbeLocationbufferdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}

	{
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, ProbeStatusDescriptorSetLayout);


		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = layouts.data();

		ProbeStatusDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, ProbeStatusDescriptorSets.data());

		////////////////////////////////////////////////////////////////////////////////////////////////
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {


			vk::DescriptorBufferInfo ProbeLocationbufferInfo{};
			ProbeLocationbufferInfo.buffer = ProbeDataStorageBuffers[0].buffer;
			ProbeLocationbufferInfo.offset = 0;
			ProbeLocationbufferInfo.range = sizeof(ProbeInformation) * 1000;

			vk::WriteDescriptorSet ProbeLocationbufferdescriptorWrite{};
			ProbeLocationbufferdescriptorWrite.dstSet = ProbeStatusDescriptorSets[i];
			ProbeLocationbufferdescriptorWrite.dstBinding = 0;
			ProbeLocationbufferdescriptorWrite.dstArrayElement = 0;
			ProbeLocationbufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
			ProbeLocationbufferdescriptorWrite.descriptorCount = 1;
			ProbeLocationbufferdescriptorWrite.pBufferInfo = &ProbeLocationbufferInfo;

			vk::DescriptorImageInfo IradiancAtlasImageInfo{};
			IradiancAtlasImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			IradiancAtlasImageInfo.imageView = IradianceImageAtlasImage.imageView;
			IradiancAtlasImageInfo.sampler = IradianceImageAtlasImage.imageSampler;

			vk::WriteDescriptorSet IradianceAtlasImageSamplerdescriptorWrite{};
			IradianceAtlasImageSamplerdescriptorWrite.dstSet = ProbeStatusDescriptorSets[i];
			IradianceAtlasImageSamplerdescriptorWrite.dstBinding = 1;
			IradianceAtlasImageSamplerdescriptorWrite.dstArrayElement = 0;
			IradianceAtlasImageSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			IradianceAtlasImageSamplerdescriptorWrite.descriptorCount = 1;
			IradianceAtlasImageSamplerdescriptorWrite.pImageInfo = &IradiancAtlasImageInfo;

			vk::DescriptorImageInfo VisibilityAtlasImageInfo{};
			VisibilityAtlasImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			VisibilityAtlasImageInfo.imageView = VisibilityImageAtlasImage.imageView;
			VisibilityAtlasImageInfo.sampler = VisibilityImageAtlasImage.imageSampler;

			vk::WriteDescriptorSet VisibilityAtlasImageSamplerdescriptorWrite{};
			VisibilityAtlasImageSamplerdescriptorWrite.dstSet = ProbeStatusDescriptorSets[i];
			VisibilityAtlasImageSamplerdescriptorWrite.dstBinding = 2;
			VisibilityAtlasImageSamplerdescriptorWrite.dstArrayElement = 0;
			VisibilityAtlasImageSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			VisibilityAtlasImageSamplerdescriptorWrite.descriptorCount = 1;
			VisibilityAtlasImageSamplerdescriptorWrite.pImageInfo = &VisibilityAtlasImageInfo;

			vk::DescriptorImageInfo RadianceVisibilityAtlasImageInfo{};
			RadianceVisibilityAtlasImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			RadianceVisibilityAtlasImageInfo.imageView = RadianceImageAtlasImage.imageView;
			RadianceVisibilityAtlasImageInfo.sampler   = RadianceImageAtlasImage.imageSampler;

			vk::WriteDescriptorSet RadianceVisibilityAtlasImageSamplerdescriptorWrite{};
			RadianceVisibilityAtlasImageSamplerdescriptorWrite.dstSet = ProbeStatusDescriptorSets[i];
			RadianceVisibilityAtlasImageSamplerdescriptorWrite.dstBinding = 3;
			RadianceVisibilityAtlasImageSamplerdescriptorWrite.dstArrayElement = 0;
			RadianceVisibilityAtlasImageSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			RadianceVisibilityAtlasImageSamplerdescriptorWrite.descriptorCount = 1;
			RadianceVisibilityAtlasImageSamplerdescriptorWrite.pImageInfo = &RadianceVisibilityAtlasImageInfo;


			std::array<vk::WriteDescriptorSet, 4> descriptorWrites{ ProbeLocationbufferdescriptorWrite,
				                                                    IradianceAtlasImageSamplerdescriptorWrite,
				                                                    VisibilityAtlasImageSamplerdescriptorWrite,RadianceVisibilityAtlasImageSamplerdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}


	{
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, RaytracingDescriptorSetLayout);


		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = layouts.data();

		RaytracingDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vk::Result result = vulkanContext->LogicalDevice.allocateDescriptorSets(
			&allocinfo,
			RaytracingDescriptorSets.data()
		);

		if (result != vk::Result::eSuccess) {
			std::cerr << "Failed to allocate Raytracing descriptor sets: "
				<< vk::to_string(result) << std::endl;

			throw std::runtime_error("Failed to allocate Raytracing descriptor sets!");
		}

		////////////////////////////////////////////////////////////////////////////////////////////////
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			vk::WriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
			descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
			descriptorAccelerationStructureInfo.pAccelerationStructures = &TLAS;

			vk::WriteDescriptorSet TLAS_descriptorWrite{};
			TLAS_descriptorWrite.dstSet = RaytracingDescriptorSets[i];
			TLAS_descriptorWrite.dstBinding = 0;
			TLAS_descriptorWrite.dstArrayElement = 0;
			TLAS_descriptorWrite.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
			TLAS_descriptorWrite.descriptorCount = 1;
			TLAS_descriptorWrite.pNext = &descriptorAccelerationStructureInfo;

			/////////////////////////////////////////////////////////////////////////////////////


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
			AssetImagSamplerdescriptorWrite.dstBinding = 1;
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
			NormalAssetImagSamplerdescriptorWrite.dstBinding = 2;
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
			MetalicRoughnessAssetImagSamplerdescriptorWrite.dstBinding = 3;
			MetalicRoughnessAssetImagSamplerdescriptorWrite.dstArrayElement = 0;
			MetalicRoughnessAssetImagSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			MetalicRoughnessAssetImagSamplerdescriptorWrite.descriptorCount = MetalicRoughnessImageAssetImagesInfos.size();
			MetalicRoughnessAssetImagSamplerdescriptorWrite.pImageInfo = MetalicRoughnessImageAssetImagesInfos.data();

			vk::DescriptorBufferInfo IndexStorageBuffersInfo{};
			IndexStorageBuffersInfo.buffer = bufferManager->AllScene_IndexStorageBuffers[0].buffer;
			IndexStorageBuffersInfo.offset = 0;
			IndexStorageBuffersInfo.range = sizeof(uint32_t) * bufferManager->AllScene_IndexGeometryData.size();;

			vk::WriteDescriptorSet IndexStorageBufferdescriptorWrite{};
			IndexStorageBufferdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			IndexStorageBufferdescriptorWrite.dstBinding = 4;
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
			VertexStorageBufferdescriptorWrite.dstBinding = 5;
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
			OffsetStorageBufferdescriptorWrite.dstBinding = 6;
			OffsetStorageBufferdescriptorWrite.dstArrayElement = 0;
			OffsetStorageBufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
			OffsetStorageBufferdescriptorWrite.descriptorCount = 1;
			OffsetStorageBufferdescriptorWrite.pBufferInfo = &OffsetStorageBuffersInfo;

			vk::DescriptorBufferInfo TransformUniformBuffersInfo{};
			TransformUniformBuffersInfo.buffer = bufferManager->AllScene_TransformationUniformBuffers[i].buffer;
			TransformUniformBuffersInfo.offset = 0;
			TransformUniformBuffersInfo.range = sizeof(glm::mat4) * 100;

			vk::WriteDescriptorSet TransformUniformBufferdescriptorWrite{};
			TransformUniformBufferdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			TransformUniformBufferdescriptorWrite.dstBinding = 7;
			TransformUniformBufferdescriptorWrite.dstArrayElement = 0;
			TransformUniformBufferdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
			TransformUniformBufferdescriptorWrite.descriptorCount = 1;
			TransformUniformBufferdescriptorWrite.pBufferInfo = &TransformUniformBuffersInfo;

			vk::DescriptorImageInfo IrradianceAtlasStorageImageInfo{};
			IrradianceAtlasStorageImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			IrradianceAtlasStorageImageInfo.imageView = RadianceImageAtlasImage.imageView;
			IrradianceAtlasStorageImageInfo.sampler = RadianceImageAtlasImage.imageSampler;

			vk::WriteDescriptorSet IrradianceAtlasdescriptorWrite{};
			IrradianceAtlasdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			IrradianceAtlasdescriptorWrite.dstBinding = 8;
			IrradianceAtlasdescriptorWrite.dstArrayElement = 0;
			IrradianceAtlasdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			IrradianceAtlasdescriptorWrite.descriptorCount = 1;
			IrradianceAtlasdescriptorWrite.pImageInfo = &IrradianceAtlasStorageImageInfo;

			vk::DescriptorBufferInfo ProbeLocationbufferInfo{};
			ProbeLocationbufferInfo.buffer = ProbeDataStorageBuffers[0].buffer;
			ProbeLocationbufferInfo.offset = 0;
			ProbeLocationbufferInfo.range = sizeof(ProbeInformation) * 1000;

			vk::WriteDescriptorSet ProbeLocationbufferdescriptorWrite{};
			ProbeLocationbufferdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			ProbeLocationbufferdescriptorWrite.dstBinding = 9;
			ProbeLocationbufferdescriptorWrite.dstArrayElement = 0;
			ProbeLocationbufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
			ProbeLocationbufferdescriptorWrite.descriptorCount = 1;
			ProbeLocationbufferdescriptorWrite.pBufferInfo = &ProbeLocationbufferInfo;

			vk::DescriptorBufferInfo FibonacciDirectionsbufferInfo{};
			FibonacciDirectionsbufferInfo.buffer = ProbeFibonacciDirectionsStorageBuffers[0].buffer;
			FibonacciDirectionsbufferInfo.offset = 0;
			FibonacciDirectionsbufferInfo.range = sizeof(glm::vec4) * 300;

			vk::WriteDescriptorSet FibonacciDirectionsbufferdescriptorWrite{};
			FibonacciDirectionsbufferdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			FibonacciDirectionsbufferdescriptorWrite.dstBinding = 10;
			FibonacciDirectionsbufferdescriptorWrite.dstArrayElement = 0;
			FibonacciDirectionsbufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
			FibonacciDirectionsbufferdescriptorWrite.descriptorCount = 1;
			FibonacciDirectionsbufferdescriptorWrite.pBufferInfo = &FibonacciDirectionsbufferInfo;

			vk::DescriptorBufferInfo LightUniformBuffersInfo{};
			LightUniformBuffersInfo.buffer = fragmentUniformBuffers[i].buffer;
			LightUniformBuffersInfo.offset = 0;
			LightUniformBuffersInfo.range = sizeof(LightUniformData) * 100;

			vk::WriteDescriptorSet lightUniformBufferdescriptorWrite{};
			lightUniformBufferdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			lightUniformBufferdescriptorWrite.dstBinding = 11;
			lightUniformBufferdescriptorWrite.dstArrayElement = 0;
			lightUniformBufferdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
			lightUniformBufferdescriptorWrite.descriptorCount = 1;
			lightUniformBufferdescriptorWrite.pBufferInfo = &LightUniformBuffersInfo;

			std::vector<vk::DescriptorImageInfo> SkyboxImageAssetImagesInfos;

			for (int j = 0; j < skyboxRef->SkyBoxImages.size(); j++)
			{
				ImageData imageData = skyboxRef->SkyBoxImages[j];

			
			    vk::DescriptorImageInfo SkyboxASSETImageInfo{};
			    SkyboxASSETImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			    SkyboxASSETImageInfo.imageView   = imageData.imageView;
			    SkyboxASSETImageInfo.sampler     = imageData.imageSampler;
			    
				SkyboxImageAssetImagesInfos.push_back(SkyboxASSETImageInfo);
			}

			vk::WriteDescriptorSet SkyboxImagSamplerdescriptorWrite{};
			SkyboxImagSamplerdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			SkyboxImagSamplerdescriptorWrite.dstBinding = 12;
			SkyboxImagSamplerdescriptorWrite.dstArrayElement = 0;
			SkyboxImagSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			SkyboxImagSamplerdescriptorWrite.descriptorCount = SkyboxImageAssetImagesInfos.size();
			SkyboxImagSamplerdescriptorWrite.pImageInfo     = SkyboxImageAssetImagesInfos.data();

			vk::DescriptorImageInfo IradianceImageInfo{};
			IradianceImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			IradianceImageInfo.imageView   = IradianceImageAtlasImage.imageView;
			IradianceImageInfo.sampler     = nullptr;
										   
			vk::WriteDescriptorSet IrradianceImagStoragedescriptorWrite{};
			IrradianceImagStoragedescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			IrradianceImagStoragedescriptorWrite.dstBinding = 13;
			IrradianceImagStoragedescriptorWrite.dstArrayElement = 0;
			IrradianceImagStoragedescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			IrradianceImagStoragedescriptorWrite.descriptorCount = 1;
			IrradianceImagStoragedescriptorWrite.pImageInfo = &IradianceImageInfo;
			
			vk::DescriptorImageInfo VisibilityImageInfo{};
			VisibilityImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			VisibilityImageInfo.imageView = VisibilityImageAtlasImage.imageView;
			VisibilityImageInfo.sampler = nullptr;

			vk::WriteDescriptorSet VisibilityImagStoragedescriptorWrite{};
			VisibilityImagStoragedescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			VisibilityImagStoragedescriptorWrite.dstBinding = 14;
			VisibilityImagStoragedescriptorWrite.dstArrayElement = 0;
			VisibilityImagStoragedescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			VisibilityImagStoragedescriptorWrite.descriptorCount = 1;
			VisibilityImagStoragedescriptorWrite.pImageInfo = &VisibilityImageInfo;


			std::vector<vk::DescriptorImageInfo> EmmisiveImageAssetImagesInfos;

			for (int j = 0; j < bufferManager->AllScene_Emissive_Images.size(); j++)
			{
				ImageData* imageData = bufferManager->AllScene_Emissive_Images[j];

				if (imageData) {

					vk::DescriptorImageInfo EmmisiveASSETImageInfo{};
					EmmisiveASSETImageInfo.imageLayout = vk::ImageLayout::eGeneral;
					EmmisiveASSETImageInfo.imageView   = imageData->imageView;
					EmmisiveASSETImageInfo.sampler     = imageData->imageSampler;

					EmmisiveImageAssetImagesInfos.push_back(EmmisiveASSETImageInfo);
				};
			}

			vk::WriteDescriptorSet EmmisiveAssetImagSamplerdescriptorWrite{};
			EmmisiveAssetImagSamplerdescriptorWrite.dstSet = RaytracingDescriptorSets[i];
			EmmisiveAssetImagSamplerdescriptorWrite.dstBinding = 15;
			EmmisiveAssetImagSamplerdescriptorWrite.dstArrayElement = 0;
			EmmisiveAssetImagSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			EmmisiveAssetImagSamplerdescriptorWrite.descriptorCount = EmmisiveImageAssetImagesInfos.size();
			EmmisiveAssetImagSamplerdescriptorWrite.pImageInfo = EmmisiveImageAssetImagesInfos.data();


			std::array<vk::WriteDescriptorSet, 16> descriptorWrites{ 
			TLAS_descriptorWrite,AssetImagSamplerdescriptorWrite,
			NormalAssetImagSamplerdescriptorWrite,MetalicRoughnessAssetImagSamplerdescriptorWrite,
			IndexStorageBufferdescriptorWrite,VertexStorageBufferdescriptorWrite,OffsetStorageBufferdescriptorWrite,
			TransformUniformBufferdescriptorWrite,IrradianceAtlasdescriptorWrite,
			ProbeLocationbufferdescriptorWrite,FibonacciDirectionsbufferdescriptorWrite,
			lightUniformBufferdescriptorWrite,SkyboxImagSamplerdescriptorWrite,
			IrradianceImagStoragedescriptorWrite,VisibilityImagStoragedescriptorWrite,EmmisiveAssetImagSamplerdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}
}



uint32_t DynamicDiffuse_RTGI::alignedSize(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}


void DynamicDiffuse_RTGI::Draw(BufferData RayGenBuffer, BufferData RayHitBuffer, BufferData RayMisBuffer, vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
{
      vk::BufferDeviceAddressInfo raygenShaderBindingTableDeviceAdressesInfo;
      raygenShaderBindingTableDeviceAdressesInfo.buffer = RayGenBuffer.buffer;
      
      vk::BufferDeviceAddressInfo missShaderBindingTableDeviceAdressesInfo;
      missShaderBindingTableDeviceAdressesInfo.buffer = RayMisBuffer.buffer;
      
      vk::BufferDeviceAddressInfo hitShaderBindingTableDeviceAdressesInfo;
      hitShaderBindingTableDeviceAdressesInfo.buffer = RayHitBuffer.buffer;
      
      auto raygenShaderBindingTableAdress = vulkanContext->LogicalDevice.getBufferAddress(raygenShaderBindingTableDeviceAdressesInfo);
      auto missShaderBindingTableAdress   = vulkanContext->LogicalDevice.getBufferAddress(missShaderBindingTableDeviceAdressesInfo);
      auto hitShaderBindingTableAdress    = vulkanContext->LogicalDevice.getBufferAddress(hitShaderBindingTableDeviceAdressesInfo);
      
      
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
      
	  uint32_t totalProbes  = NumOfProbesX * NumOfProbesY * NumOfProbesZ;
      int depth  = 1;

	  RTpcInfo rtpcInfo;
	  rtpcInfo.sampleGridInfo.GridBaseLocation_ScreenSizeWidth = glm::vec4(GridLocation.x, GridLocation.y, GridLocation.z, vulkanContext->swapchainExtent.width);
	  rtpcInfo.sampleGridInfo.ProbeSpacing_ScreenSizeHeight = glm::vec4(ProbeOffset.x, ProbeOffset.y, ProbeOffset.z, vulkanContext->swapchainExtent.height);
	  rtpcInfo.sampleGridInfo.ProbeCount = glm::vec4(NumOfProbesX, NumOfProbesY, NumOfProbesZ, skyboxRef->SkyBoxIndex);
	  rtpcInfo.sampleGridInfo.generalAtlasInfo.AtlasWidthSize = IradianceImageExtent.width;
	  rtpcInfo.sampleGridInfo.generalAtlasInfo.ProbeSideLength = ProbeSideLength;
	  rtpcInfo.sampleGridInfo.generalAtlasInfo.GutterSize = GutterSize;
	  rtpcInfo.sampleGridInfo.generalAtlasInfo.RaysPerProbe = RaysPerProbe;
	  rtpcInfo.UseInfiniteBounce_infinite_bounces_multiplier_Padding = glm::vec4(UseinfiniteBounce, infiniteBounceMultiplyer, SampleCount, LightCount);

	  commandbuffer.pushConstants(pipelinelayout, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR, 0, sizeof(RTpcInfo), &rtpcInfo);
      commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, pipelinelayout, 0, 1, &RaytracingDescriptorSets[imageIndex],0,nullptr);
      
      vulkanContext->vkCmdTraceRaysKHR(
      	commandbuffer,
      	&TEMP_raygenShaderSbtEntry,
      	&TEMP_missShaderSbtEntry,
      	&TEMP_hitShaderSbtEntry,
      	&TEMP_callableShaderSbtEntry,
		RaysPerProbe,
		totalProbes,
      	depth);


	  FrameCount++;
}

bool DynamicDiffuse_RTGI::UpdateUniformBuffer(vk::DescriptorPool descriptorpool, vk::AccelerationStructureKHR TLAS, std::vector<BufferData>& fragmentUniformBuffers,GBuffer gbuffer,bool ForceUpdate,int lightcount)
{
	if (Last_NumOfProbesX != NumOfProbesX || Last_NumOfProbesY != NumOfProbesY || Last_NumOfProbesZ != NumOfProbesZ
		|| Last_ProbeOffset != ProbeOffset || Last_GridLocation != GridLocation || Last_RaysPerProbe != RaysPerProbe ||ForceUpdate)
	{
		Last_NumOfProbesX = NumOfProbesX;
		Last_NumOfProbesY = NumOfProbesY;
		Last_NumOfProbesZ = NumOfProbesZ;
		Last_ProbeOffset  = ProbeOffset;
		Last_GridLocation = GridLocation;
		Last_RaysPerProbe = RaysPerProbe;

		vulkanContext->LogicalDevice.waitIdle();

		DestroyAtlasImages();

		CreateAtlasImages();

		createRaytracedDescriptorSets(descriptorpool, TLAS, fragmentUniformBuffers);
		createDescriptorSets(descriptorpool, gbuffer);
		UpdateGrid = 1;
		return true;
	}

	LightCount = lightcount;

	return false;
}

void DynamicDiffuse_RTGI::DrawNode(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex, const std::vector<std::shared_ptr<Node>>& nodes)
{
	for (const auto& node : nodes) {

		if (!node) continue;


		for (int i = 0; i < node->meshPrimitives.size(); i++)
		{

			if (node->meshPrimitives[i].numIndices > 0) {

				CameraConstantBuffer cameraConstantBuffer;
				cameraConstantBuffer.ViewMatrix = camera->GetViewMatrix();
				cameraConstantBuffer.ProjectionMatrix = camera->GetProjectionMatrix();
				cameraConstantBuffer.ProjectionMatrix[1][1] *= -1;
				cameraConstantBuffer.ModelMatrix = glm::mat4(1);
				cameraConstantBuffer.ModelMatrix = glm::scale(glm::vec3(0.5, 0.5, 0.5));
				cameraConstantBuffer.generalAtlasInfo.AtlasWidthSize = IradianceImageExtent.width;
				cameraConstantBuffer.generalAtlasInfo.ProbeSideLength = ProbeSideLength;
				cameraConstantBuffer.generalAtlasInfo.GutterSize = GutterSize;
				cameraConstantBuffer.generalAtlasInfo.RaysPerProbe = RaysPerProbe;
				cameraConstantBuffer.ShowDebugStatus = ShowDEBUG_Status;

				commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(CameraConstantBuffer), &cameraConstantBuffer);
						
				commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &ProbeDescriptorSets[imageIndex], 0, nullptr);

				commandBuffer.drawIndexed(node->meshPrimitives[i].numIndices, NumOfProbesX * NumOfProbesY * NumOfProbesZ, node->meshPrimitives[i].indicesStart, 0, 0);
			}
		}



		DrawNode(commandBuffer, pipelineLayout, imageIndex, node->children);
	}
}

void DynamicDiffuse_RTGI::DispatchGridCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex)
{
		uint32_t numElements = NumOfProbesX * NumOfProbesY * NumOfProbesZ;
		uint32_t localSizeX = 32;
		uint32_t workGroupsX = (numElements + localSizeX - 1) / localSizeX;

		GridData gridData;
		gridData.probeCount = glm::vec4(NumOfProbesX, NumOfProbesY, NumOfProbesZ, RaysPerProbe);
		gridData.probeOffset = glm::vec4(ProbeOffset, UpdateGrid);
		gridData.probeBaseLocation = glm::vec4(GridLocation, 0);

		commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(GridData), &gridData);

		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, 1, &GridDescriptorSets[imageIndex], 0, nullptr);
		commandBuffer.dispatch(workGroupsX, 1, 1);

		UpdateGrid = 0;
}

void DynamicDiffuse_RTGI::DispatchDirectionsCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex, float deltaTime)
{
	if (RayRotationRadians >= 360.0f) {
		RayRotationRadians  = 0.0f;
	}


	uint32_t numElements = RaysPerProbe;
	uint32_t localSizeX = 32;
	uint32_t workGroupsX = (numElements + localSizeX - 1) / localSizeX;

	GridData gridData;
	gridData.probeCount = glm::vec4(NumOfProbesX, NumOfProbesY, NumOfProbesZ, RaysPerProbe);
	gridData.probeOffset = glm::vec4(ProbeOffset, 1);
	gridData.probeBaseLocation = glm::vec4(GridLocation, 1);

	gridData.RotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(RayRotationRadians), glm::vec3(1.0f, 1.0f, 1.0f));
	RayRotationRadians += 0.1 * deltaTime;



	commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(GridData), &gridData);

	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, 1, &GridDescriptorSets[imageIndex], 0, nullptr);
	commandBuffer.dispatch(workGroupsX, 1, 1);
}

void DynamicDiffuse_RTGI::DispatchCalcProbeDataCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex)
{

	GeneralAtlasInfo generalAtlasInfo;
	generalAtlasInfo.AtlasWidthSize = IradianceImageExtent.width;
	generalAtlasInfo.ProbeSideLength = ProbeSideLength;
	generalAtlasInfo.GutterSize = GutterSize;
	generalAtlasInfo.RaysPerProbe = RaysPerProbe;

	commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(GeneralAtlasInfo), &generalAtlasInfo);

	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, 1, &ConstructProbeDataDescriptorSets[imageIndex], 0, nullptr);

	uint32_t workGroupsX = (IradianceImageExtent.width  +  7) / 8;
	uint32_t workGroupsY = (IradianceImageExtent.height +  7) / 8;

	commandBuffer.dispatch(workGroupsX, workGroupsY, 1);
}

void DynamicDiffuse_RTGI::DispatchProbeStatus(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex)
{

	GeneralAtlasInfo generalAtlasInfo;
	generalAtlasInfo.AtlasWidthSize = IradianceImageExtent.width;
	generalAtlasInfo.ProbeSideLength = ProbeSideLength;
	generalAtlasInfo.GutterSize = GutterSize;
	generalAtlasInfo.RaysPerProbe = RaysPerProbe;

	commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(GeneralAtlasInfo), &generalAtlasInfo);

	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, 1, &ProbeStatusDescriptorSets[imageIndex], 0, nullptr);

	uint32_t workGroupsX = (IradianceImageExtent.width  + 7) / 8;
	uint32_t workGroupsY = (IradianceImageExtent.height + 7) / 8;
	commandBuffer.dispatch(workGroupsX, workGroupsY, 1);
}


void DynamicDiffuse_RTGI::DispatchSampleGIFromProbeDataCompute(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex)
{

	SampleGridInfo sampleGridInfo;
	sampleGridInfo.GridBaseLocation_ScreenSizeWidth = glm::vec4(GridLocation.x, GridLocation.y, GridLocation.z, vulkanContext->swapchainExtent.width);
	sampleGridInfo.ProbeSpacing_ScreenSizeHeight    = glm::vec4( ProbeOffset.x, ProbeOffset.y , ProbeOffset.z , vulkanContext->swapchainExtent.height);
	sampleGridInfo.ProbeCount                       = glm::vec4( NumOfProbesX , NumOfProbesY  , NumOfProbesZ  , 0);
	sampleGridInfo.generalAtlasInfo.AtlasWidthSize  = IradianceImageExtent.width;
	sampleGridInfo.generalAtlasInfo.ProbeSideLength = ProbeSideLength;
	sampleGridInfo.generalAtlasInfo.GutterSize      = GutterSize;
	sampleGridInfo.generalAtlasInfo.RaysPerProbe    = RaysPerProbe;

	commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(SampleGridInfo), &sampleGridInfo);

	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, 1, &DDGISamplingDescriptorSets[imageIndex], 0, nullptr);

	

	uint32_t workGroupsX = (vulkanContext->swapchainExtent.width  + 31) / 32;
	uint32_t workGroupsY = (vulkanContext->swapchainExtent.height + 31) / 32;

	commandBuffer.dispatch(workGroupsX, workGroupsY, 1);
}


void DynamicDiffuse_RTGI::Draw(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer vertexBuffers[] = { vertexBufferData.buffer };

	commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
	commandBuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint32);

	if (DrawDEBUG_Probes)
	{
		DrawNode(commandBuffer, pipelineLayout, imageIndex, storedModelData->nodes);
	}
}



void DynamicDiffuse_RTGI::CleanUp()
{

	for (auto& buffer : ProbeDataStorageBuffers)
	{
		if (buffer.buffer)
		{
			//bufferManager->UnmapMemory(buffer);
			bufferManager->DestroyBuffer(buffer);
		}
	}

	for (auto& buffer : ProbeFibonacciDirectionsStorageBuffers)
	{
		if (buffer.buffer)
		{
			//bufferManager->UnmapMemory(buffer);
			bufferManager->DestroyBuffer(buffer);
		}
	}


	ProbeDataStorageBuffers.clear();

	if (vertexBufferData.buffer)
	{
		bufferManager->DestroyBuffer(vertexBufferData);
	}

	if (indexBufferData.buffer)
	{
		bufferManager->DestroyBuffer(indexBufferData);
	}

	vulkanContext->LogicalDevice.destroyDescriptorSetLayout(ProbeDescriptorSetLayout);
	vulkanContext->LogicalDevice.destroyDescriptorSetLayout(GridDescriptorSetLayout);
	vulkanContext->LogicalDevice.destroyDescriptorSetLayout(RaytracingDescriptorSetLayout);
	vulkanContext->LogicalDevice.destroyDescriptorSetLayout(ConstructProbeDataDescriptorSetLayout);
	vulkanContext->LogicalDevice.destroyDescriptorSetLayout(DDGISamplingDescriptorSetLayout);
	vulkanContext->LogicalDevice.destroyDescriptorSetLayout(ProbeStatusDescriptorSetLayout);


	DestroyAtlasImages();	
	ProbeData.clear();
}



