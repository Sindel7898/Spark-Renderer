#include "RT_Reflections.h"
#include "VulkanContext.h"
#include "BufferManager.h"
#include "Camera.h"
#include <stdexcept>
#include "Model.h"

RT_Reflections::RT_Reflections( VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger)
{
	bufferManager = buffermanger;
	vulkanContext = vulkancontext;
	camera        = rcamera;
	commandPool   = commandpool;
	CreateUniformBuffer();
	createRayTracingDescriptorSetLayout();
}
void RT_Reflections::CreateUniformBuffer() {

	{
		RayGen_UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		RayGen_UniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

		VkDeviceSize RayGenuniformBufferSize = sizeof(Reflection_RayGen_UniformBufferData);

		for (size_t i = 0; i < RayGen_UniformBuffers.size(); i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "RayGen Uniform Buffer" + i;
			bufferManager->CreateBuffer(&bufferdata, RayGenuniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
			RayGen_UniformBuffers[i] = bufferdata;

			RayGen_UniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
		}
	}

	{
		IndexStorageBuffers.resize(1);
		IndexStorageBuffersMappedMem.resize(1);

		VkDeviceSize RayGenIndexStorageBufferSize = sizeof(uint32_t) * bufferManager->AllScene_IndexGeometryData.size();

		for (size_t i = 0; i < IndexStorageBuffers.size(); i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "RayGen Index Storage Buffer" + i;
			bufferManager->CreateBuffer(&bufferdata, RayGenIndexStorageBufferSize, vk::BufferUsageFlagBits::eStorageBuffer, commandPool, vulkanContext->graphicsQueue);
			IndexStorageBuffers[i] = bufferdata;

			IndexStorageBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);

			memcpy(IndexStorageBuffersMappedMem[i], bufferManager->AllScene_IndexGeometryData.data(), sizeof(uint32_t) * bufferManager->AllScene_IndexGeometryData.size());

			bufferManager->UnmapMemory(bufferdata);

		}

	}

	{
		VertexStorageBuffers.resize(1);
		VertexStorageBuffersMappedMem.resize(1);

		VkDeviceSize RayGenIndexStorageBufferSize = sizeof(PaddedModelVertex) * bufferManager->AllScene_VertexGeometryData.size();

		for (size_t i = 0; i < VertexStorageBuffers.size(); i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "RayGen Vertex Storage Buffer" + i;
			bufferManager->CreateBuffer(&bufferdata, RayGenIndexStorageBufferSize, vk::BufferUsageFlagBits::eStorageBuffer, commandPool, vulkanContext->graphicsQueue);
			VertexStorageBuffers[i] = bufferdata;

			VertexStorageBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);

			memcpy(VertexStorageBuffersMappedMem[i], bufferManager->AllScene_VertexGeometryData.data(), sizeof(PaddedModelVertex) * bufferManager->AllScene_VertexGeometryData.size());

			bufferManager->UnmapMemory(bufferdata);

		}
	}

	{
		OffsetStorageBuffers.resize(1);
		OffsetStorageBuffersMappedMem.resize(1);

		VkDeviceSize RayGenIndexStorageBufferSize = sizeof(VertexAndIndexOffsets) * bufferManager->AllScene_VertexAndIndexOffsets.size();

		for (size_t i = 0; i < OffsetStorageBuffers.size(); i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "RayGen Offset Storage Buffer" + i;
			bufferManager->CreateBuffer(&bufferdata, RayGenIndexStorageBufferSize, vk::BufferUsageFlagBits::eStorageBuffer, commandPool, vulkanContext->graphicsQueue);
			OffsetStorageBuffers[i] = bufferdata;

			OffsetStorageBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);

			memcpy(OffsetStorageBuffersMappedMem[i], bufferManager->AllScene_VertexAndIndexOffsets.data(), sizeof(VertexAndIndexOffsets) * bufferManager->AllScene_VertexAndIndexOffsets.size());

			bufferManager->UnmapMemory(bufferdata);

		}
	}

	{
		TransformationUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		TransformationUniformMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

		VkDeviceSize RayGenIndexStorageBufferSize = sizeof(glm::mat4) * 100;

		for (size_t i = 0; i < TransformationUniformBuffers.size(); i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "RayGen Transformation Uniform Buffer" + i;
			bufferManager->CreateBuffer(&bufferdata, RayGenIndexStorageBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
			TransformationUniformBuffers[i] = bufferdata;

			TransformationUniformMappedMem[i] = bufferManager->MapMemory(bufferdata);

		}
	}


}

