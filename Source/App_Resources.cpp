#include "App.h"
#include "Window.h"
#include "BufferManager.h"
#include "VulkanContext.h"
#include "Camera.h"
#include "Light.h"
#include "Model.h"
#include "SkyBox.h"
#include "SSAO_FullScreenQuad.h"
#include "FXAA_FullScreenQuad.h"
#include "SSGI.h"
#include "CombinedResult_FullScreenQuad.h"
#include "Lighting_RTX.h"
#include "DynamicDiffuse_RTGI.h"
#include "ReSTIR_DI.h"
#include "NvdiaDLSS_Intergration.h"
#include <iostream>
#include <array>

void App::createSyncObjects() {
	// Present complete semaphores - one per frame in flight
	presentCompleteSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

	// Render complete semaphores - one per frame in flight
	renderCompleteSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

	// Fences - one per frame in flight
	waitFences.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::SemaphoreCreateInfo semaphoreInfo{};
		vulkanContext.LogicalDevice.createSemaphore(&semaphoreInfo, nullptr, &presentCompleteSemaphores[i]);
		vulkanContext.LogicalDevice.createSemaphore(&semaphoreInfo, nullptr, &renderCompleteSemaphores[i]);

		vk::FenceCreateInfo fenceInfo{};
		fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
		vulkanContext.LogicalDevice.createFence(&fenceInfo, nullptr, &waitFences[i]);
	}
}

void App::DestroySyncObjects()
{
	for (auto& presentSemaphores : presentCompleteSemaphores)
	{
		vulkanContext.LogicalDevice.destroySemaphore(presentSemaphores);
	}

	for (auto& renderSemaphores : renderCompleteSemaphores)
	{
		vulkanContext.LogicalDevice.destroySemaphore(renderSemaphores);
	}

	for (auto& Fences : waitFences)
	{
		vulkanContext.LogicalDevice.destroyFence(Fences);
	}
}

void App::createCommandPool()
{ 
	vk::CommandPoolCreateInfo poolInfo{};
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	poolInfo.queueFamilyIndex = vulkanContext.graphicsQueueFamilyIndex;

	commandPool = vulkanContext.LogicalDevice.createCommandPool(poolInfo);

	if (!commandPool)
	{
		throw std::runtime_error("failed to create command pool!");
	}
}

void App::createCommandBuffer()
{
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	vk::CommandBufferAllocateInfo allocateInfo{};
	allocateInfo.commandPool = commandPool;
	allocateInfo.level = vk::CommandBufferLevel::ePrimary;
	allocateInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	commandBuffers = vulkanContext.LogicalDevice.allocateCommandBuffers(allocateInfo);

	if (commandBuffers.empty())
	{
		throw std::runtime_error("failed to create command Buffer!");
	}
}

void App::createDescriptorPool()
{
	vk::DescriptorPoolSize Uniformpoolsize;
	Uniformpoolsize.type = vk::DescriptorType::eUniformBuffer;
	Uniformpoolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2000;

	vk::DescriptorPoolSize Samplerpoolsize;
	Samplerpoolsize.type = vk::DescriptorType::eCombinedImageSampler;
	Samplerpoolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2000;

	vk::DescriptorPoolSize AccelerationStructurepoolsize;
	AccelerationStructurepoolsize.type = vk::DescriptorType::eAccelerationStructureKHR;
	AccelerationStructurepoolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 16;

	vk::DescriptorPoolSize StorageImagepoolsize;
	StorageImagepoolsize.type = vk::DescriptorType::eStorageImage;
	StorageImagepoolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 64;

	vk::DescriptorPoolSize StorageBufferpoolsize;
	StorageBufferpoolsize.type = vk::DescriptorType::eStorageBuffer;
	StorageBufferpoolsize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 64;

	std::array<vk::DescriptorPoolSize, 5> poolSizes{ Uniformpoolsize, Samplerpoolsize,
													  AccelerationStructurepoolsize, StorageImagepoolsize, StorageBufferpoolsize };

	vk::DescriptorPoolCreateInfo poolInfo{};
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2000;

	DescriptorPool = vulkanContext.LogicalDevice.createDescriptorPool(poolInfo, nullptr);

	if (skyBox) {
		skyBox->createDescriptorSets(DescriptorPool);
	}
}

