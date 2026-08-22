#include "App.h"
#include "VulkanContext.h"
#include "BufferManager.h"
#include "Camera.h"
#include "Model.h"
#include "Light.h"
#include "SkyBox.h"
#include "UserInterface.h"
#include "Lighting_RTX.h"
#include "DynamicDiffuse_RTGI.h"
#include "ReSTIR_DI.h"
#include "SSAO_FullScreenQuad.h"
#include "SSGI.h"
#include "CombinedResult_FullScreenQuad.h"
#include "FXAA_FullScreenQuad.h"
#include "NvdiaDLSS_Intergration.h"
#include "Tracy.hpp"
#include <array>
#include <vector>

void App::updateUniformBuffer(uint32_t currentImage) {
	UpdateTLAS();

	bufferManger.Update_Raytracing_Data(currentImage, Models);
	for (auto& light : lights)
	{
		light->UpdateUniformBuffer(currentImage);
	}

	for (auto& model : Models)
	{
		model->UpdateUniformBuffer(currentImage);
	}

	skyBox->UpdateUniformBuffer(currentImage);

	lighting_RTX->UpdateUniformBuffer(currentImage, lights);
	ssao_FullScreenQuad->UpdataeUniformBufferData();
	SSGI_FullScreenQuad->UpdateUniformBuffer(currentImage, deltaTime);
	Combined_FullScreenQuad->UpdataeUniformBufferData();

	bool ddgiRecreated = dynamicDiffuse_RTGI->UpdateUniformBuffer(DescriptorPool, TLAS, lighting_RTX->UniformBuffers, gbuffer, false, lights.size());

	if (ddgiRecreated)
	{
		DDGIIrradianceAtlasID = ImGui_ImplVulkan_AddTexture(
			dynamicDiffuse_RTGI->IradianceImageAtlasImage.imageSampler,
			dynamicDiffuse_RTGI->IradianceImageAtlasImage.imageView,
			VK_IMAGE_LAYOUT_GENERAL
		);

		DDGIIVisibilityAtlasID = ImGui_ImplVulkan_AddTexture(
			dynamicDiffuse_RTGI->VisibilityImageAtlasImage.imageSampler,
			dynamicDiffuse_RTGI->VisibilityImageAtlasImage.imageView,
			VK_IMAGE_LAYOUT_GENERAL
		);

		Restir_DI->createDescriptorDDGIATLAS(DescriptorPool);
	}
}