void RT_Reflections::CreateStorageImage() {

	 swapchainextent = vk::Extent3D(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, 1);

     ReflectionPassImage.ImageID = "RT Reflection Pass Image";
	 bufferManager->CreateImage(&ReflectionPassImage, swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled);
     ReflectionPassImage.imageView = bufferManager->CreateImageView(&ReflectionPassImage, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
     ReflectionPassImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);
     

}


void RT_Reflections::DestroyStorageImage() {

	bufferManager->DestroyImage(ReflectionPassImage);
}

void RT_Reflections::createRayTracingDescriptorSetLayout(){

	vk::DescriptorSetLayoutBinding TLASLayout{};
	TLASLayout.binding = 0;
	TLASLayout.descriptorCount = 1;
	TLASLayout.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
	TLASLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;


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

	vk::DescriptorSetLayoutBinding ReflectionResultSamplerLayout{};
	ReflectionResultSamplerLayout.binding = 3;
	ReflectionResultSamplerLayout.descriptorCount = 1;
	ReflectionResultSamplerLayout.descriptorType = vk::DescriptorType::eStorageImage;
	ReflectionResultSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

	vk::DescriptorSetLayoutBinding RayGenUniformBufferLayout{};
	RayGenUniformBufferLayout.binding = 4;
	RayGenUniformBufferLayout.descriptorCount = 1;
	RayGenUniformBufferLayout.descriptorType = vk::DescriptorType::eUniformBuffer;
	RayGenUniformBufferLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

	vk::DescriptorSetLayoutBinding IndexStorageBuffersLayout{};
	IndexStorageBuffersLayout.binding = 5;
	IndexStorageBuffersLayout.descriptorCount = 1;
	IndexStorageBuffersLayout.descriptorType = vk::DescriptorType::eStorageBuffer;
	IndexStorageBuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

	vk::DescriptorSetLayoutBinding VertexStorageBuffersLayout{};
	VertexStorageBuffersLayout.binding = 6;
	VertexStorageBuffersLayout.descriptorCount = 1;
	VertexStorageBuffersLayout.descriptorType = vk::DescriptorType::eStorageBuffer;
	VertexStorageBuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

	vk::DescriptorSetLayoutBinding offsetStorageBuffersLayout{};
	offsetStorageBuffersLayout.binding = 7;
	offsetStorageBuffersLayout.descriptorCount = 1;
	offsetStorageBuffersLayout.descriptorType = vk::DescriptorType::eStorageBuffer;
	offsetStorageBuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

	vk::DescriptorSetLayoutBinding trasnformationUniformBuffersLayout{};
	trasnformationUniformBuffersLayout.binding = 8;
	trasnformationUniformBuffersLayout.descriptorCount = 1;
	trasnformationUniformBuffersLayout.descriptorType = vk::DescriptorType::eUniformBuffer;
	trasnformationUniformBuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR; 

	std::array<vk::DescriptorSetLayoutBinding, 9> bindings = {
		                                                       TLASLayout,              
															   AlbedoAssetTexturesSamplerLayout,
															   NormalAssetTexturesSamplerLayout,
		                                                       ReflectionResultSamplerLayout,
		                                                       RayGenUniformBufferLayout,
															   IndexStorageBuffersLayout,
															   VertexStorageBuffersLayout,
															   offsetStorageBuffersLayout,
															   trasnformationUniformBuffersLayout
	};



	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();


	if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &RayTracingDescriptorSetLayout) != vk::Result::eSuccess)
	{
		throw std::runtime_error("Failed to create descriptorset layout!");
	}

}