void App::createDepthTextureImage()
{
	vk::Extent3D swapchainextent = vk::Extent3D(vulkanContext.swapchainExtent.width, vulkanContext.swapchainExtent.height, 1);

	DepthTextureData.ImageID = "Depth Texture";
	bufferManger.CreateImage(&DepthTextureData, swapchainextent, vulkanContext.FindCompatableDepthFormat(), vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled);
	DepthTextureData.imageView = bufferManger.CreateImageView(&DepthTextureData, vulkanContext.FindCompatableDepthFormat(), vk::ImageAspectFlagBits::eDepth);
	DepthTextureData.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	vk::CommandBuffer commandBuffer = bufferManger.CreateSingleUseCommandBuffer(commandPool);

	ImageTransitionData DataToTransitionInfo;
	DataToTransitionInfo.oldlayout = vk::ImageLayout::eUndefined;
	DataToTransitionInfo.newlayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	DataToTransitionInfo.AspectFlag = vk::ImageAspectFlagBits::eDepth;
	DataToTransitionInfo.SourceAccessflag = vk::AccessFlagBits::eNone;
	DataToTransitionInfo.DestinationAccessflag = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	DataToTransitionInfo.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	DataToTransitionInfo.DestinationOnThePipeline = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;

	bufferManger.TransitionImage(commandBuffer, &DepthTextureData, DataToTransitionInfo);
	bufferManger.SubmitAndDestoyCommandBuffer(commandPool, commandBuffer, vulkanContext.graphicsQueue);
}

void App::destroy_DepthImage()
{
	bufferManger.DestroyImage(DepthTextureData);
}