void App::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) {
	commandBuffer.reset();

	vk::CommandBufferBeginInfo begininfo{};
	begininfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	commandBuffer.begin(begininfo);
#if ENABLE_NVPERF
	// Only access m_apiTracers when NVPerf successfully initialized
	const bool nvperfActive = userinterface.m_nvperfReady &&
							  (currentFrame < userinterface.m_apiTracers.size());
	nv::perf::mini_trace::APITracerVulkan* apiTracerPtr =
		nvperfActive ? &userinterface.m_apiTracers[currentFrame] : nullptr;

	struct ScopedApiTracer {
		nv::perf::mini_trace::APITracerVulkan* ptr;
		void ClearData()  { if (ptr) ptr->ClearData(); }
		void ResetQueries(vk::CommandBuffer cb) { if (ptr) ptr->ResetQueries(cb); }
		void BeginRange(vk::CommandBuffer cb, const char* name, int n, size_t& idx)
					   { if (ptr) ptr->BeginRange(cb, name, n, idx); }
		void EndRange(vk::CommandBuffer cb, size_t& idx)
					 { if (ptr) ptr->EndRange(cb, idx); }
	} apiTracer{ apiTracerPtr };

	apiTracer.ClearData();
	apiTracer.ResetQueries(commandBuffer);
#endif

	size_t passIndex = 0;

	// Initial Image Transitions
	{
		ImageTransitionData ResetDepth{};
		ResetDepth.oldlayout = vk::ImageLayout::eUndefined;
		ResetDepth.newlayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		ResetDepth.AspectFlag = vk::ImageAspectFlagBits::eDepth;
		ResetDepth.SourceAccessflag = vk::AccessFlagBits::eNone;
		ResetDepth.DestinationAccessflag = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		ResetDepth.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
		ResetDepth.DestinationOnThePipeline = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		bufferManger.TransitionImage(commandBuffer, &DepthTextureData, ResetDepth);

		ImageTransitionData ResetDenoised{};
		ResetDenoised.oldlayout = vk::ImageLayout::eUndefined;
		ResetDenoised.newlayout = vk::ImageLayout::eGeneral;
		ResetDenoised.AspectFlag = vk::ImageAspectFlagBits::eColor;
		ResetDenoised.SourceAccessflag = vk::AccessFlagBits::eNone;
		ResetDenoised.DestinationAccessflag = vk::AccessFlagBits::eShaderWrite;
		ResetDenoised.SourceOnThePipeline = vk::PipelineStageFlagBits::eTopOfPipe;
		ResetDenoised.DestinationOnThePipeline = vk::PipelineStageFlagBits::eComputeShader | vk::PipelineStageFlagBits::eRayTracingShaderKHR;
		bufferManger.TransitionImage(commandBuffer, &Combined_FullScreenQuad->Final_Denoised_Image, ResetDenoised);
	}

	vk::ClearValue clearColor{};
	clearColor.color = { 0.0f, 0.0f, 0.0f, 0.0f };

	VkOffset2D imageoffset = { 0, 0 };

	vk::Viewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)vulkanContext.swapchainExtent.width;
	viewport.height = (float)vulkanContext.swapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor{};
	scissor.offset = imageoffset;
	scissor.extent.width = vulkanContext.swapchainExtent.width;
	scissor.extent.height = vulkanContext.swapchainExtent.height;

	// ==========================================
	// 1. G-BUFFER RASTERIZATION PASS
	// ==========================================
	vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, Gbuffer_Label);
	{
		ImageTransitionData TransitiontoGeneraRT{};
		TransitiontoGeneraRT.oldlayout = vk::ImageLayout::eUndefined;
		TransitiontoGeneraRT.newlayout = vk::ImageLayout::eGeneral;
		TransitiontoGeneraRT.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitiontoGeneraRT.SourceAccessflag = vk::AccessFlagBits::eNone;
		TransitiontoGeneraRT.DestinationAccessflag = vk::AccessFlagBits::eShaderWrite;
		TransitiontoGeneraRT.SourceOnThePipeline = vk::PipelineStageFlagBits::eNone;
		TransitiontoGeneraRT.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

		bufferManger.TransitionImage(commandBuffer, &gbuffer.Normal, TransitiontoGeneraRT);
		bufferManger.TransitionImage(commandBuffer, &gbuffer.PrevNormal, TransitiontoGeneraRT);

		vk::ImageSubresourceLayers SrcSubresourceLayers;
		SrcSubresourceLayers.mipLevel = 0;
		SrcSubresourceLayers.baseArrayLayer = 0;
		SrcSubresourceLayers.layerCount = 1;
		SrcSubresourceLayers.aspectMask = vk::ImageAspectFlagBits::eColor;

		vk::Extent3D ImageSize = {
			vulkanContext.swapchainExtent.width,
			vulkanContext.swapchainExtent.height,
			1
		};

		bufferManger.CopyImageToAnotherImage(commandBuffer,
			gbuffer.Normal, vk::ImageLayout::eGeneral, SrcSubresourceLayers,
			gbuffer.PrevNormal, vk::ImageLayout::eGeneral, SrcSubresourceLayers,
			ImageSize, vulkanContext.graphicsQueue);

		std::array<ImageData*, 8> gbufferAttachments = {
			&gbuffer.Position,
			&gbuffer.Normal,
			&gbuffer.ViewSpaceNormal,
			&gbuffer.Albedo,
			&gbuffer.Materials,
			&gbuffer.Emissive,
			&gbuffer.MotionVector,
			&gbuffer.SpecularAlbedo
		};

		std::vector<vk::ImageMemoryBarrier> toColorAttachmentBarriers;
		toColorAttachmentBarriers.reserve(gbufferAttachments.size());
		for (auto* img : gbufferAttachments) {
			vk::ImageMemoryBarrier b{};
			b.srcAccessMask = vk::AccessFlagBits::eNone;
			b.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
			b.oldLayout = vk::ImageLayout::eUndefined;
			b.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
			b.image = img->image;
			b.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
			toColorAttachmentBarriers.push_back(b);
		}

		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			{},
			0, nullptr,
			0, nullptr,
			static_cast<uint32_t>(toColorAttachmentBarriers.size()), toColorAttachmentBarriers.data()
		);

		vk::RenderingAttachmentInfo PositioncolorAttachmentInfo{};
		PositioncolorAttachmentInfo.imageView = gbuffer.Position.imageView;
		PositioncolorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		PositioncolorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		PositioncolorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		PositioncolorAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfo NormalcolorAttachmentInfo{};
		NormalcolorAttachmentInfo.imageView = gbuffer.Normal.imageView;
		NormalcolorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		NormalcolorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		NormalcolorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		NormalcolorAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfo ViewSpaceNormalcolorAttachmentInfo{};
		ViewSpaceNormalcolorAttachmentInfo.imageView = gbuffer.ViewSpaceNormal.imageView;
		ViewSpaceNormalcolorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		ViewSpaceNormalcolorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		ViewSpaceNormalcolorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		ViewSpaceNormalcolorAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfo AlbedocolorAttachmentInfo{};
		AlbedocolorAttachmentInfo.imageView = gbuffer.Albedo.imageView;
		AlbedocolorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		AlbedocolorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		AlbedocolorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		AlbedocolorAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfo EmmisivAttachmentInfo{};
		EmmisivAttachmentInfo.imageView = gbuffer.Emissive.imageView;
		EmmisivAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		EmmisivAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		EmmisivAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		EmmisivAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfo MaterialscolorAttachmentInfo{};
		MaterialscolorAttachmentInfo.imageView = gbuffer.Materials.imageView;
		MaterialscolorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		MaterialscolorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		MaterialscolorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		MaterialscolorAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfo MotionVectorcolorAttachmentInfo{};
		MotionVectorcolorAttachmentInfo.imageView = gbuffer.MotionVector.imageView;
		MotionVectorcolorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		MotionVectorcolorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		MotionVectorcolorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		MotionVectorcolorAttachmentInfo.clearValue = clearColor;

		vk::RenderingAttachmentInfo SpecularcolorAttachmentInfo{};
		SpecularcolorAttachmentInfo.imageView = gbuffer.SpecularAlbedo.imageView;
		SpecularcolorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		SpecularcolorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		SpecularcolorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		SpecularcolorAttachmentInfo.clearValue = clearColor;

		std::array<vk::RenderingAttachmentInfo, 8> ColorAttachments{
			PositioncolorAttachmentInfo, NormalcolorAttachmentInfo, ViewSpaceNormalcolorAttachmentInfo,
			AlbedocolorAttachmentInfo, EmmisivAttachmentInfo, MaterialscolorAttachmentInfo,
			MotionVectorcolorAttachmentInfo, SpecularcolorAttachmentInfo
		};

		vk::RenderingAttachmentInfo depthStencilAttachment;
		depthStencilAttachment.imageView = DepthTextureData.imageView;
		depthStencilAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		depthStencilAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		depthStencilAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		depthStencilAttachment.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

		vk::RenderingInfo renderingInfo{};
		renderingInfo.renderArea.offset = imageoffset;
		renderingInfo.renderArea.extent.height = vulkanContext.swapchainExtent.height;
		renderingInfo.renderArea.extent.width = vulkanContext.swapchainExtent.width;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = static_cast<uint32_t>(ColorAttachments.size());
		renderingInfo.pColorAttachments = ColorAttachments.data();
		renderingInfo.pDepthAttachment = &depthStencilAttachment;

		commandBuffer.beginRendering(renderingInfo);
		commandBuffer.setViewport(0, 1, &viewport);
		commandBuffer.setScissor(0, 1, &scissor);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, geometryPassPipeline);

		if (bWireFrame)
		{
			vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_LINE);
		}
		else
		{
			vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_FILL);
		}

		for (auto& model : Models)
		{
			model->Draw(commandBuffer, geometryPassPipelineLayout, currentFrame);
		}

		commandBuffer.endRendering();

		// Transition G-Buffer to eGeneral for downstream RT, Compute, SSAO
		std::vector<vk::ImageMemoryBarrier> toGeneralBarriers;
		toGeneralBarriers.reserve(gbufferAttachments.size());
		for (auto* img : gbufferAttachments) {
			vk::ImageMemoryBarrier b{};
			b.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
			b.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			b.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
			b.newLayout = vk::ImageLayout::eGeneral;
			b.image = img->image;
			b.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
			toGeneralBarriers.push_back(b);
		}

		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eComputeShader | vk::PipelineStageFlagBits::eRayTracingShaderKHR,
			{},
			0, nullptr,
			0, nullptr,
			static_cast<uint32_t>(toGeneralBarriers.size()), toGeneralBarriers.data()
		);
	}
	vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);

	vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_FILL);

	// ==========================================
	// 2. SCREEN-SPACE AMBIENT OCCLUSION (SSAO) PASS
	// ==========================================
	vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, SSAO_Label);
	{
		ImageTransitionData GBufferDepthToSample{};
		GBufferDepthToSample.oldlayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		GBufferDepthToSample.newlayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		GBufferDepthToSample.AspectFlag = vk::ImageAspectFlagBits::eDepth;
		GBufferDepthToSample.SourceAccessflag = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		GBufferDepthToSample.DestinationAccessflag = vk::AccessFlagBits::eShaderRead;
		GBufferDepthToSample.SourceOnThePipeline = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
		GBufferDepthToSample.DestinationOnThePipeline = vk::PipelineStageFlagBits::eFragmentShader;

		bufferManger.TransitionImage(commandBuffer, &DepthTextureData, GBufferDepthToSample);

		vk::RenderingAttachmentInfo SSAOColorAttachment{};
		SSAOColorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		SSAOColorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		SSAOColorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		SSAOColorAttachment.imageView = ssao_FullScreenQuad->SSAOImage.imageView;
		SSAOColorAttachment.clearValue = clearColor;

		vk::RenderingInfo renderingInfo{};
		renderingInfo.renderArea.offset = imageoffset;
		renderingInfo.renderArea.extent.height = ssao_FullScreenQuad->SSAOImageSize.height;
		renderingInfo.renderArea.extent.width = ssao_FullScreenQuad->SSAOImageSize.width;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &SSAOColorAttachment;

		vk::Viewport SSAOviewport{};
		SSAOviewport.x = 0.0f;
		SSAOviewport.y = 0.0f;
		SSAOviewport.width = static_cast<float>(ssao_FullScreenQuad->SSAOImageSize.width);
		SSAOviewport.height = static_cast<float>(ssao_FullScreenQuad->SSAOImageSize.height);
		SSAOviewport.minDepth = 0.0f;
		SSAOviewport.maxDepth = 1.0f;

		vk::Rect2D SSAOscissor{};
		SSAOscissor.offset = imageoffset;
		SSAOscissor.extent.width = ssao_FullScreenQuad->SSAOImageSize.width;
		SSAOscissor.extent.height = ssao_FullScreenQuad->SSAOImageSize.height;

		commandBuffer.beginRendering(renderingInfo);
		commandBuffer.setViewport(0, 1, &SSAOviewport);
		commandBuffer.setScissor(0, 1, &SSAOscissor);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, SSAOPipeline);

		ssao_FullScreenQuad->Draw(commandBuffer, SSAOPipelineLayout, currentFrame);
		commandBuffer.endRendering();

		vk::ImageMemoryBarrier barrier{};
		barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
		barrier.newLayout = vk::ImageLayout::eGeneral;
		barrier.image = ssao_FullScreenQuad->SSAOImage.image;
		barrier.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader, {}, 0, nullptr, 0, nullptr, 1, &barrier);

		// Horizontal SSAO Blur
		vk::RenderingAttachmentInfo SSAOIntermediateColorAttachment{};
		SSAOIntermediateColorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		SSAOIntermediateColorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		SSAOIntermediateColorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		SSAOIntermediateColorAttachment.imageView = ssao_FullScreenQuad->IntermediateBlurImage.imageView;
		SSAOIntermediateColorAttachment.clearValue = clearColor;

		vk::RenderingInfo blurIntermediateInfo{};
		blurIntermediateInfo.renderArea.offset = imageoffset;
		blurIntermediateInfo.renderArea.extent.height = ssao_FullScreenQuad->BluredSSAOImageSize.height;
		blurIntermediateInfo.renderArea.extent.width = ssao_FullScreenQuad->BluredSSAOImageSize.width;
		blurIntermediateInfo.layerCount = 1;
		blurIntermediateInfo.colorAttachmentCount = 1;
		blurIntermediateInfo.pColorAttachments = &SSAOIntermediateColorAttachment;

		vk::Viewport blurViewport{};
		blurViewport.x = 0.0f;
		blurViewport.y = 0.0f;
		blurViewport.width = static_cast<float>(ssao_FullScreenQuad->BluredSSAOImageSize.width);
		blurViewport.height = static_cast<float>(ssao_FullScreenQuad->BluredSSAOImageSize.height);
		blurViewport.minDepth = 0.0f;
		blurViewport.maxDepth = 1.0f;

		vk::Rect2D blurScissor{};
		blurScissor.offset = imageoffset;
		blurScissor.extent.width = ssao_FullScreenQuad->BluredSSAOImageSize.width;
		blurScissor.extent.height = ssao_FullScreenQuad->BluredSSAOImageSize.height;

		commandBuffer.beginRendering(blurIntermediateInfo);
		commandBuffer.setViewport(0, 1, &blurViewport);
		commandBuffer.setScissor(0, 1, &blurScissor);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, SSAOBlurPipeline);

		ssao_FullScreenQuad->DrawSSAOBlurHorizontal(commandBuffer, SSAOBlurPipelineLayout, currentFrame);
		commandBuffer.endRendering();

		vk::ImageMemoryBarrier intermediateBarrier{};
		intermediateBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		intermediateBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		intermediateBarrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
		intermediateBarrier.newLayout = vk::ImageLayout::eGeneral;
		intermediateBarrier.image = ssao_FullScreenQuad->IntermediateBlurImage.image;
		intermediateBarrier.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader, {}, 0, nullptr, 0, nullptr, 1, &intermediateBarrier);

		// Vertical SSAO Blur
		vk::RenderingAttachmentInfo SSAOBluredColorAttachment{};
		SSAOBluredColorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		SSAOBluredColorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		SSAOBluredColorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		SSAOBluredColorAttachment.imageView = ssao_FullScreenQuad->BluredSSAOImage.imageView;
		SSAOBluredColorAttachment.clearValue = clearColor;

		vk::RenderingInfo blurFinalInfo{};
		blurFinalInfo.renderArea.offset = imageoffset;
		blurFinalInfo.renderArea.extent.height = ssao_FullScreenQuad->BluredSSAOImageSize.height;
		blurFinalInfo.renderArea.extent.width = ssao_FullScreenQuad->BluredSSAOImageSize.width;
		blurFinalInfo.layerCount = 1;
		blurFinalInfo.colorAttachmentCount = 1;
		blurFinalInfo.pColorAttachments = &SSAOBluredColorAttachment;

		commandBuffer.beginRendering(blurFinalInfo);
		commandBuffer.setViewport(0, 1, &blurViewport);
		commandBuffer.setScissor(0, 1, &blurScissor);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, SSAOBlurPipeline);

		ssao_FullScreenQuad->DrawSSAOBlurVertical(commandBuffer, SSAOBlurPipelineLayout, currentFrame);
		commandBuffer.endRendering();

		vk::ImageMemoryBarrier finalBlurBarrier{};
		finalBlurBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		finalBlurBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		finalBlurBarrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
		finalBlurBarrier.newLayout = vk::ImageLayout::eGeneral;
		finalBlurBarrier.image = ssao_FullScreenQuad->BluredSSAOImage.image;
		finalBlurBarrier.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader, {}, 0, nullptr, 0, nullptr, 1, &finalBlurBarrier);
	}
	vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);

	// ==========================================
	// 3. DYNAMIC DIFFUSE GLOBAL ILLUMINATION (DDGI)
	// ==========================================
	if (lighting_RTX->GISolutionIndex == 0) {
#if ENABLE_NVPERF
		apiTracer.BeginRange(commandBuffer, "DDGI Grid and Direction Generation", 2, passIndex);
#endif

		vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, DDGI_Grid_Generation_Label);
		ImageTransitionData TransitiontoGeneralRT{};
		TransitiontoGeneralRT.oldlayout = vk::ImageLayout::eUndefined;
		TransitiontoGeneralRT.newlayout = vk::ImageLayout::eGeneral;
		TransitiontoGeneralRT.AspectFlag = vk::ImageAspectFlagBits::eColor;
		TransitiontoGeneralRT.SourceAccessflag = vk::AccessFlagBits::eNone;
		TransitiontoGeneralRT.DestinationAccessflag = vk::AccessFlagBits::eShaderWrite;
		TransitiontoGeneralRT.SourceOnThePipeline = vk::PipelineStageFlagBits::eNone;
		TransitiontoGeneralRT.DestinationOnThePipeline = vk::PipelineStageFlagBits::eRayTracingShaderKHR;
		bufferManger.TransitionImage(commandBuffer, &dynamicDiffuse_RTGI->RadianceImageAtlasImage, TransitiontoGeneralRT);
		bufferManger.TransitionImage(commandBuffer, &dynamicDiffuse_RTGI->IradianceImageAtlasImage, TransitiontoGeneralRT);
		bufferManger.TransitionImage(commandBuffer, &dynamicDiffuse_RTGI->VisibilityImageAtlasImage, TransitiontoGeneralRT);
		bufferManger.TransitionImage(commandBuffer, &dynamicDiffuse_RTGI->Prev_IradianceImageAtlasImage, TransitiontoGeneralRT);
		bufferManger.TransitionImage(commandBuffer, &dynamicDiffuse_RTGI->Prev_VisibilityImageAtlasImage, TransitiontoGeneralRT);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, GridComputePassPipeline);
		dynamicDiffuse_RTGI->DispatchGridCompute(commandBuffer, GridComputePipelineLayout, currentFrame);
		vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);

		vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, DDGI_Directions_Generation_Label);
		dynamicDiffuse_RTGI->DispatchDirectionsCompute(commandBuffer, GridComputePipelineLayout, currentFrame, deltaTime);
		vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);

		vk::BufferMemoryBarrier barrier{};
		barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		barrier.buffer = dynamicDiffuse_RTGI->ProbeDataStorageBuffers.buffer;
		barrier.offset = 0;
		barrier.size = VK_WHOLE_SIZE;
		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eRayTracingShaderKHR, {}, 0, nullptr, 1, &barrier, 0, nullptr);

		vk::BufferMemoryBarrier barrier2{};
		barrier2.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
		barrier2.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		barrier2.buffer = dynamicDiffuse_RTGI->ProbeFibonacciDirectionsStorageBuffers.buffer;
		barrier2.offset = 0;
		barrier2.size = VK_WHOLE_SIZE;
		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eRayTracingShaderKHR, {}, 0, nullptr, 1, &barrier2, 0, nullptr);

