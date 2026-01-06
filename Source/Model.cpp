#include "Model.h"
#include <stdexcept>
#include <chrono>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

Model::Model(const std::string filepath, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger)
	      : Drawable()
{

	FilePath = filepath;
	vulkanContext = vulkancontext;
	commandPool = commandpool;
	camera = rcamera;
	bufferManager = buffermanger;

	CreateVertexAndIndexBuffer();

	LoadTextures();
	CreateBLAS();
	CreateUniformBuffer();

	createDescriptorSetLayout();
	Instantiate();
}

void Model::LoadTextures()
{
	GlobalTextureOffset = static_cast<uint32_t>(bufferManager->AllScene_Albedo_Images.size());

	std::vector<StoredImageData> ModelTextures = AssetManager::GetInstance().GetStoredImageData(FilePath);

    materialCount = ModelTextures.size() / 5;

	for (size_t i = 0; i < materialCount; i++) {


		ImageData  albedoTextureData;

		albedoTextureData.ImageID = FilePath + "Albedo Image" + std::to_string(i);

		StoredImageData AlbedoImageData = ModelTextures[i * 5 + 0];
		vk::DeviceSize AlbedoImagesize = AlbedoImageData.imageWidth * AlbedoImageData.imageHeight * 4;

		bufferManager->CreateTextureImage(&albedoTextureData,AlbedoImageData.imageData, AlbedoImagesize, AlbedoImageData.imageWidth, AlbedoImageData.imageHeight, vk::Format::eR8G8B8A8Srgb, commandPool, vulkanContext->graphicsQueue);

		AlbedoTextures.push_back(albedoTextureData);


		ImageData  normalTextureData;
		normalTextureData.ImageID = FilePath + "Normal Image" + std::to_string(i);

		StoredImageData NormalImageData = ModelTextures[i * 5 + 1];
		vk::DeviceSize NormalImagesize = NormalImageData.imageWidth * NormalImageData.imageHeight * 4;

		 bufferManager->CreateTextureImage(&normalTextureData,NormalImageData.imageData, NormalImagesize, NormalImageData.imageWidth, NormalImageData.imageHeight, vk::Format::eR8G8B8A8Unorm, commandPool, vulkanContext->graphicsQueue);

		NormalTextures.push_back(normalTextureData);


		ImageData  MetallicRoughnessTextureData;
		MetallicRoughnessTextureData.ImageID = FilePath + "Metallic Roughness Image" + std::to_string(i);

		StoredImageData MetallicRoughnessImageData = ModelTextures[i * 5 + 2];
		vk::DeviceSize  MetallicRoughnessImagesize = MetallicRoughnessImageData.imageWidth * MetallicRoughnessImageData.imageHeight * 4;

		bufferManager->CreateTextureImage(&MetallicRoughnessTextureData,MetallicRoughnessImageData.imageData, MetallicRoughnessImagesize, MetallicRoughnessImageData.imageWidth, MetallicRoughnessImageData.imageHeight, vk::Format::eR8G8B8A8Unorm, commandPool, vulkanContext->graphicsQueue);

		MetallicRoughnessTextures.push_back(MetallicRoughnessTextureData);


		ImageData  AOTextureData;
		AOTextureData.ImageID = FilePath + "AO Image" + std::to_string(i);

		StoredImageData AOImageData = ModelTextures[i * 5 + 3];
		vk::DeviceSize  AOImagesize = AOImageData.imageWidth * AOImageData.imageHeight * 4;

		bufferManager->CreateTextureImage(&AOTextureData,AOImageData.imageData, AOImagesize, AOImageData.imageWidth, AOImageData.imageHeight, vk::Format::eR8G8B8A8Unorm, commandPool, vulkanContext->graphicsQueue);

		AOTextures.push_back(AOTextureData);


		ImageData  EmissiveTextureData;
		EmissiveTextureData.ImageID = FilePath + "Emissive Image" + std::to_string(i);

		StoredImageData EmissiveImageData = ModelTextures[i * 5 + 4];
		vk::DeviceSize  EmissiveImagesize = EmissiveImageData.imageWidth * EmissiveImageData.imageHeight * 4;

		bufferManager->CreateTextureImage(&EmissiveTextureData, EmissiveImageData.imageData, EmissiveImagesize, EmissiveImageData.imageWidth, EmissiveImageData.imageHeight, vk::Format::eR8G8B8A8Srgb, commandPool, vulkanContext->graphicsQueue);

		EmissiveTextures.push_back(EmissiveTextureData);
	}

	for (ImageData& albedo : AlbedoTextures)
	{
		bufferManager->AllScene_Albedo_Images.push_back(&albedo);

	}

	for (ImageData& normal : NormalTextures)
	{
		bufferManager->AllScene_Normal_Images.push_back(&normal);

	}

	for (ImageData& MetallicRoughnessTexture : MetallicRoughnessTextures)
	{
		bufferManager->AllScene_MetalicRoughness_Images.push_back(&MetallicRoughnessTexture);

	}

	for (ImageData& EmmisiveTexture : EmissiveTextures)
	{
		bufferManager->AllScene_Emissive_Images.push_back(&EmmisiveTexture);

	}
}

