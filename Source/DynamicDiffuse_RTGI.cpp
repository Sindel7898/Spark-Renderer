#include "DynamicDiffuse_RTGI.h"
#include "VulkanContext.h"
#include "BufferManager.h"
#include "Camera.h"
#include <stdexcept>
#include "AssetManager.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

DynamicDiffuse_RTGI::DynamicDiffuse_RTGI(const std::string filepath, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger)
{
	bufferManager = buffermanger;
	vulkanContext = vulkancontext;
	camera        = rcamera;
	commandPool   = commandpool;
	FilePath = filepath;

	CreateVertexAndIndexBuffer();
	CreateUniformBuffer();
	createRayTracingDescriptorSetLayout();
	GenerateGrid();
}
void DynamicDiffuse_RTGI::CreateVertexAndIndexBuffer() {

	storedModelData = &AssetManager::GetInstance().GetStoredModelData(FilePath);

	VkDeviceSize VertexBufferSize = sizeof(storedModelData->VertexData[0]) * storedModelData->VertexData.size();
	vertexBufferData.BufferID = "Model Vertex Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&vertexBufferData, storedModelData->VertexData.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(uint32_t) * storedModelData->IndexData.size();
	indexBufferData.BufferID = "Model Index Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&indexBufferData, storedModelData->IndexData.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, commandPool, vulkanContext->graphicsQueue);


}