#if ENABLE_NVPERF
		apiTracer.EndRange(commandBuffer, passIndex);
		apiTracer.BeginRange(commandBuffer, "DDGI Ray tracing", 2, passIndex);
#endif

		vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, DDGI_Trace_Ray_Label);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, RT_DDGIPassPipeline);
		dynamicDiffuse_RTGI->Draw(DDGI_raygenShaderBindingTableBuffer, DDGI_hitShaderBindingTableBuffer, DDGI_missShaderBindingTableBuffer, commandBuffer, RT_DDGIPipelineLayout, currentFrame);

		vk::ImageMemoryBarrier rtToComputeBarrier{};
		rtToComputeBarrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
		rtToComputeBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		rtToComputeBarrier.oldLayout = vk::ImageLayout::eGeneral;
		rtToComputeBarrier.newLayout = vk::ImageLayout::eGeneral;
		rtToComputeBarrier.image = dynamicDiffuse_RTGI->RadianceImageAtlasImage.image;
		rtToComputeBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		rtToComputeBarrier.subresourceRange.baseMipLevel = 0;
		rtToComputeBarrier.subresourceRange.levelCount = 1;
		rtToComputeBarrier.subresourceRange.baseArrayLayer = 0;
		rtToComputeBarrier.subresourceRange.layerCount = 1;
		rtToComputeBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		rtToComputeBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eRayTracingShaderKHR, vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr, 0, nullptr, 1, &rtToComputeBarrier);
		vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);