void Model::CreateVertexAndIndexBuffer()
{


	storedModelData = &AssetManager::GetInstance().GetStoredModelData(FilePath);

	m_baseVertexOffset = static_cast<uint32_t>(bufferManager->AllScene_VertexGeometryData.size());
	m_baseIndexOffset = static_cast<uint32_t>(bufferManager->AllScene_IndexGeometryData.size());

	VkDeviceSize VertexBufferSize = sizeof(storedModelData->VertexData[0]) * storedModelData->VertexData.size();
	vertexBufferData.BufferID = "Model Vertex Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&vertexBufferData,storedModelData->VertexData.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(uint32_t) * storedModelData->IndexData.size();
	indexBufferData.BufferID = "Model Index Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&indexBufferData,storedModelData->IndexData.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, commandPool, vulkanContext->graphicsQueue);
   


	for (int i = 0; i < storedModelData->VertexData.size(); i++)
	{
		PaddedModelVertex paddedModelVertex;
		paddedModelVertex.vert_Padding    = glm::vec4(storedModelData->VertexData[i].vert, 0);
		paddedModelVertex.text_Padding    = glm::vec4(storedModelData->VertexData[i].text, 0,0);
		paddedModelVertex.normal_Padding  = glm::vec4(storedModelData->VertexData[i].normal, 0);
		paddedModelVertex.tangent_Padding = glm::vec4(storedModelData->VertexData[i].tangent, 0);
 
		bufferManager->AllScene_VertexGeometryData.push_back(paddedModelVertex);
	}

	for (int i = 0; i < storedModelData->IndexData.size(); i++)
	{
		uint32_t rebasedIndex = m_baseVertexOffset + storedModelData->IndexData[i];
		bufferManager->AllScene_IndexGeometryData.push_back(rebasedIndex);
	}
}