void DynamicDiffuse_RTGI::CreateUniformBuffer()
{
	{
		ProbeWorldMatrixUniformBuffers.resize(1);
		ProbeWorldMatrixUniformBuffersMappedMem.resize(1);

		VkDeviceSize VertexuniformBufferSize = sizeof(glm::mat4) * 1000;

		for (size_t i = 0; i < 1; i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "Prob Uniform Buffer" + i;
			bufferManager->CreateBuffer(&bufferdata, VertexuniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
			ProbeWorldMatrixUniformBuffers[i] = bufferdata;

			ProbeWorldMatrixUniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
		}
	}
}

void DynamicDiffuse_RTGI::CreateStorageImage() {

	 swapchainextent = vk::Extent3D(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, 1);

	 IrradianceImageAtlasImage.ImageID = " DDGI Irradiance Atlas Image";
	 bufferManager->CreateImage(&IrradianceImageAtlasImage, swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, true);
	 IrradianceImageAtlasImage.imageView = bufferManager->CreateImageView(&IrradianceImageAtlasImage, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	 IrradianceImageAtlasImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);
     

	 ProbeVisibilityAtlasImage.ImageID = " DDGI Probe Visibility Atlas Image";
	 bufferManager->CreateImage(&ProbeVisibilityAtlasImage, swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, true);
	 ProbeVisibilityAtlasImage.imageView = bufferManager->CreateImageView(&ProbeVisibilityAtlasImage, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
	 ProbeVisibilityAtlasImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);
}


void DynamicDiffuse_RTGI::DestroyStorageImage() {

	bufferManager->DestroyImage(IrradianceImageAtlasImage);
	bufferManager->DestroyImage(ProbeVisibilityAtlasImage);
}

void DynamicDiffuse_RTGI::UpdateProbeCount(glm::vec3 NewProbeCount)
{
	NumOfProbesX = (int)NewProbeCount.x;
	NumOfProbesY = (int)NewProbeCount.y;
	NumOfProbesZ = (int)NewProbeCount.z;


	if (LastNumOfProbesX != NumOfProbesX)
	{

		LastNumOfProbesX = NumOfProbesX;

		GenerateGrid();
	}

	if (LastNumOfProbesY != NumOfProbesY)
	{

		LastNumOfProbesY = NumOfProbesY;

		GenerateGrid();
	}

	if (LastNumOfProbesZ != NumOfProbesZ)
	{

		LastNumOfProbesZ = NumOfProbesZ;

		GenerateGrid();
	}
}

void DynamicDiffuse_RTGI::UpdateProbsOffset(glm::vec3 NewProbeOffset)
{
	ProbeOffset = NewProbeOffset;

	if (LastProbeOffset != ProbeOffset)
	{
		LastProbeOffset = ProbeOffset;

		GenerateGrid();
	}
}

void DynamicDiffuse_RTGI::UpdateGridLocation(glm::vec3 NewGridLocation)
{
	GridLocation = NewGridLocation;

	if (LastProbeOffset != GridLocation)
	{
		LastProbeOffset = GridLocation;

		GenerateGrid();
	}
}

void DynamicDiffuse_RTGI::GenerateGrid()
{
	ProbeLocations.clear();
	ProbeLocations.reserve(NumOfProbesX * NumOfProbesY * NumOfProbesZ);

	for (int x = 0; x < NumOfProbesX; ++x)
	{
		for (int y = 0; y < NumOfProbesY; ++y)
		{
			for (int z = 0; z < NumOfProbesZ; ++z)
			{
				glm::vec3 position = glm::vec3(
					ProbeOffset.x * x,
					ProbeOffset.y * y,
					ProbeOffset.z * z
				);

				glm::mat4 transform = glm::translate(glm::mat4(1.0f), (position + GridLocation));
				ProbeLocations.push_back(transform);
			}
		}
	}

	size_t dataSize = sizeof(glm::mat4) * ProbeLocations.size();
	memcpy(ProbeWorldMatrixUniformBuffersMappedMem[0], ProbeLocations.data(), dataSize);
}

void DynamicDiffuse_RTGI::createRayTracingDescriptorSetLayout(){

	{
		vk::DescriptorSetLayoutBinding IrradianceAtlasSamplerLayout{};
		IrradianceAtlasSamplerLayout.binding = 0;
		IrradianceAtlasSamplerLayout.descriptorCount = 1;
		IrradianceAtlasSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		IrradianceAtlasSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding  VisibilitiyAtlasSamplerLayout{};
		VisibilitiyAtlasSamplerLayout.binding = 1;
		VisibilitiyAtlasSamplerLayout.descriptorCount = 1;
		VisibilitiyAtlasSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		VisibilitiyAtlasSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding  ProbeLocationUniformBuffer{};
		ProbeLocationUniformBuffer.binding = 2;
		ProbeLocationUniformBuffer.descriptorCount = 1;
		ProbeLocationUniformBuffer.descriptorType = vk::DescriptorType::eUniformBuffer;
		ProbeLocationUniformBuffer.stageFlags = vk::ShaderStageFlagBits::eVertex;

		std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {
																   IrradianceAtlasSamplerLayout,
																   VisibilitiyAtlasSamplerLayout,
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
}


void DynamicDiffuse_RTGI::createRaytracedDescriptorSets(vk::DescriptorPool descriptorpool)
{
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
			IrradiancAtlasImageInfo.imageView   =  IrradianceImageAtlasImage.imageView;
			IrradiancAtlasImageInfo.sampler     =  IrradianceImageAtlasImage.imageSampler;
		

			vk::WriteDescriptorSet IrradianceAtlasImageSamplerdescriptorWrite{};
			IrradianceAtlasImageSamplerdescriptorWrite.dstSet          = ProbeDescriptorSets[i];
			IrradianceAtlasImageSamplerdescriptorWrite.dstBinding      = 0;
			IrradianceAtlasImageSamplerdescriptorWrite.dstArrayElement = 0;
			IrradianceAtlasImageSamplerdescriptorWrite.descriptorType  = vk::DescriptorType::eCombinedImageSampler;
			IrradianceAtlasImageSamplerdescriptorWrite.descriptorCount = 1;
			IrradianceAtlasImageSamplerdescriptorWrite.pImageInfo      = &IrradiancAtlasImageInfo;

			vk::DescriptorImageInfo ProbeVisibilityAtlasImageInfo{};
			ProbeVisibilityAtlasImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			ProbeVisibilityAtlasImageInfo.imageView   = ProbeVisibilityAtlasImage.imageView;
			ProbeVisibilityAtlasImageInfo.sampler     = ProbeVisibilityAtlasImage.imageSampler;


			vk::WriteDescriptorSet  ProbeVisibilityAtlasSamplerdescriptorWrite{};
			ProbeVisibilityAtlasSamplerdescriptorWrite.dstSet = ProbeDescriptorSets[i];
			ProbeVisibilityAtlasSamplerdescriptorWrite.dstBinding = 1;
			ProbeVisibilityAtlasSamplerdescriptorWrite.dstArrayElement = 0;
			ProbeVisibilityAtlasSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			ProbeVisibilityAtlasSamplerdescriptorWrite.descriptorCount = 1;
			ProbeVisibilityAtlasSamplerdescriptorWrite.pImageInfo = &ProbeVisibilityAtlasImageInfo;

			vk::DescriptorBufferInfo ProbeLocationbufferInfo{};
			ProbeLocationbufferInfo.buffer = ProbeWorldMatrixUniformBuffers[0].buffer;
			ProbeLocationbufferInfo.offset = 0;
			ProbeLocationbufferInfo.range = sizeof(glm::mat4) * 1000;

			vk::WriteDescriptorSet ProbeLocationbufferdescriptorWrite{};
			ProbeLocationbufferdescriptorWrite.dstSet = ProbeDescriptorSets[i];
			ProbeLocationbufferdescriptorWrite.dstBinding = 2;
			ProbeLocationbufferdescriptorWrite.dstArrayElement = 0;
			ProbeLocationbufferdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
			ProbeLocationbufferdescriptorWrite.descriptorCount = 1;
			ProbeLocationbufferdescriptorWrite.pBufferInfo = &ProbeLocationbufferInfo;


			std::array<vk::WriteDescriptorSet, 3> descriptorWrites{ IrradianceAtlasImageSamplerdescriptorWrite,
																	ProbeVisibilityAtlasSamplerdescriptorWrite,ProbeLocationbufferdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}
}


void DynamicDiffuse_RTGI::UpdateUniformBuffer(uint32_t currentImage, std::vector<std::shared_ptr<Light>>& lightref, std::vector<std::shared_ptr<Model>>& Modelref)
{


}


uint32_t DynamicDiffuse_RTGI::alignedSize(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}


void DynamicDiffuse_RTGI::Draw(BufferData RayGenBuffer, BufferData RayHitBuffer, BufferData RayMisBuffer, vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
{
	//vk::BufferDeviceAddressInfo raygenShaderBindingTableDeviceAdressesInfo;
	//raygenShaderBindingTableDeviceAdressesInfo.buffer = RayGenBuffer.buffer;
	//
	//vk::BufferDeviceAddressInfo missShaderBindingTableDeviceAdressesInfo;
	//missShaderBindingTableDeviceAdressesInfo.buffer = RayMisBuffer.buffer;
	//
	//vk::BufferDeviceAddressInfo hitShaderBindingTableDeviceAdressesInfo;
	//hitShaderBindingTableDeviceAdressesInfo.buffer = RayHitBuffer.buffer;
	//
	//auto raygenShaderBindingTableAdress = vulkanContext->LogicalDevice.getBufferAddress(raygenShaderBindingTableDeviceAdressesInfo);
	//auto missShaderBindingTableAdress   = vulkanContext->LogicalDevice.getBufferAddress(missShaderBindingTableDeviceAdressesInfo);
	//auto hitShaderBindingTableAdress    = vulkanContext->LogicalDevice.getBufferAddress(hitShaderBindingTableDeviceAdressesInfo);
	//
	//
	//const uint32_t handleSizeAligned = alignedSize(
	//	vulkanContext->RayTracingPipelineProperties.shaderGroupHandleSize,
	//	vulkanContext->RayTracingPipelineProperties.shaderGroupHandleAlignment);
	//
	//vk::StridedDeviceAddressRegionKHR    raygenShaderSbtEntry{};
	//raygenShaderSbtEntry.deviceAddress = raygenShaderBindingTableAdress;
	//raygenShaderSbtEntry.stride = handleSizeAligned;
	//raygenShaderSbtEntry.size = handleSizeAligned;
	//
	//
	//vk::StridedDeviceAddressRegionKHR  missShaderSbtEntry{};
	//missShaderSbtEntry.deviceAddress = missShaderBindingTableAdress;
	//missShaderSbtEntry.stride = handleSizeAligned;
	//missShaderSbtEntry.size = handleSizeAligned;
	//
	//vk::StridedDeviceAddressRegionKHR hitShaderSbtEntry{};
	//hitShaderSbtEntry.deviceAddress = hitShaderBindingTableAdress;
	//hitShaderSbtEntry.stride = handleSizeAligned;
	//hitShaderSbtEntry.size = handleSizeAligned;
	//
	////vk::StridedDeviceAddressRegionKHR hitShaderSbtEntry{};
	//
	//vk::StridedDeviceAddressRegionKHR callableShaderSbtEntry{};
	//
	//VkStridedDeviceAddressRegionKHR  TEMP_raygenShaderSbtEntry   = static_cast<VkStridedDeviceAddressRegionKHR>(raygenShaderSbtEntry);
	//VkStridedDeviceAddressRegionKHR  TEMP_missShaderSbtEntry     = static_cast<VkStridedDeviceAddressRegionKHR>(missShaderSbtEntry);;
	//VkStridedDeviceAddressRegionKHR  TEMP_hitShaderSbtEntry      = static_cast<VkStridedDeviceAddressRegionKHR>(hitShaderSbtEntry);;
	//VkStridedDeviceAddressRegionKHR  TEMP_callableShaderSbtEntry = static_cast<VkStridedDeviceAddressRegionKHR>(callableShaderSbtEntry);;
	//
	//int width  = swapchainextent.width;
	//int height = swapchainextent.height;
	//int depth  = 1;
	//commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, pipelinelayout, 0, 1, &RayTracingDescriptorSets[imageIndex],0,nullptr);
	//
	//vulkanContext->vkCmdTraceRaysKHR(
	//	commandbuffer,
	//	&TEMP_raygenShaderSbtEntry,
	//	&TEMP_missShaderSbtEntry,
	//	&TEMP_hitShaderSbtEntry,
	//	&TEMP_callableShaderSbtEntry,
	//	width,
	//	height,
	//	depth);
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

				commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(CameraConstantBuffer), &cameraConstantBuffer);
				commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &ProbeDescriptorSets[imageIndex], 0, nullptr);

				commandBuffer.drawIndexed(node->meshPrimitives[i].numIndices, ProbeLocations.size(), node->meshPrimitives[i].indicesStart, 0, 0);
			}
		}



		DrawNode(commandBuffer, pipelineLayout, imageIndex, node->children);
	}
}


void DynamicDiffuse_RTGI::Draw(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer vertexBuffers[] = { vertexBufferData.buffer };

	commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
	commandBuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint32);

	DrawNode(commandBuffer, pipelineLayout, imageIndex, storedModelData->nodes);
}

void DynamicDiffuse_RTGI::CleanUp()
{

}