void RT_Reflections::createRaytracedDescriptorSets(vk::DescriptorPool descriptorpool, vk::AccelerationStructureKHR TLAS,GBuffer gbuffer)
{
	{
		// create sets from the pool based on the layout
		// 	     
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, RayTracingDescriptorSetLayout);


		vk::DescriptorSetAllocateInfo allocinfo;
		allocinfo.descriptorPool = descriptorpool;
		allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocinfo.pSetLayouts = layouts.data();

		RayTracingDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, RayTracingDescriptorSets.data());

		////////////////////////////////////////////////////////////////////////////////////////////////
		//specifies what exactly to send
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			vk::WriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
			descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
			descriptorAccelerationStructureInfo.pAccelerationStructures = &TLAS;

			vk::WriteDescriptorSet TLAS_descriptorWrite{};
			TLAS_descriptorWrite.dstSet = RayTracingDescriptorSets[i];
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
			AssetImagSamplerdescriptorWrite.dstSet = RayTracingDescriptorSets[i];
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
			NormalAssetImagSamplerdescriptorWrite.dstSet = RayTracingDescriptorSets[i];
			NormalAssetImagSamplerdescriptorWrite.dstBinding = 2;
			NormalAssetImagSamplerdescriptorWrite.dstArrayElement = 0;
			NormalAssetImagSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			NormalAssetImagSamplerdescriptorWrite.descriptorCount = NormalImageAssetImagesInfos.size();
			NormalAssetImagSamplerdescriptorWrite.pImageInfo = NormalImageAssetImagesInfos.data();

			/////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorImageInfo StoreageImageInfo{};
			StoreageImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			StoreageImageInfo.imageView = ReflectionPassImage.imageView;
			StoreageImageInfo.sampler = ReflectionPassImage.imageSampler;

			vk::WriteDescriptorSet StoreageImagSamplerdescriptorWrite{};
			StoreageImagSamplerdescriptorWrite.dstSet = RayTracingDescriptorSets[i];
			StoreageImagSamplerdescriptorWrite.dstBinding = 3;
			StoreageImagSamplerdescriptorWrite.dstArrayElement = 0;
			StoreageImagSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
			StoreageImagSamplerdescriptorWrite.descriptorCount = 1;
			StoreageImagSamplerdescriptorWrite.pImageInfo = &StoreageImageInfo;

			/////////////////////////////////////////////////////////////////////////////////////

			vk::DescriptorBufferInfo rayuniformbufferInfo{};
			rayuniformbufferInfo.buffer = RayGen_UniformBuffers[i].buffer;
			rayuniformbufferInfo.offset = 0;
			rayuniformbufferInfo.range = sizeof(Reflection_RayGen_UniformBufferData);

			vk::WriteDescriptorSet RayUniformdescriptorWrite{};
			RayUniformdescriptorWrite.dstSet = RayTracingDescriptorSets[i];
			RayUniformdescriptorWrite.dstBinding = 4;
			RayUniformdescriptorWrite.dstArrayElement = 0;
			RayUniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
			RayUniformdescriptorWrite.descriptorCount = 1;
			RayUniformdescriptorWrite.pBufferInfo = &rayuniformbufferInfo;

			vk::DescriptorBufferInfo IndexStorageBuffersInfo{};
			IndexStorageBuffersInfo.buffer = IndexStorageBuffers[0].buffer;
			IndexStorageBuffersInfo.offset = 0;
			IndexStorageBuffersInfo.range  = sizeof(uint32_t) * bufferManager->AllScene_IndexGeometryData.size();;

			vk::WriteDescriptorSet IndexStorageBufferdescriptorWrite{};
			IndexStorageBufferdescriptorWrite.dstSet = RayTracingDescriptorSets[i];
			IndexStorageBufferdescriptorWrite.dstBinding = 5;
			IndexStorageBufferdescriptorWrite.dstArrayElement = 0;
			IndexStorageBufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
			IndexStorageBufferdescriptorWrite.descriptorCount = 1;
			IndexStorageBufferdescriptorWrite.pBufferInfo = &IndexStorageBuffersInfo;

			vk::DescriptorBufferInfo VertexStorageBuffersInfo{};
			VertexStorageBuffersInfo.buffer = VertexStorageBuffers[0].buffer;
			VertexStorageBuffersInfo.offset = 0;
			VertexStorageBuffersInfo.range = sizeof(PaddedModelVertex) * bufferManager->AllScene_VertexGeometryData.size();;

			vk::WriteDescriptorSet VertexStorageBufferdescriptorWrite{};
			VertexStorageBufferdescriptorWrite.dstSet = RayTracingDescriptorSets[i];
			VertexStorageBufferdescriptorWrite.dstBinding = 6;
			VertexStorageBufferdescriptorWrite.dstArrayElement = 0;
			VertexStorageBufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
			VertexStorageBufferdescriptorWrite.descriptorCount = 1;
			VertexStorageBufferdescriptorWrite.pBufferInfo = &VertexStorageBuffersInfo;

			vk::DescriptorBufferInfo OffsetStorageBuffersInfo{};
			OffsetStorageBuffersInfo.buffer = OffsetStorageBuffers[0].buffer;
			OffsetStorageBuffersInfo.offset = 0;
			OffsetStorageBuffersInfo.range = sizeof(VertexAndIndexOffsets) * bufferManager->AllScene_VertexAndIndexOffsets.size();;

			vk::WriteDescriptorSet OffsetStorageBufferdescriptorWrite{};
			OffsetStorageBufferdescriptorWrite.dstSet = RayTracingDescriptorSets[i];
			OffsetStorageBufferdescriptorWrite.dstBinding = 7;
			OffsetStorageBufferdescriptorWrite.dstArrayElement = 0;
			OffsetStorageBufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
			OffsetStorageBufferdescriptorWrite.descriptorCount = 1;
			OffsetStorageBufferdescriptorWrite.pBufferInfo = &OffsetStorageBuffersInfo;

			vk::DescriptorBufferInfo TransformUniformBuffersInfo{};
			TransformUniformBuffersInfo.buffer = TransformationUniformBuffers[i].buffer;
			TransformUniformBuffersInfo.offset = 0;
			TransformUniformBuffersInfo.range = sizeof(glm::mat4) * 100;

			vk::WriteDescriptorSet TransformUniformBufferdescriptorWrite{};
			TransformUniformBufferdescriptorWrite.dstSet = RayTracingDescriptorSets[i];
			TransformUniformBufferdescriptorWrite.dstBinding = 8;
			TransformUniformBufferdescriptorWrite.dstArrayElement = 0;
			TransformUniformBufferdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
			TransformUniformBufferdescriptorWrite.descriptorCount = 1;
			TransformUniformBufferdescriptorWrite.pBufferInfo = &TransformUniformBuffersInfo;

			std::array<vk::WriteDescriptorSet, 9> descriptorWrites{ TLAS_descriptorWrite,
				                                                    AssetImagSamplerdescriptorWrite,
				                                                    NormalAssetImagSamplerdescriptorWrite,
																	StoreageImagSamplerdescriptorWrite,
			                                                        RayUniformdescriptorWrite,
			                                                        IndexStorageBufferdescriptorWrite,
			                                                        VertexStorageBufferdescriptorWrite,
			                                                        OffsetStorageBufferdescriptorWrite,
			                                                        TransformUniformBufferdescriptorWrite };

			vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}

}