void Model::CreateBLAS()
{
	vk::BufferDeviceAddressInfo VertexBufferDeviceAdressesInfo;
	VertexBufferDeviceAdressesInfo.buffer = vertexBufferData.buffer;

	vk::BufferDeviceAddressInfo IndexDeviceAdressesInfo;
	IndexDeviceAdressesInfo.buffer = indexBufferData.buffer;

	auto vertexBufferAddress = vulkanContext->LogicalDevice.getBufferAddress(VertexBufferDeviceAdressesInfo);
	auto indexBufferAddress = vulkanContext->LogicalDevice.getBufferAddress(IndexDeviceAdressesInfo);


	for (const auto& node : storedModelData->nodes) {

		if (node) {

			const auto& primitives = !node->meshPrimitives.empty() ? node->meshPrimitives :
				std::vector<Primitive>{ Primitive{
					0,
					static_cast<uint32_t>(storedModelData->IndexData.size()),
					0
				} };


			for (const auto& primitive : primitives) {

				BLASDATA  BLAS_Data;
				BLAS_Data.ModelMatrix = node->matrix;

				//Triangle Data
				vk::AccelerationStructureGeometryTrianglesDataKHR BLAS_TriangleData{};
				BLAS_TriangleData.vertexFormat = vk::Format::eR32G32B32Sfloat;
				BLAS_TriangleData.vertexData.deviceAddress = vertexBufferAddress;
				BLAS_TriangleData.vertexStride = sizeof(ModelVertex);
				BLAS_TriangleData.maxVertex = storedModelData->VertexData.size() - 1;
				BLAS_TriangleData.indexData.deviceAddress = indexBufferAddress + (primitive.indicesStart * sizeof(uint32_t));
				BLAS_TriangleData.indexType = vk::IndexType::eUint32;
				BLAS_TriangleData.transformData.deviceAddress = 0;
				BLAS_TriangleData.transformData.hostAddress = nullptr;

				VertexAndIndexOffsets ModelOffset;
				ModelOffset.VertexOffset = m_baseVertexOffset;
				ModelOffset.IndexOffset = m_baseIndexOffset + primitive.indicesStart;
				ModelOffset.MaterialIndex = primitive.materialIndex + GlobalTextureOffset;
				BLAS_Data.GlobalPrimitiveIndex = static_cast<uint32_t>(bufferManager->AllScene_VertexAndIndexOffsets.size());
				bufferManager->AllScene_VertexAndIndexOffsets.push_back(ModelOffset);

				//Geometry Data
				vk::AccelerationStructureGeometryDataKHR AccelerationStructureGeometryData;
				AccelerationStructureGeometryData.triangles = BLAS_TriangleData;


				//Geometry
				vk::AccelerationStructureGeometryKHR AccelerationStructureGeometry;
				AccelerationStructureGeometry.geometryType = vk::GeometryTypeKHR::eTriangles;
				AccelerationStructureGeometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
				AccelerationStructureGeometry.geometry = AccelerationStructureGeometryData;
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////
				uint32_t maxPrimitiveCount = primitive.numIndices / 3;

				vk::AccelerationStructureBuildRangeInfoKHR BuildRangeInfo;
				BuildRangeInfo.firstVertex = 0;
				BuildRangeInfo.primitiveCount = maxPrimitiveCount;
				BuildRangeInfo.primitiveOffset = 0;
				BuildRangeInfo.transformOffset = 0;

				vk::AccelerationStructureBuildGeometryInfoKHR AccelerationStructureBuildGeometryInfo;
				AccelerationStructureBuildGeometryInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
				AccelerationStructureBuildGeometryInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
				AccelerationStructureBuildGeometryInfo.geometryCount = 1;
				AccelerationStructureBuildGeometryInfo.pGeometries = &AccelerationStructureGeometry;
				AccelerationStructureBuildGeometryInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;

				//Temp Holders
				VkAccelerationStructureBuildGeometryInfoKHR TempGI = AccelerationStructureBuildGeometryInfo;
				VkAccelerationStructureBuildSizesInfoKHR TempASBuildSizeInfo;
				TempASBuildSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
				TempASBuildSizeInfo.pNext = nullptr;

				vulkanContext->vkGetAccelerationStructureBuildSizesKHR(vulkanContext->LogicalDevice, VkAccelerationStructureBuildTypeKHR::VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &TempGI, &maxPrimitiveCount, &TempASBuildSizeInfo);
				vk::AccelerationStructureBuildSizesInfoKHR ASBuildSizeInfo = TempASBuildSizeInfo;

				//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				// Create Scratch buffer and BLAS buffer based on the size calculated  
				BLAS_Data.BLAS_ScratchBuffer.BufferID = "Model BLAS_ScratchBuffer Buffer";
				bufferManager->CreateDeviceBuffer(&BLAS_Data.BLAS_ScratchBuffer,
					ASBuildSizeInfo.buildScratchSize,
					vk::BufferUsageFlagBits::eStorageBuffer |
					vk::BufferUsageFlagBits::eShaderDeviceAddress,
					commandPool,
					vulkanContext->graphicsQueue);


				//////////////////////////////////////////////////////////////////////////////////////////////////////////////
				BLAS_Data.BLAS_Buffer.BufferID = "Model bottomLevelASBuffer Buffer";
				bufferManager->CreateDeviceBuffer(&BLAS_Data.BLAS_Buffer,
					ASBuildSizeInfo.accelerationStructureSize,
					vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
					vk::BufferUsageFlagBits::eShaderDeviceAddress,
					commandPool,
					vulkanContext->graphicsQueue);

				vk::BufferDeviceAddressInfo BLAS_ScratchBufferAdress;
				BLAS_ScratchBufferAdress.buffer = BLAS_Data.BLAS_ScratchBuffer.buffer;

				vk::AccelerationStructureCreateInfoKHR AccelerationStructureCreateInfo;
				AccelerationStructureCreateInfo.buffer = BLAS_Data.BLAS_Buffer.buffer;
				AccelerationStructureCreateInfo.offset = 0;
				AccelerationStructureCreateInfo.size = ASBuildSizeInfo.accelerationStructureSize;
				AccelerationStructureCreateInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;

				//Temp hold
				VkAccelerationStructureCreateInfoKHR tempASCI = AccelerationStructureCreateInfo;
				VkAccelerationStructureKHR tempBLAS = BLAS_Data.BLAS;

				vulkanContext->vkCreateAccelerationStructureKHR(vulkanContext->LogicalDevice, &tempASCI, nullptr, &tempBLAS);

				//Reassighn vulkan hpp types
				AccelerationStructureCreateInfo = tempASCI;
				BLAS_Data.BLAS = tempBLAS;


				AccelerationStructureBuildGeometryInfo.dstAccelerationStructure = BLAS_Data.BLAS;
				AccelerationStructureBuildGeometryInfo.scratchData.deviceAddress = vulkanContext->LogicalDevice.getBufferAddress(BLAS_ScratchBufferAdress);
				//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				vk::CommandBuffer cmd = bufferManager->CreateSingleUseCommandBuffer(commandPool);

				VkAccelerationStructureBuildRangeInfoKHR tempRange = BuildRangeInfo;
				std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &tempRange };

				VkAccelerationStructureBuildGeometryInfoKHR tempGeometryInfo = AccelerationStructureBuildGeometryInfo;

				vulkanContext->vkCmdBuildAccelerationStructuresKHR(cmd, 1,
					&tempGeometryInfo,
					accelerationBuildStructureRangeInfos.data());

				bufferManager->SubmitAndDestoyCommandBuffer(commandPool, cmd, vulkanContext->graphicsQueue);

				BLAS_Datas.push_back(BLAS_Data);
			}
		}
	}


}