#if ENABLE_NVPERF
		apiTracer.EndRange(commandBuffer, passIndex);
		apiTracer.BeginRange(commandBuffer, "DDGI calculate irradiance", 2, passIndex);
#endif

		vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, DDGI_Calculate_Irradiance_Label);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, IrradianceComputePassPipeline);
		dynamicDiffuse_RTGI->DispatchCalcProbeDataCompute(commandBuffer, IrradianceComputePipelineLayout, currentFrame);

		vk::ImageMemoryBarrier imagebarrier{};
		imagebarrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
		imagebarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		imagebarrier.oldLayout = vk::ImageLayout::eGeneral;
		imagebarrier.newLayout = vk::ImageLayout::eGeneral;
		imagebarrier.image = dynamicDiffuse_RTGI->VisibilityImageAtlasImage.image;
		imagebarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imagebarrier.subresourceRange.baseMipLevel = 0;
		imagebarrier.subresourceRange.levelCount = 1;
		imagebarrier.subresourceRange.baseArrayLayer = 0;
		imagebarrier.subresourceRange.layerCount = 1;
		imagebarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imagebarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eRayTracingShaderKHR, {}, 0, nullptr, 0, nullptr, 1, &imagebarrier);
		vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);

#if ENABLE_NVPERF
		apiTracer.EndRange(commandBuffer, passIndex);
		apiTracer.BeginRange(commandBuffer, "DDGI probe status update", 2, passIndex);