void RT_Reflections::UpdateUniformBuffer(uint32_t currentImage, std::vector<std::shared_ptr<Light>>& lightref, std::vector<std::shared_ptr<Model>>& Modelref)
{

	Reflection_RayGen_UniformBufferData RayGent_UniformBufferData;
	RayGent_UniformBufferData.ViewMatrix = glm::inverse(camera->GetViewMatrix());
	RayGent_UniformBufferData.ProjectionMatrix = glm::inverse(camera->GetProjectionMatrix());
	RayGent_UniformBufferData.ProjectionMatrix[1][1] *= -1;

	memcpy(RayGen_UniformBuffersMappedMem[currentImage], &RayGent_UniformBufferData, sizeof(RayGent_UniformBufferData));

	std::vector<glm::mat4> ModelTransfomations;

	for (int i = 0; i < Modelref.size(); i++)
	{

		if (Modelref[i])
		{
			glm::mat4 projmodelInstanceTransformection = Modelref[i]->Instances[0]->GetTransformationMatrix();

			ModelTransfomations.push_back(projmodelInstanceTransformection);
		}
	}

	memcpy(TransformationUniformMappedMem[currentImage], ModelTransfomations.data(), ModelTransfomations.size() * sizeof(glm::mat4));

}


uint32_t RT_Reflections::alignedSize(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}


void RT_Reflections::Draw(BufferData RayGenBuffer, BufferData RayHitBuffer, BufferData RayMisBuffer, vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
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

	int width  = swapchainextent.width;
	int height = swapchainextent.height;
	int depth  = 1;
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, pipelinelayout, 0, 1, &RayTracingDescriptorSets[imageIndex],0,nullptr);
	
	vulkanContext->vkCmdTraceRaysKHR(
		commandbuffer,
		&TEMP_raygenShaderSbtEntry,
		&TEMP_missShaderSbtEntry,
		&TEMP_hitShaderSbtEntry,
		&TEMP_callableShaderSbtEntry,
		width,
		height,
		depth);
}


void RT_Reflections::CleanUp()
{
	if (bufferManager)
	{
		for (auto& RayGen_Buffer : RayGen_UniformBuffers)
		{
			if (RayGen_Buffer.buffer)
			{
				bufferManager->UnmapMemory(RayGen_Buffer);
				bufferManager->DestroyBuffer(RayGen_Buffer);
			}
		}

		for (auto& Buffer : IndexStorageBuffers)
		{
			if (Buffer.buffer)
			{
				bufferManager->DestroyBuffer(Buffer);
			}
		}

		for (auto& Buffer : VertexStorageBuffers)
		{
			if (Buffer.buffer)
			{
				bufferManager->DestroyBuffer(Buffer);
			}
		}

		for (auto& Buffer : OffsetStorageBuffers)
		{
			if (Buffer.buffer)
			{
				bufferManager->DestroyBuffer(Buffer);
			}
		}

		for (auto& Buffer : TransformationUniformBuffers)
		{
			if (Buffer.buffer)
			{
				bufferManager->DestroyBuffer(Buffer);
			}
		}

		vulkanContext->LogicalDevice.destroyDescriptorSetLayout(RayTracingDescriptorSetLayout);
		RayGen_UniformBuffers.clear();
	}

}