void Model::CreateUniformBuffer()
{
	{
		vertexUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		VertexUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

		VkDeviceSize VertexuniformBufferSize = sizeof(VertexData);

		for (size_t i = 0; i < vertexUniformBuffers.size(); i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "Model Vertex Uniform Buffer" + i;
			bufferManager->CreateBuffer(&bufferdata,VertexuniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
			vertexUniformBuffers[i] = bufferdata;

			VertexUniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
		}
	}

	{
		Model_GPU_DataUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		Model_GPU_DataUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

		VkDeviceSize ModeluniformBufferSize = sizeof(GPU_InstanceData) * 300;

		for (size_t i = 0; i < Model_GPU_DataUniformBuffers.size(); i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "Model Vertex Uniform Buffer" + i;
			bufferManager->CreateBuffer(&bufferdata, ModeluniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
			Model_GPU_DataUniformBuffers[i] = bufferdata;

			Model_GPU_DataUniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
		}
	}

}

void Model::Instantiate()
{
	vulkanContext->ResetTemporalAccumilation();

	if (!Instances.empty())
	{
		int LastIndex = Instances.size() - 1;

		auto NewInstance = std::make_unique<InstanceData>(Instances[LastIndex].get(), vulkanContext);

		GPU_InstancesData.push_back(NewInstance->gpu_InstanceData);
		Instances.push_back(std::move(NewInstance));
	}
	else
	{
		auto NewInstance = std::make_unique<InstanceData>(nullptr, vulkanContext);
		NewInstance->SetTrasnformationMatrix(glm::mat4(1.0f));

		GPU_InstancesData.push_back(NewInstance->gpu_InstanceData);
		Instances.push_back(std::move(NewInstance));
	}
}

void Model::Destroy(int instanceIndex)
{
	vulkanContext->ResetTemporalAccumilation();

	if (!Instances.empty() && Instances[instanceIndex] && GPU_InstancesData[instanceIndex])
	{
		Instances.erase(Instances.begin() + instanceIndex);
		GPU_InstancesData.erase(GPU_InstancesData.begin() + instanceIndex);
	}
}

void Model::createDescriptorSetLayout()
{
	vk::DescriptorSetLayoutBinding VertexUniformBufferBinding{};
	VertexUniformBufferBinding.binding = 0;
	VertexUniformBufferBinding.descriptorCount = 1;
	VertexUniformBufferBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	VertexUniformBufferBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

	vk::DescriptorSetLayoutBinding ModelUniformBufferBinding{};
	ModelUniformBufferBinding.binding = 1;
	ModelUniformBufferBinding.descriptorCount = 1;
	ModelUniformBufferBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	ModelUniformBufferBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

	vk::DescriptorSetLayoutBinding AlbedoSamplerLayout{};
	AlbedoSamplerLayout.binding = 2;
	AlbedoSamplerLayout.descriptorCount = 1;
	AlbedoSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	AlbedoSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutBinding NormalSamplerLayout{};
	NormalSamplerLayout.binding = 3;
	NormalSamplerLayout.descriptorCount = 1;
	NormalSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	NormalSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutBinding MetallicRoughnessSamplerLayout{};
	MetallicRoughnessSamplerLayout.binding = 4;
	MetallicRoughnessSamplerLayout.descriptorCount = 1;
	MetallicRoughnessSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	MetallicRoughnessSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutBinding AOSamplerLayout{};
	AOSamplerLayout.binding = 5;
	AOSamplerLayout.descriptorCount = 1;
	AOSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	AOSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutBinding EmissiveSamplerLayout{};
	EmissiveSamplerLayout.binding = 6;
	EmissiveSamplerLayout.descriptorCount = 1;
	EmissiveSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	EmissiveSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

	std::array<vk::DescriptorSetLayoutBinding, 7> bindings = { VertexUniformBufferBinding,ModelUniformBufferBinding,
		                                                       AlbedoSamplerLayout,NormalSamplerLayout,
		                                                       MetallicRoughnessSamplerLayout,AOSamplerLayout,EmissiveSamplerLayout
	                                                            };

	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();


	if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess)
	{
		throw std::runtime_error("Failed to create descriptorset layout!");
	}

}


void Model::createDescriptorSets(vk::DescriptorPool descriptorpool)
{


	for (size_t j = 0; j < materialCount; j++)
	{

			std::vector<vk::DescriptorSet> DescriptorSets;

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

				vk::DescriptorBufferInfo vertexbufferInfo{};
				vertexbufferInfo.buffer = vertexUniformBuffers[i].buffer;
				vertexbufferInfo.offset = 0;
				vertexbufferInfo.range = sizeof(VertexData);

				vk::WriteDescriptorSet VertexUniformdescriptorWrite{};
				VertexUniformdescriptorWrite.dstSet = DescriptorSets[i];
				VertexUniformdescriptorWrite.dstBinding = 0;
				VertexUniformdescriptorWrite.dstArrayElement = 0;
				VertexUniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
				VertexUniformdescriptorWrite.descriptorCount = 1;
				VertexUniformdescriptorWrite.pBufferInfo = &vertexbufferInfo;


				vk::DescriptorBufferInfo ModelbufferInfo{};
				ModelbufferInfo.buffer = Model_GPU_DataUniformBuffers[i].buffer;
				ModelbufferInfo.offset = 0;
				ModelbufferInfo.range = sizeof(GPU_InstanceData) * 300;

				vk::WriteDescriptorSet ModelUniformdescriptorWrite{};
				ModelUniformdescriptorWrite.dstSet = DescriptorSets[i];
				ModelUniformdescriptorWrite.dstBinding = 1;
				ModelUniformdescriptorWrite.dstArrayElement = 0;
				ModelUniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
				ModelUniformdescriptorWrite.descriptorCount = 1;
				ModelUniformdescriptorWrite.pBufferInfo = &ModelbufferInfo;

				/////////////////////////////////////////////////////////////////////////////////////
				vk::DescriptorImageInfo imageInfo{};
				imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				imageInfo.imageView   = AlbedoTextures[j].imageView;
				imageInfo.sampler     = AlbedoTextures[j].imageSampler;

				vk::WriteDescriptorSet SamplerdescriptorWrite{};
				SamplerdescriptorWrite.dstSet = DescriptorSets[i];
				SamplerdescriptorWrite.dstBinding = 2;
				SamplerdescriptorWrite.dstArrayElement = 0;
				SamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
				SamplerdescriptorWrite.descriptorCount = 1;
				SamplerdescriptorWrite.pImageInfo = &imageInfo;
				/////////////////////////////////////////////////////////////////////////////////////

				vk::DescriptorImageInfo NormalimageInfo{};
				NormalimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				NormalimageInfo.imageView = NormalTextures[j].imageView;
				NormalimageInfo.sampler = NormalTextures[j].imageSampler;

				vk::WriteDescriptorSet NormalSamplerdescriptorWrite{};
				NormalSamplerdescriptorWrite.dstSet = DescriptorSets[i];
				NormalSamplerdescriptorWrite.dstBinding = 3;
				NormalSamplerdescriptorWrite.dstArrayElement = 0;
				NormalSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
				NormalSamplerdescriptorWrite.descriptorCount = 1;
				NormalSamplerdescriptorWrite.pImageInfo = &NormalimageInfo;
				/////////////////////////////////////////////////////////////////////////////////////

				vk::DescriptorImageInfo MetallicRoughnessimageInfo{};
				MetallicRoughnessimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				MetallicRoughnessimageInfo.imageView = MetallicRoughnessTextures[j].imageView;
				MetallicRoughnessimageInfo.sampler = MetallicRoughnessTextures[j].imageSampler;

				vk::WriteDescriptorSet MetallicRoughnessSamplerdescriptorWrite{};
				MetallicRoughnessSamplerdescriptorWrite.dstSet = DescriptorSets[i];
				MetallicRoughnessSamplerdescriptorWrite.dstBinding = 4;
				MetallicRoughnessSamplerdescriptorWrite.dstArrayElement = 0;
				MetallicRoughnessSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
				MetallicRoughnessSamplerdescriptorWrite.descriptorCount = 1;
				MetallicRoughnessSamplerdescriptorWrite.pImageInfo = &MetallicRoughnessimageInfo;

				vk::DescriptorImageInfo AOimageInfo{};
				AOimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				AOimageInfo.imageView = AOTextures[j].imageView;
				AOimageInfo.sampler = AOTextures[j].imageSampler;

				vk::WriteDescriptorSet AOSamplerdescriptorWrite{};
				AOSamplerdescriptorWrite.dstSet = DescriptorSets[i];
				AOSamplerdescriptorWrite.dstBinding = 5;
				AOSamplerdescriptorWrite.dstArrayElement = 0;
				AOSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
				AOSamplerdescriptorWrite.descriptorCount = 1;
				AOSamplerdescriptorWrite.pImageInfo = &AOimageInfo;


				vk::DescriptorImageInfo EmissiveimageInfo{};
				EmissiveimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				EmissiveimageInfo.imageView   = EmissiveTextures[j].imageView;
				EmissiveimageInfo.sampler     = EmissiveTextures[j].imageSampler;

				vk::WriteDescriptorSet EmissiveSamplerdescriptorWrite{};
				EmissiveSamplerdescriptorWrite.dstSet = DescriptorSets[i];
				EmissiveSamplerdescriptorWrite.dstBinding = 6;
				EmissiveSamplerdescriptorWrite.dstArrayElement = 0;
				EmissiveSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
				EmissiveSamplerdescriptorWrite.descriptorCount = 1;
				EmissiveSamplerdescriptorWrite.pImageInfo = &EmissiveimageInfo;

				/////////////////////////////////////////////////////////////////////////////////////


				std::array<vk::WriteDescriptorSet, 7> descriptorWrites{ VertexUniformdescriptorWrite,ModelUniformdescriptorWrite,SamplerdescriptorWrite,
					                                                    NormalSamplerdescriptorWrite,MetallicRoughnessSamplerdescriptorWrite,
					                                                    AOSamplerdescriptorWrite,EmissiveSamplerdescriptorWrite };

				vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
			
			}


			SceneDescriptorSets.push_back(DescriptorSets);
	}
}