void App::createGBuffer()
{
	vulkanContext.ResetFrameCount();

	vk::Extent3D swapchainextent = vk::Extent3D(vulkanContext.swapchainExtent.width, vulkanContext.swapchainExtent.height, 1);

	gbuffer.Position.ImageID = "Gbuffer Position Texture";
	bufferManger.CreateImage(&gbuffer.Position, swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.Position.imageView = bufferManger.CreateImageView(&gbuffer.Position, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	gbuffer.Position.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	gbuffer.Normal.ImageID = "Gbuffer WorldSpaceNormal Texture";
	bufferManger.CreateImage(&gbuffer.Normal, swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc);
	gbuffer.Normal.imageView = bufferManger.CreateImageView(&gbuffer.Normal, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	gbuffer.Normal.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);
	
	gbuffer.PrevNormal.ImageID = "Gbuffer prev WorldSpaceNormal Texture";
	bufferManger.CreateImage(&gbuffer.PrevNormal, swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
	gbuffer.PrevNormal.imageView = bufferManger.CreateImageView(&gbuffer.PrevNormal, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	gbuffer.PrevNormal.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	gbuffer.ViewSpaceNormal.ImageID = "Gbuffer ViewSpaceNormal Texture";
	bufferManger.CreateImage(&gbuffer.ViewSpaceNormal, swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.ViewSpaceNormal.imageView = bufferManger.CreateImageView(&gbuffer.ViewSpaceNormal, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	gbuffer.ViewSpaceNormal.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	gbuffer.Materials.ImageID = "Gbuffer Materials Texture";
	bufferManger.CreateImage(&gbuffer.Materials, swapchainextent, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.Materials.imageView = bufferManger.CreateImageView(&gbuffer.Materials, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
	gbuffer.Materials.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	gbuffer.Albedo.ImageID = "Gbuffer Albedo Texture";
	bufferManger.CreateImage(&gbuffer.Albedo, swapchainextent, vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.Albedo.imageView = bufferManger.CreateImageView(&gbuffer.Albedo, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
	gbuffer.Albedo.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	gbuffer.Emissive.ImageID = "Gbuffer Emissive Texture";
	bufferManger.CreateImage(&gbuffer.Emissive, swapchainextent, vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.Emissive.imageView = bufferManger.CreateImageView(&gbuffer.Emissive, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
	gbuffer.Emissive.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	gbuffer.MotionVector.ImageID = "MotionVectors Texture";
	bufferManger.CreateImage(&gbuffer.MotionVector, swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.MotionVector.imageView = bufferManger.CreateImageView(&gbuffer.MotionVector, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	gbuffer.MotionVector.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	gbuffer.SpecularAlbedo.ImageID = "SpecularAlbedo Texture";
	bufferManger.CreateImage(&gbuffer.SpecularAlbedo, swapchainextent, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	gbuffer.SpecularAlbedo.imageView = bufferManger.CreateImageView(&gbuffer.SpecularAlbedo, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
	gbuffer.SpecularAlbedo.imageSampler = bufferManger.CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

	fxaa_FullScreenQuad->CreateImage(swapchainextent);
	SSGI_FullScreenQuad->CreateGIImage();
	Combined_FullScreenQuad->CreateImage(swapchainextent);
	ssao_FullScreenQuad->CreateImage();
	dynamicDiffuse_RTGI->CreateSampledGIImage(); 
	Restir_DI->CreateImage();
	lighting_RTX->CreateStorageImage();

	lighting_RTX->createDescriptorSetsBasedOnGBuffer(DescriptorPool, &gbuffer, &TLAS);
	ssao_FullScreenQuad->createDescriptorSetsBasedOnGBuffer(DescriptorPool, gbuffer);
	Combined_FullScreenQuad->createDescriptorSetsBasedOnGBuffer(DescriptorPool, lighting_RTX->ResultingStorageImage, SSGI_FullScreenQuad->SSGIPassImage, ssao_FullScreenQuad->BluredSSAOImage, gbuffer.Materials, gbuffer.Albedo, dynamicDiffuse_RTGI->Probe_Sampled_GI_Image, lighting_RTX->PTGI_StorageImage, lighting_RTX->Prev_Frame_PTGI_StorageImage, gbuffer.MotionVector);
	fxaa_FullScreenQuad->createDescriptorSets(DescriptorPool, Combined_FullScreenQuad->Combined_Lighting_Image);
	SSGI_FullScreenQuad->createDescriptorSets(DescriptorPool, gbuffer, lighting_RTX->ResultingStorageImage, DepthTextureData);
	dynamicDiffuse_RTGI->createDescriptorSets(DescriptorPool, gbuffer);
	Restir_DI->createDescriptorSetsBasedOnGBuffer(DescriptorPool, &TLAS);

	vk::CommandBuffer cmd = bufferManger.CreateSingleUseCommandBuffer(commandPool);
	ImageTransitionData TransitionToGeneral{};
	TransitionToGeneral.oldlayout = vk::ImageLayout::eUndefined;
	TransitionToGeneral.newlayout = vk::ImageLayout::eGeneral;
	TransitionToGeneral.AspectFlag = vk::ImageAspectFlagBits::eColor;
	TransitionToGeneral.SourceAccessflag = vk::AccessFlagBits::eNone;
	TransitionToGeneral.DestinationAccessflag = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eShaderRead;
	TransitionToGeneral.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
	TransitionToGeneral.DestinationOnThePipeline = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eFragmentShader;

	vulkanContext.ResetFrameCount();

	bufferManger.TransitionImage(cmd, &gbuffer.Position, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &gbuffer.Normal, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &gbuffer.ViewSpaceNormal, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &gbuffer.Albedo, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &gbuffer.Emissive, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &gbuffer.Materials, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &gbuffer.MotionVector, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &Combined_FullScreenQuad->Combined_Lighting_Image, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &Combined_FullScreenQuad->Final_Denoised_Image, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &SSGI_FullScreenQuad->SSGIPassImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &ssao_FullScreenQuad->SSAOImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &ssao_FullScreenQuad->IntermediateBlurImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &ssao_FullScreenQuad->BluredSSAOImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &fxaa_FullScreenQuad->FxaaImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &dynamicDiffuse_RTGI->Probe_Sampled_GI_Image, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &Restir_DI->PrevResevoirImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &Restir_DI->ReSTIRDI_Results, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &Restir_DI->ReSTIRDI_Denoised_Results, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &lighting_RTX->ResultingStorageImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &lighting_RTX->PTGI_StorageImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &lighting_RTX->Prev_Frame_PTGI_StorageImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &Restir_DI->GI_SamplePosImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &Restir_DI->GI_SampleFluxImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &Restir_DI->PrevGI_SamplePosImage, TransitionToGeneral);
	bufferManger.TransitionImage(cmd, &Restir_DI->PrevGI_SampleFluxImage, TransitionToGeneral);

	bufferManger.SubmitAndDestoyCommandBuffer(commandPool, cmd, vulkanContext.graphicsQueue);

	UpdateTextureID();
}

void App::destroy_GbufferImages()
{
	bufferManger.DestroyImage(gbuffer.Position);
	bufferManger.DestroyImage(gbuffer.Normal);
	bufferManger.DestroyImage(gbuffer.ViewSpaceNormal);
	bufferManger.DestroyImage(gbuffer.Materials);
	bufferManger.DestroyImage(gbuffer.Albedo);
	bufferManger.DestroyImage(gbuffer.Emissive);
	bufferManger.DestroyImage(gbuffer.MotionVector);
	bufferManger.DestroyImage(gbuffer.PrevNormal);
	bufferManger.DestroyImage(gbuffer.SpecularAlbedo);

	ssao_FullScreenQuad->DestroyImage();
	fxaa_FullScreenQuad->DestroyImage();
	SSGI_FullScreenQuad->DestroyImage();
	Combined_FullScreenQuad->DestroyImage();
	dynamicDiffuse_RTGI->DestroySampledGIImage();
	Restir_DI->DestroyImage();
	lighting_RTX->DestroyStorageImage();
}

void App::DestroyBuffers()
{
	destroy_DepthImage();
	destroy_GbufferImages();

	for (auto& model : SponzaSceneModels)
	{
		model.reset();
	}

	for (auto& model : CornelSceneModels)
	{
		model.reset();
	}

	for (auto& model : AltCornelSceneModels)
	{
		model.reset();
	}
	
	for (auto& model : Alt_2_CornelSceneModels)
	{
		model.reset();
	}

	for (auto& light : lights)
	{
		light.reset();
	}

	skyBox.reset();

	lighting_RTX.reset();
	ssao_FullScreenQuad.reset();
	fxaa_FullScreenQuad.reset();
	SSGI_FullScreenQuad.reset();
	Combined_FullScreenQuad.reset();
	Restir_DI.reset();
	dynamicDiffuse_RTGI.reset();
	DestroyTLAS();
	bufferManger.DestroySharedBuffers();
	DestroyShaderBindingTable();
}

void App::recreateSwapChain() {
	int width = 0, height = 0;
	glfwGetFramebufferSize(window.GetWindow(), &width, &height);

	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window.GetWindow(), &width, &height);
		glfwWaitEvents();
	}

	vulkanContext.LogicalDevice.waitForFences(
		static_cast<uint32_t>(waitFences.size()), waitFences.data(),
		vk::True, UINT64_MAX);
	vulkanContext.LogicalDevice.waitIdle();

	vulkanContext.destroy_swapchain();
	destroy_DepthImage();
	destroy_GbufferImages();

	vulkanContext.create_swapchain();

	camera.SetSwapchainHeight(vulkanContext.swapchainExtent.height);
	camera.SetSwapchainWidth(vulkanContext.swapchainExtent.width);
	createDepthTextureImage();
	createGBuffer();
	vulkanContext.DLSS_IntergrationRef->init(commandPool);
}

void App::SwapchainResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}

void App::UpdateTextureID()
{
	if (bUseDLSS)
	{
		FinalRenderTextureId = ImGui_ImplVulkan_AddTexture(Combined_FullScreenQuad->Final_Denoised_Image.imageSampler,
			Combined_FullScreenQuad->Final_Denoised_Image.imageView,
			VK_IMAGE_LAYOUT_GENERAL);
	}
	else
	{
		FinalRenderTextureId = ImGui_ImplVulkan_AddTexture(Combined_FullScreenQuad->Combined_Lighting_Image.imageSampler,
			Combined_FullScreenQuad->Combined_Lighting_Image.imageView,
			VK_IMAGE_LAYOUT_GENERAL);
	}

	if (bUseDLSS)
	{
		ReSTIR_DITextureId = ImGui_ImplVulkan_AddTexture(Restir_DI->ReSTIRDI_Denoised_Results.imageSampler,
			Restir_DI->ReSTIRDI_Denoised_Results.imageView,
			VK_IMAGE_LAYOUT_GENERAL);
	}
	else
	{
		ReSTIR_DITextureId = ImGui_ImplVulkan_AddTexture(Restir_DI->ReSTIRDI_Results.imageSampler,
			Restir_DI->ReSTIRDI_Results.imageView,
			VK_IMAGE_LAYOUT_GENERAL);
	}

	SSGITextureId = ImGui_ImplVulkan_AddTexture(SSGI_FullScreenQuad->SSGIPassImage.imageSampler,
		SSGI_FullScreenQuad->SSGIPassImage.imageView,
		VK_IMAGE_LAYOUT_GENERAL);

	DDGI_Radiance = ImGui_ImplVulkan_AddTexture(dynamicDiffuse_RTGI->RadianceImageAtlasImage.imageSampler,
		dynamicDiffuse_RTGI->RadianceImageAtlasImage.imageView,
		VK_IMAGE_LAYOUT_GENERAL);

	Sampled_GI_ID = ImGui_ImplVulkan_AddTexture(dynamicDiffuse_RTGI->Probe_Sampled_GI_Image.imageSampler,
		dynamicDiffuse_RTGI->Probe_Sampled_GI_Image.imageView,
		VK_IMAGE_LAYOUT_GENERAL);
}