#endif

		vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, DDGI_Update_Probe_Status_Label);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, ProbeStatusComputePassPipeline);
		dynamicDiffuse_RTGI->DispatchProbeStatus(commandBuffer, ProbeStatusPipelineLayout, currentFrame);

		vk::BufferMemoryBarrier barrier3{};
		barrier3.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
		barrier3.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		barrier3.buffer = dynamicDiffuse_RTGI->ProbeDataStorageBuffers.buffer;
		barrier3.offset = 0;
		barrier3.size = VK_WHOLE_SIZE;
		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eRayTracingShaderKHR, {}, 0, nullptr, 1, &barrier3, 0, nullptr);
		vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);

#if ENABLE_NVPERF
		apiTracer.EndRange(commandBuffer, passIndex);
		apiTracer.BeginRange(commandBuffer, "DDGI Image copy", 2, passIndex);
#endif

		ImageTransitionData TransitiontoGeneraCompute{};
		TransitiontoGeneraCompute.oldlayout = vk::ImageLayout::eGeneral;
		TransitiontoGeneraCompute.newlayout = vk::ImageLayout::eGeneral;
		TransitiontoGeneraCompute.SourceAccessflag = vk::AccessFlagBits::eShaderWrite;
		TransitiontoGeneraCompute.DestinationAccessflag = vk::AccessFlagBits::eTransferRead;
		TransitiontoGeneraCompute.SourceOnThePipeline = vk::PipelineStageFlagBits::eComputeShader;
		TransitiontoGeneraCompute.DestinationOnThePipeline = vk::PipelineStageFlagBits::eTransfer;
		bufferManger.TransitionImage(commandBuffer, &dynamicDiffuse_RTGI->IradianceImageAtlasImage, TransitiontoGeneraCompute);
		bufferManger.TransitionImage(commandBuffer, &dynamicDiffuse_RTGI->VisibilityImageAtlasImage, TransitiontoGeneraCompute);
		bufferManger.TransitionImage(commandBuffer, &dynamicDiffuse_RTGI->Prev_IradianceImageAtlasImage, TransitiontoGeneraCompute);
		bufferManger.TransitionImage(commandBuffer, &dynamicDiffuse_RTGI->Prev_VisibilityImageAtlasImage, TransitiontoGeneraCompute);

		vk::ImageSubresourceLayers SrcLayers{};
		SrcLayers.mipLevel = 0;
		SrcLayers.baseArrayLayer = 0;
		SrcLayers.layerCount = 1;
		SrcLayers.aspectMask = vk::ImageAspectFlagBits::eColor;
		vk::Extent3D DdgiImageSize = { dynamicDiffuse_RTGI->IradianceImageExtent.width, dynamicDiffuse_RTGI->IradianceImageExtent.height, 1 };
		bufferManger.CopyImageToAnotherImage(commandBuffer, dynamicDiffuse_RTGI->IradianceImageAtlasImage, vk::ImageLayout::eGeneral, SrcLayers, dynamicDiffuse_RTGI->Prev_IradianceImageAtlasImage, vk::ImageLayout::eGeneral, SrcLayers, DdgiImageSize, vulkanContext.graphicsQueue);
		bufferManger.CopyImageToAnotherImage(commandBuffer, dynamicDiffuse_RTGI->VisibilityImageAtlasImage, vk::ImageLayout::eGeneral, SrcLayers, dynamicDiffuse_RTGI->Prev_VisibilityImageAtlasImage, vk::ImageLayout::eGeneral, SrcLayers, DdgiImageSize, vulkanContext.graphicsQueue);

#if ENABLE_NVPERF
		apiTracer.EndRange(commandBuffer, passIndex);
		apiTracer.BeginRange(commandBuffer, "DDGI Sample from Probes", 2, passIndex);
#endif

		vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, DDGI_Sample_From_PorbeLabel);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, SampleDDGIComputePassPipeline);
		dynamicDiffuse_RTGI->DispatchSampleGIFromProbeDataCompute(commandBuffer, SampleDDGIComputePipelineLayout, currentFrame);

		vk::ImageMemoryBarrier imagebarrier2{};
		imagebarrier2.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
		imagebarrier2.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		imagebarrier2.oldLayout = vk::ImageLayout::eUndefined;
		imagebarrier2.newLayout = vk::ImageLayout::eGeneral;
		imagebarrier2.image = dynamicDiffuse_RTGI->Probe_Sampled_GI_Image.image;
		imagebarrier2.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imagebarrier2.subresourceRange.baseMipLevel = 0;
		imagebarrier2.subresourceRange.levelCount = 1;
		imagebarrier2.subresourceRange.baseArrayLayer = 0;
		imagebarrier2.subresourceRange.layerCount = 1;
		imagebarrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imagebarrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eFragmentShader, {}, 0, nullptr, 0, nullptr, 1, &imagebarrier2);
		vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);

#if ENABLE_NVPERF
		apiTracer.EndRange(commandBuffer, passIndex);
#endif
	}

	// ==========================================
	// 4. RESTIR DI PASS
	// ==========================================
	vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, ReSTIR_Label);
	{
		if (DefferedDecider == 2) {
#if ENABLE_NVPERF
			apiTracer.BeginRange(commandBuffer, "ReSTIR DI", 3, passIndex);
#endif
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, ReSTIR_Temporal_RT_PassPipeline);

			Restir_DI->Draw(
				ReSTIR_DI_Temporal_raygenShaderBindingTableBuffer,
				ReSTIR_DI_Temporal_hitShaderBindingTableBuffer,
				ReSTIR_DI_Temporal_missShaderBindingTableBuffer,
				commandBuffer,
				ReSTIR_Temporal_RT_PipelineLayout,
				currentFrame);

			vk::MemoryBarrier2 memoryBarrier{};
			memoryBarrier.srcStageMask = vk::PipelineStageFlagBits2::eRayTracingShaderKHR;
			memoryBarrier.srcAccessMask = vk::AccessFlagBits2::eShaderStorageWrite | vk::AccessFlagBits2::eShaderStorageRead;
			memoryBarrier.dstStageMask = vk::PipelineStageFlagBits2::eRayTracingShaderKHR;
			memoryBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite;

			vk::DependencyInfo dependencyInfo{};
			dependencyInfo.memoryBarrierCount = 1;
			dependencyInfo.pMemoryBarriers = &memoryBarrier;
			commandBuffer.pipelineBarrier2(dependencyInfo);

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, ReSTIR_SPATIAL_RT_PassPipeline);

			Restir_DI->Draw(
				ReSTIR_DI_Spatial_raygenShaderBindingTableBuffer,
				ReSTIR_DI_Spatial_hitShaderBindingTableBuffer,
				ReSTIR_DI_Spatial_missShaderBindingTableBuffer,
				commandBuffer,
				ReSTIR_Spatial_RT_PipelineLayout,
				currentFrame);