void Model::UpdateUniformBuffer(uint32_t currentImage)
{
	VertexData VertexData;
	VertexData.ViewMatrix            = camera->GetViewMatrix();
	VertexData.Prev_ViewMatrix       = camera->GetPrevViewMatrix();
	VertexData.ProjectionMatrix      = camera->GetProjectionMatrix();
	VertexData.Prev_ProjectionMatrix = camera->GetPrevProjectionMatrix();

	VertexData.Prev_ProjectionMatrix[1][1] *= -1;
	VertexData.ProjectionMatrix[1][1] *= -1;

	memcpy(VertexUniformBuffersMappedMem[currentImage], &VertexData, sizeof(VertexData));

	for (size_t i = 0; i < GPU_InstancesData.size(); i++) {
		GPU_InstanceData* instanceData = GPU_InstancesData[i].get();
		memcpy((char*)Model_GPU_DataUniformBuffersMappedMem[currentImage] + i * sizeof(GPU_InstanceData), instanceData, sizeof(GPU_InstanceData));
	}
}

void Model::UpdateHistory() {

	for (auto& instance : Instances) {
		if (instance) {
			instance->UpdateHistory();
		}
	}

	UpdateNodeHistory(storedModelData->nodes);
}

void Model::UpdateNodeHistory(const std::vector<std::shared_ptr<Node>>& nodes) {
	for (const auto& node : nodes) {
		if (!node) continue;

		node->prevMatrix = node->matrix;

		UpdateNodeHistory(node->children);
	}
}

bool Model::CalcDistanceCulling(glm::mat4 Matrix)
{
	glm::vec3 translation = glm::vec3(Matrix[3]);
	float distance = glm::distance(camera->GetPosition(), translation);

	//return distance <= 100.0f;

	return true;
}


void Model::DrawNode(vk::CommandBuffer commandBuffer,vk::PipelineLayout pipelineLayout, uint32_t imageIndex,const std::vector<std::shared_ptr<Node>>& nodes,const glm::mat4& parentMatrix, const glm::mat4& previousParentMatrix)
{
	for (const auto& node : nodes) {
		
		if (!node) continue;

		glm::mat4 worldMatrix = parentMatrix * node->matrix;
		glm::mat4 previousWorldMatrix = previousParentMatrix * node->prevMatrix;

		Instances[0]->SetModelMatrix(worldMatrix);


		for (int i = 0; i < node->meshPrimitives.size(); i++)
		{
			
			if (node->meshPrimitives[i].numIndices > 0) {
				
				PushConstantData pushData;
				pushData.worldMatrix = worldMatrix;
				pushData.previousWorldMatrix = previousWorldMatrix;

				commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &SceneDescriptorSets[node->meshPrimitives[i].materialIndex][imageIndex], 0, nullptr);
				
				commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstantData), &pushData);
				
				commandBuffer.drawIndexed(node->meshPrimitives[i].numIndices, 1, node->meshPrimitives[i].indicesStart, 0, 0);
			}
		}
			
		

		DrawNode(commandBuffer, pipelineLayout, imageIndex ,node->children, worldMatrix, previousWorldMatrix);
	}
}