#if ENABLE_NVPERF
			apiTracer.EndRange(commandBuffer, passIndex);
#endif

			ImageData* ReSTIR_Image = nullptr;

			if (bUseDLSS)
			{
#if ENABLE_NVPERF
				apiTracer.BeginRange(commandBuffer, "DLSS", 4, passIndex);
#endif
				vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, RayReconstruction);

				vk::ImageMemoryBarrier barrier{};
				barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
				barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
				barrier.oldLayout = vk::ImageLayout::eGeneral;
				barrier.newLayout = vk::ImageLayout::eGeneral;
				barrier.image = Restir_DI->ReSTIRDI_Results.image;
				barrier.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
				commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eRayTracingShaderKHR, vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr, 0, nullptr, 1, &barrier);

				vulkanContext.DLSS_IntergrationRef->render(commandBuffer, Restir_DI->ReSTIRDI_Results, gbuffer,
					DepthTextureData, Restir_DI->ReSTIRDI_Denoised_Results,
					(VkFormat)vulkanContext.FindCompatableDepthFormat(), deltaTime);
				vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);

				ReSTIR_Image = &Restir_DI->ReSTIRDI_Denoised_Results;
#if ENABLE_NVPERF
				apiTracer.EndRange(commandBuffer, passIndex);