void Model::Draw(vk::CommandBuffer commandBuffer,vk::PipelineLayout pipelineLayout,uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer vertexBuffers[] = { vertexBufferData.buffer };

	commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
	commandBuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint32);

	DrawNode(commandBuffer, pipelineLayout, imageIndex,storedModelData->nodes, Instances[0]->GetTransformationMatrix(), Instances[0]->GetPreviousTransformationMatrix());
}

void Model::CleanUp()
{
	if (bufferManager)
	{
		storedModelData = nullptr;

		for (auto& AlbedoTexture : AlbedoTextures)
		{
			bufferManager->DestroyImage(AlbedoTexture);
		}
		AlbedoTextures.clear();

		for (auto& NormalTexture : NormalTextures)
		{
			bufferManager->DestroyImage(NormalTexture);
		}
		NormalTextures.clear();


		for (auto& MetallicRoughnessTexture : MetallicRoughnessTextures)
		{
			bufferManager->DestroyImage(MetallicRoughnessTexture);
		}
		MetallicRoughnessTextures.clear();


		for (auto& AOTexture : AOTextures)
		{
			bufferManager->DestroyImage(AOTexture);
		}

		AOTextures.clear();

		for (auto& EmissiveTexture : EmissiveTextures)
		{
			bufferManager->DestroyImage(EmissiveTexture);
		}

		EmissiveTextures.clear();


		for (auto BLAS_Data : BLAS_Datas)
		{
			bufferManager->DestroyBuffer(BLAS_Data.BLAS_Buffer);
			bufferManager->DestroyBuffer(BLAS_Data.BLAS_ScratchBuffer);
			vulkanContext->vkDestroyAccelerationStructureKHR(vulkanContext->LogicalDevice, static_cast<VkAccelerationStructureKHR>(BLAS_Data.BLAS), nullptr);

		}
	  
		for (auto& uniformBuffer : Model_GPU_DataUniformBuffers)
		{
			if (uniformBuffer.buffer)
			{
				bufferManager->UnmapMemory(uniformBuffer);
				bufferManager->DestroyBuffer(uniformBuffer);
			}
		}


		SceneDescriptorSets.clear();
		Model_GPU_DataUniformBuffers.clear();
		Model_GPU_DataUniformBuffersMappedMem.clear();
		Instances.clear();
		GPU_InstancesData.clear();
	}
	
	vulkanContext->LogicalDevice.destroyDescriptorSetLayout(RayTracingDescriptorSetLayout);

	Drawable::Destructor();
}