#endif
			}
			else
			{
				ReSTIR_Image = &Restir_DI->ReSTIRDI_Results;
			}

			// Skybox over ReSTIR
			vk::RenderingAttachmentInfo SkyBoxRenderAttachInfo;
			SkyBoxRenderAttachInfo.clearValue = clearColor;
			SkyBoxRenderAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
			SkyBoxRenderAttachInfo.imageView = ReSTIR_Image->imageView;
			SkyBoxRenderAttachInfo.loadOp = vk::AttachmentLoadOp::eLoad;
			SkyBoxRenderAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

			vk::RenderingAttachmentInfo DepthAttachInfo;
			DepthAttachInfo.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
			DepthAttachInfo.imageView = DepthTextureData.imageView;
			DepthAttachInfo.loadOp = vk::AttachmentLoadOp::eLoad;
			DepthAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;
			DepthAttachInfo.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

			vk::RenderingInfo SkyBoxRenderInfo{};
			SkyBoxRenderInfo.layerCount = 1;
			SkyBoxRenderInfo.colorAttachmentCount = 1;
			SkyBoxRenderInfo.pColorAttachments = &SkyBoxRenderAttachInfo;
			SkyBoxRenderInfo.pDepthAttachment = &DepthAttachInfo;
			SkyBoxRenderInfo.renderArea.extent.width = vulkanContext.swapchainExtent.width;
			SkyBoxRenderInfo.renderArea.extent.height = vulkanContext.swapchainExtent.height;

			commandBuffer.setViewport(0, 1, &viewport);
			commandBuffer.setScissor(0, 1, &scissor);
			commandBuffer.beginRendering(SkyBoxRenderInfo);
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, SkyBoxgraphicsPipeline);
			skyBox->Draw(commandBuffer, SkyBoxpipelineLayout, currentFrame);
			commandBuffer.endRendering();

			// Light Visualizers over ReSTIR
			vk::RenderingAttachmentInfo LightPassColorAttachmentInfo{};
			LightPassColorAttachmentInfo.imageView = ReSTIR_Image->imageView;
			LightPassColorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
			LightPassColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eLoad;
			LightPassColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
			LightPassColorAttachmentInfo.clearValue = clearColor;

			vk::RenderingAttachmentInfo depthStencilAttachment;
			depthStencilAttachment.imageView = DepthTextureData.imageView;
			depthStencilAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			depthStencilAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
			depthStencilAttachment.storeOp = vk::AttachmentStoreOp::eStore;
			depthStencilAttachment.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

			vk::RenderingInfo renderingInfo{};
			renderingInfo.renderArea.offset = imageoffset;
			renderingInfo.renderArea.extent.height = vulkanContext.swapchainExtent.height;
			renderingInfo.renderArea.extent.width = vulkanContext.swapchainExtent.width;
			renderingInfo.layerCount = 1;
			renderingInfo.colorAttachmentCount = 1;
			renderingInfo.pColorAttachments = &LightPassColorAttachmentInfo;
			renderingInfo.pDepthAttachment = &depthStencilAttachment;

			if (bWireFrame)
			{
				vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_LINE);
			}
			else
			{
				vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_FILL);
			}

			vk::MemoryBarrier2 memoryBarrier2{};
			memoryBarrier2.srcStageMask = vk::PipelineStageFlagBits2::eAllGraphics;
			memoryBarrier2.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
			memoryBarrier2.dstStageMask = vk::PipelineStageFlagBits2::eAllGraphics;
			memoryBarrier2.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite;

			vk::DependencyInfo dependencyInfo2{};
			dependencyInfo2.memoryBarrierCount = 1;
			dependencyInfo2.pMemoryBarriers = &memoryBarrier2;
			commandBuffer.pipelineBarrier2(dependencyInfo2);

			commandBuffer.setViewport(0, 1, &viewport);
			commandBuffer.setScissor(0, 1, &scissor);
			commandBuffer.beginRendering(renderingInfo);
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, LightgraphicsPipeline);

			if (!bHideLights)
			{
				for (auto& light : lights)
				{
					light->Draw(commandBuffer, LightpipelineLayout, currentFrame);
				}
			}
			commandBuffer.endRendering();

			// DDGI Probes over ReSTIR
			{
				vk::RenderingAttachmentInfo ProbeDrawColorAttachmentInfo{};
				ProbeDrawColorAttachmentInfo.imageView = ReSTIR_Image->imageView;
				ProbeDrawColorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
				ProbeDrawColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eLoad;
				ProbeDrawColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
				ProbeDrawColorAttachmentInfo.clearValue = clearColor;

				vk::RenderingInfo probeRenderingInfo{};
				probeRenderingInfo.renderArea.offset = imageoffset;
				probeRenderingInfo.renderArea.extent.height = vulkanContext.swapchainExtent.height;
				probeRenderingInfo.renderArea.extent.width = vulkanContext.swapchainExtent.width;
				probeRenderingInfo.layerCount = 1;
				probeRenderingInfo.colorAttachmentCount = 1;
				probeRenderingInfo.pColorAttachments = &ProbeDrawColorAttachmentInfo;
				probeRenderingInfo.pDepthAttachment = &depthStencilAttachment;

				if (bWireFrame)
				{
					vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_LINE);
				}
				else
				{
					vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_FILL);
				}

				commandBuffer.setViewport(0, 1, &viewport);
				commandBuffer.setScissor(0, 1, &scissor);
				commandBuffer.beginRendering(probeRenderingInfo);
				commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, DDGIProbePipeline);
				dynamicDiffuse_RTGI->Draw(commandBuffer, DDGIProbepipelineLayout, currentFrame);
				commandBuffer.endRendering();
			}
		}
	}
	vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);

	// ==========================================
	// 5. DIRECT LIGHTING & PATH TRACING PASS
	// ==========================================
	if (DefferedDecider != 2) {
		vulkanContext.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, DirectLighting_Label);
		{
			if (DefferedDecider == 3 || DefferedDecider == 0) {
#if ENABLE_NVPERF
				apiTracer.BeginRange(commandBuffer, "Brute force Direct Lighting ", 1, passIndex);
#endif
				commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, DeferedLightingPassPipeline);
				lighting_RTX->Draw(
					Lighting_raygenShaderBindingTableBuffer,
					Lighting_hitShaderBindingTableBuffer,
					Lighting_missShaderBindingTableBuffer,
					commandBuffer,
					DeferedLightingPassPipelineLayout,
					currentFrame);
#if ENABLE_NVPERF
				apiTracer.EndRange(commandBuffer, passIndex);
#endif
			}
		}
		vulkanContext.vkCmdEndDebugUtilsLabelEXT(commandBuffer);

		// ==========================================
		// 6. COMPOSITE LIGHTING PASS
		// ==========================================
		{
			vk::MemoryBarrier2 lightingToCombinedBarrier{};
			lightingToCombinedBarrier.srcStageMask = vk::PipelineStageFlagBits2::eRayTracingShaderKHR;
			lightingToCombinedBarrier.srcAccessMask = vk::AccessFlagBits2::eShaderStorageWrite | vk::AccessFlagBits2::eShaderWrite;
			lightingToCombinedBarrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
			lightingToCombinedBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite | vk::AccessFlagBits2::eShaderSampledRead;

			vk::DependencyInfo lightingDepInfo{};
			lightingDepInfo.memoryBarrierCount = 1;
			lightingDepInfo.pMemoryBarriers = &lightingToCombinedBarrier;
			commandBuffer.pipelineBarrier2(lightingDepInfo);

			vk::RenderingAttachmentInfo CombinedImageAttachInfo{};
			CombinedImageAttachInfo.imageView = Combined_FullScreenQuad->Combined_Lighting_Image.imageView;
			CombinedImageAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
			CombinedImageAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
			CombinedImageAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;
			CombinedImageAttachInfo.clearValue = clearColor;

			vk::RenderingInfo CombinedImageInfo{};
			CombinedImageInfo.layerCount = 1;
			CombinedImageInfo.colorAttachmentCount = 1;
			CombinedImageInfo.pColorAttachments = &CombinedImageAttachInfo;
			CombinedImageInfo.renderArea.extent = vulkanContext.swapchainExtent;

			commandBuffer.beginRendering(CombinedImageInfo);
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, CombinedImagePassPipeline);
			Combined_FullScreenQuad->Draw(commandBuffer, CombinedImagePipelineLayout, currentFrame);
			commandBuffer.endRendering();
		}

		ImageData* CombinedImageRef = nullptr;

		if (bUseDLSS) {
			vk::ImageMemoryBarrier dlssReadBarrier{};
			dlssReadBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
			dlssReadBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			dlssReadBarrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
			dlssReadBarrier.newLayout = vk::ImageLayout::eGeneral;
			dlssReadBarrier.image = Combined_FullScreenQuad->Combined_Lighting_Image.image;
			dlssReadBarrier.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
			commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr, 0, nullptr, 1, &dlssReadBarrier);

			vulkanContext.DLSS_IntergrationRef->render(commandBuffer, Combined_FullScreenQuad->Combined_Lighting_Image, gbuffer,
				DepthTextureData, Combined_FullScreenQuad->Final_Denoised_Image, (VkFormat)vulkanContext.FindCompatableDepthFormat(), deltaTime);
		  
			CombinedImageRef = &Combined_FullScreenQuad->Final_Denoised_Image;
		}
		else
		{
			CombinedImageRef = &Combined_FullScreenQuad->Combined_Lighting_Image;
		}
		
		// Skybox over Combined
		vk::RenderingAttachmentInfo SkyBoxRenderAttachInfo{};
		SkyBoxRenderAttachInfo.imageView = CombinedImageRef->imageView;
		SkyBoxRenderAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		SkyBoxRenderAttachInfo.loadOp = vk::AttachmentLoadOp::eLoad; 
		SkyBoxRenderAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingAttachmentInfo DepthAttachInfo{};
		DepthAttachInfo.imageView = DepthTextureData.imageView;
		DepthAttachInfo.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
		DepthAttachInfo.loadOp = vk::AttachmentLoadOp::eLoad;
		DepthAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingInfo SkyBoxRenderInfo{};
		SkyBoxRenderInfo.layerCount = 1;
		SkyBoxRenderInfo.colorAttachmentCount = 1;
		SkyBoxRenderInfo.pColorAttachments = &SkyBoxRenderAttachInfo;
		SkyBoxRenderInfo.pDepthAttachment = &DepthAttachInfo;
		SkyBoxRenderInfo.renderArea.extent = vulkanContext.swapchainExtent;

		commandBuffer.beginRendering(SkyBoxRenderInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, SkyBoxgraphicsPipeline);
		skyBox->Draw(commandBuffer, SkyBoxpipelineLayout, currentFrame);
		commandBuffer.endRendering();

		// Lights over Combined
		{
			vk::RenderingAttachmentInfo LightPassColorAttachmentInfo{};
			LightPassColorAttachmentInfo.imageView = CombinedImageRef->imageView;
			LightPassColorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
			LightPassColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eLoad;
			LightPassColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
			LightPassColorAttachmentInfo.clearValue = clearColor;

			vk::RenderingAttachmentInfo depthStencilAttachment;
			depthStencilAttachment.imageView = DepthTextureData.imageView;
			depthStencilAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			depthStencilAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
			depthStencilAttachment.storeOp = vk::AttachmentStoreOp::eStore;
			depthStencilAttachment.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

			vk::RenderingInfo renderingInfo{};
			renderingInfo.renderArea.offset = imageoffset;
			renderingInfo.renderArea.extent.height = vulkanContext.swapchainExtent.height;
			renderingInfo.renderArea.extent.width = vulkanContext.swapchainExtent.width;
			renderingInfo.layerCount = 1;
			renderingInfo.colorAttachmentCount = 1;
			renderingInfo.pColorAttachments = &LightPassColorAttachmentInfo;
			renderingInfo.pDepthAttachment = &depthStencilAttachment;

			if (bWireFrame)
			{
				vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_LINE);
			}
			else
			{
				vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_FILL);
			}

			commandBuffer.setViewport(0, 1, &viewport);
			commandBuffer.setScissor(0, 1, &scissor);
			commandBuffer.beginRendering(renderingInfo);
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, LightgraphicsPipeline);

			if (!bHideLights)
			{
				for (auto& light : lights)
				{
					light->Draw(commandBuffer, LightpipelineLayout, currentFrame);
				}
			}
			commandBuffer.endRendering();
		}

		// DDGI Probes over Combined
		{
			vk::RenderingAttachmentInfo ProbeDrawColorAttachmentInfo{};
			ProbeDrawColorAttachmentInfo.imageView = CombinedImageRef->imageView;
			ProbeDrawColorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
			ProbeDrawColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eLoad;
			ProbeDrawColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
			ProbeDrawColorAttachmentInfo.clearValue = clearColor;

			vk::RenderingAttachmentInfo depthStencilAttachment;
			depthStencilAttachment.imageView = DepthTextureData.imageView;
			depthStencilAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			depthStencilAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
			depthStencilAttachment.storeOp = vk::AttachmentStoreOp::eStore;
			depthStencilAttachment.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

			vk::RenderingInfo renderingInfo{};
			renderingInfo.renderArea.offset = imageoffset;
			renderingInfo.renderArea.extent.height = vulkanContext.swapchainExtent.height;
			renderingInfo.renderArea.extent.width = vulkanContext.swapchainExtent.width;
			renderingInfo.layerCount = 1;
			renderingInfo.colorAttachmentCount = 1;
			renderingInfo.pColorAttachments = &ProbeDrawColorAttachmentInfo;
			renderingInfo.pDepthAttachment = &depthStencilAttachment;

			if (bWireFrame)
			{
				vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_LINE);
			}
			else
			{
				vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_FILL);
			}

			commandBuffer.setViewport(0, 1, &viewport);
			commandBuffer.setScissor(0, 1, &scissor);
			commandBuffer.beginRendering(renderingInfo);
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, DDGIProbePipeline);
			dynamicDiffuse_RTGI->Draw(commandBuffer, DDGIProbepipelineLayout, currentFrame);
			commandBuffer.endRendering();
		}
	}

	vulkanContext.vkCmdSetPolygonModeEXT(commandBuffer, VkPolygonMode::VK_POLYGON_MODE_FILL);

	// ==========================================
	// 7. IMGUI UI & GAMMA CORRECTION PASS
	// ==========================================
	vk::MemoryBarrier2 memoryBarrier{};
	memoryBarrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput | vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eRayTracingShaderKHR;
	memoryBarrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eShaderWrite;
	memoryBarrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
	memoryBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;

	vk::DependencyInfo dependencyInfo{};
	dependencyInfo.memoryBarrierCount = 1;
	dependencyInfo.pMemoryBarriers = &memoryBarrier;

	userinterface.RenderUi(commandBuffer, imageIndex, Combined_FullScreenQuad->IMGUI_PRESENT_IMAGE);

	// Transition IMGUI_PRESENT_IMAGE to eGeneral for Gamma Correction
	{
		vk::ImageMemoryBarrier uiBarrier{};
		uiBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		uiBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		uiBarrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
		uiBarrier.newLayout = vk::ImageLayout::eGeneral;
		uiBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		uiBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		uiBarrier.image = Combined_FullScreenQuad->IMGUI_PRESENT_IMAGE.image;
		uiBarrier.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eFragmentShader,
			{},
			0, nullptr,
			0, nullptr,
			1, &uiBarrier
		);
	}

	// Prepare Swapchain Image for Color Attachment
	{
		vk::ImageMemoryBarrier barrier{};
		barrier.oldLayout = vk::ImageLayout::eUndefined;
		barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = vulkanContext.swapchainImageData[imageIndex].image;
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = vk::AccessFlagBits::eNone;
		barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			{},
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}

	// Gamma Correction to Swapchain Output
	{
		vk::RenderingAttachmentInfo SwapchainImageAttachInfo;
		SwapchainImageAttachInfo.clearValue = clearColor;
		SwapchainImageAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		SwapchainImageAttachInfo.imageView = vulkanContext.swapchainImageData[imageIndex].imageView;
		SwapchainImageAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
		SwapchainImageAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;

		vk::RenderingInfo GammaCorrectedImageInfo{};
		GammaCorrectedImageInfo.layerCount = 1;
		GammaCorrectedImageInfo.colorAttachmentCount = 1;
		GammaCorrectedImageInfo.pColorAttachments = &SwapchainImageAttachInfo;
		GammaCorrectedImageInfo.renderArea.extent.width = vulkanContext.swapchainExtent.width;
		GammaCorrectedImageInfo.renderArea.extent.height = vulkanContext.swapchainExtent.height;

		commandBuffer.setViewport(0, 1, &viewport);
		commandBuffer.setScissor(0, 1, &scissor);
		commandBuffer.beginRendering(GammaCorrectedImageInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, Gamma_Corrected_IMGUI_PassPipeline);
		Combined_FullScreenQuad->DrawGammaCorrection(commandBuffer, Gamma_Corrected_IMGUI_PipelineLayout, currentFrame);
		commandBuffer.endRendering();
	}

	// Transition Swapchain Image to Present
	{
		vk::ImageMemoryBarrier barrier{};
		barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
		barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = vulkanContext.swapchainImageData[imageIndex].image;
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eNone;

		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eBottomOfPipe,
			{},
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}

	commandBuffer.end();
	FrameMark;
}
