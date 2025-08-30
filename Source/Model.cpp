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
	CreateBLAS();
	LoadTextures();
	CreateUniformBuffer();
	createDescriptorSetLayout();
	Instantiate();
}

void Model::LoadTextures()
{

	std::vector<StoredImageData> ModelTextures = AssetManager::GetInstance().GetStoredImageData(FilePath);

    materialCount = ModelTextures.size() / 4;

	for (size_t i = 0; i < materialCount; i++) {


		ImageData  albedoTextureData;

		StoredImageData AlbedoImageData = ModelTextures[i * 4 + 0];
		vk::DeviceSize AlbedoImagesize = AlbedoImageData.imageWidth * AlbedoImageData.imageHeight * 4;

		albedoTextureData = bufferManager->CreateTextureImage(AlbedoImageData.imageData, AlbedoImagesize, AlbedoImageData.imageWidth, AlbedoImageData.imageHeight, vk::Format::eR8G8B8A8Srgb, commandPool, vulkanContext->graphicsQueue);

		AlbedoTextures.push_back(albedoTextureData);


		ImageData  normalTextureData;

		StoredImageData NormalImageData = ModelTextures[i * 4 + 1];
		vk::DeviceSize NormalImagesize = NormalImageData.imageWidth * NormalImageData.imageHeight * 4;

		normalTextureData = bufferManager->CreateTextureImage(NormalImageData.imageData, NormalImagesize, NormalImageData.imageWidth, NormalImageData.imageHeight, vk::Format::eR8G8B8A8Unorm, commandPool, vulkanContext->graphicsQueue);

		NormalTextures.push_back(normalTextureData);


		ImageData  MetallicRoughnessTextureData;

		StoredImageData MetallicRoughnessImageData = ModelTextures[i * 4 + 2];
		vk::DeviceSize  MetallicRoughnessImagesize = MetallicRoughnessImageData.imageWidth * MetallicRoughnessImageData.imageHeight * 4;

		MetallicRoughnessTextureData = bufferManager->CreateTextureImage(MetallicRoughnessImageData.imageData, MetallicRoughnessImagesize, MetallicRoughnessImageData.imageWidth, MetallicRoughnessImageData.imageHeight, vk::Format::eR8G8B8A8Unorm, commandPool, vulkanContext->graphicsQueue);

		MetallicRoughnessTextures.push_back(MetallicRoughnessTextureData);


		ImageData  AOTextureData;

		StoredImageData AOImageData = ModelTextures[i * 4 + 3];
		vk::DeviceSize  AOImagesize = AOImageData.imageWidth * AOImageData.imageHeight * 4;

		AOTextureData = bufferManager->CreateTextureImage(AOImageData.imageData, AOImagesize, AOImageData.imageWidth, AOImageData.imageHeight, vk::Format::eR8G8B8A8Unorm, commandPool, vulkanContext->graphicsQueue);

		AOTextures.push_back(AOTextureData);
	}






}

void Model::CreateVertexAndIndexBuffer()
{

	storedModelData = &AssetManager::GetInstance().GetStoredModelData(FilePath);


	VkDeviceSize VertexBufferSize = sizeof(storedModelData->VertexData[0]) * storedModelData->VertexData.size();
	vertexBufferData.BufferID = "Model Vertex Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&vertexBufferData,storedModelData->VertexData.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(uint32_t) * storedModelData->IndexData.size();
	indexBufferData.BufferID = "Model Index Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&indexBufferData,storedModelData->IndexData.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, commandPool, vulkanContext->graphicsQueue);

}


void Model::CreateBLAS()
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	vk::BufferDeviceAddressInfo VertexBufferDeviceAdressesInfo;
	VertexBufferDeviceAdressesInfo.buffer = vertexBufferData.buffer;

	vk::BufferDeviceAddressInfo IndexDeviceAdressesInfo;
	IndexDeviceAdressesInfo.buffer = indexBufferData.buffer;

	//Add an offset so we always point to the address of the verticies
	auto vertexBufferAddress = vulkanContext->LogicalDevice.getBufferAddress(VertexBufferDeviceAdressesInfo);
	auto indexBufferAddress  = vulkanContext->LogicalDevice.getBufferAddress(IndexDeviceAdressesInfo);

	//Triangle Data
	vk::AccelerationStructureGeometryTrianglesDataKHR BLAS_TriangleData {};
	BLAS_TriangleData.vertexFormat = vk::Format::eR32G32B32Sfloat;
	BLAS_TriangleData.vertexData.deviceAddress = vertexBufferAddress;
	BLAS_TriangleData.vertexStride = sizeof(ModelVertex);
	BLAS_TriangleData.maxVertex    = storedModelData->VertexData.size() - 1;
	BLAS_TriangleData.indexData.deviceAddress = indexBufferAddress;
	BLAS_TriangleData.indexType = vk::IndexType::eUint32;
	BLAS_TriangleData.transformData.deviceAddress = 0;
	BLAS_TriangleData.transformData.hostAddress = nullptr;

	//Geometry Data
	vk::AccelerationStructureGeometryDataKHR AccelerationStructureGeometryData; 
	AccelerationStructureGeometryData.triangles = BLAS_TriangleData;
	
	//Geometry
	vk::AccelerationStructureGeometryKHR AccelerationStructureGeometry;
	AccelerationStructureGeometry.geometryType = vk::GeometryTypeKHR::eTriangles;
	AccelerationStructureGeometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
	AccelerationStructureGeometry.geometry = AccelerationStructureGeometryData;
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////


	uint32_t maxPrimitiveCount = storedModelData->IndexData.size() / 3;

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
	// 
	// 
	// Create Scratch buffer and BLAS buffer based on the size calculated  
	BLAS_ScratchBuffer.BufferID = "Model BLAS_ScratchBuffer Buffer";
	bufferManager->CreateDeviceBuffer(&BLAS_ScratchBuffer,
		                               ASBuildSizeInfo.buildScratchSize,
		                               vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
		                               vk::BufferUsageFlagBits::eShaderDeviceAddress,
		                               commandPool,
		                               vulkanContext->graphicsQueue);


	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	BLAS_Buffer.BufferID = "Model bottomLevelASBuffer Buffer";
	bufferManager->CreateDeviceBuffer(&BLAS_Buffer,
		ASBuildSizeInfo.accelerationStructureSize,
		vk::BufferUsageFlagBits::eStorageBuffer |
		vk::BufferUsageFlagBits::eShaderDeviceAddress,
		commandPool,
		vulkanContext->graphicsQueue);

	vk::BufferDeviceAddressInfo BLAS_ScratchBufferAdress;
	BLAS_ScratchBufferAdress.buffer = BLAS_ScratchBuffer.buffer;


	vk::AccelerationStructureCreateInfoKHR AccelerationStructureCreateInfo;
	AccelerationStructureCreateInfo.buffer = BLAS_Buffer.buffer;
	AccelerationStructureCreateInfo.offset = 0;
	AccelerationStructureCreateInfo.size = ASBuildSizeInfo.accelerationStructureSize;
	AccelerationStructureCreateInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;

	//Temp hold
	VkAccelerationStructureCreateInfoKHR tempASCI = AccelerationStructureCreateInfo;
	VkAccelerationStructureKHR tempBLAS = BLAS;

	vulkanContext->vkCreateAccelerationStructureKHR(vulkanContext->LogicalDevice, &tempASCI, nullptr, &tempBLAS);

	//Reassighn vulkan hpp types
	AccelerationStructureCreateInfo = tempASCI;
	BLAS = tempBLAS;


	AccelerationStructureBuildGeometryInfo.dstAccelerationStructure  = BLAS;
	AccelerationStructureBuildGeometryInfo.scratchData.deviceAddress = vulkanContext->LogicalDevice.getBufferAddress(BLAS_ScratchBufferAdress);
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	vk::CommandBuffer cmd =  bufferManager->CreateSingleUseCommandBuffer(commandPool);

	VkAccelerationStructureBuildRangeInfoKHR tempRange = BuildRangeInfo;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &tempRange };

	VkAccelerationStructureBuildGeometryInfoKHR tempGeometryInfo = AccelerationStructureBuildGeometryInfo;

	vulkanContext->vkCmdBuildAccelerationStructuresKHR(cmd, 1,
		                                               &tempGeometryInfo, 
		                                               accelerationBuildStructureRangeInfos.data());

	bufferManager->SubmitAndDestoyCommandBuffer(commandPool, cmd, vulkanContext->graphicsQueue);
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
			bufferdata.BufferID = "Vertex Uniform Buffer" + i;
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

		InstanceData* NewInstance = new InstanceData(Instances[LastIndex],vulkanContext);

		Instances.push_back(NewInstance);
		GPU_InstancesData.push_back(NewInstance->gpu_InstanceData);

	}
	else
	{
		InstanceData* NewInstance = new InstanceData(nullptr, vulkanContext);
		NewInstance->SetModelMatrix(storedModelData->modelMatrix);

		Instances.push_back(NewInstance);
		GPU_InstancesData.push_back(NewInstance->gpu_InstanceData);
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


	std::array<vk::DescriptorSetLayoutBinding, 6> bindings = { VertexUniformBufferBinding,
																ModelUniformBufferBinding,AlbedoSamplerLayout,NormalSamplerLayout,MetallicRoughnessSamplerLayout,AOSamplerLayout
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

				/////////////////////////////////////////////////////////////////////////////////////


				std::array<vk::WriteDescriptorSet, 6> descriptorWrites{ VertexUniformdescriptorWrite,
																		ModelUniformdescriptorWrite,SamplerdescriptorWrite,NormalSamplerdescriptorWrite,MetallicRoughnessSamplerdescriptorWrite,AOSamplerdescriptorWrite };

				vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
			
			}


			SceneDescriptorSets.push_back(DescriptorSets);
	}
}

void Model::UpdateUniformBuffer(uint32_t currentImage)
{
	VertexData VertexData;
	VertexData.ViewMatrix = camera->GetViewMatrix();
	VertexData.ProjectionMatrix = camera->GetProjectionMatrix();
	VertexData.ProjectionMatrix[1][1] *= -1;

	memcpy(VertexUniformBuffersMappedMem[currentImage], &VertexData, sizeof(VertexData));

	for (size_t i = 0; i < GPU_InstancesData.size(); i++) {
		GPU_InstanceData* instanceData = GPU_InstancesData[i].get();
		memcpy((char*)Model_GPU_DataUniformBuffersMappedMem[currentImage] + i * sizeof(GPU_InstanceData), instanceData, sizeof(GPU_InstanceData));
	}
}


void Model::DrawNode(vk::CommandBuffer commandBuffer,vk::PipelineLayout pipelineLayout, uint32_t imageIndex,const std::vector<std::shared_ptr<Node>>& nodes,const glm::mat4& parentMatrix)
{
	for (const auto& node : nodes) {
		if (!node) continue;

		glm::mat4 worldMatrix = parentMatrix * node->matrix;

		for (const auto& primitive : node->meshPrimitives) {

			if (primitive.numIndices > 0) {

				commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &SceneDescriptorSets[primitive.materialIndex][imageIndex], 0, nullptr);

				commandBuffer.pushConstants(pipelineLayout,vk::ShaderStageFlagBits::eVertex,0,sizeof(glm::mat4),&worldMatrix);

				commandBuffer.drawIndexed(primitive.numIndices,1,primitive.indicesStart,0,0);
			}
		}

		DrawNode(commandBuffer, pipelineLayout, imageIndex,node->children, worldMatrix);
	}
}

void Model::Draw(vk::CommandBuffer commandBuffer,vk::PipelineLayout pipelineLayout,uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer vertexBuffers[] = { vertexBufferData.buffer };

	commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
	commandBuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint32);

	DrawNode(commandBuffer, pipelineLayout, imageIndex,storedModelData->nodes, glm::mat4(1.0f));
}

void Model::CleanUp()
{
	if (bufferManager)
	{
		storedModelData = nullptr;
		//bufferManager->DestroyImage(albedoTextureData);
		//bufferManager->DestroyImage(normalTextureData);
		//bufferManager->DestroyImage(MetallicRoughnessTextureData);

	    bufferManager->DestroyBuffer(BLAS_Buffer);
		bufferManager->DestroyBuffer(BLAS_ScratchBuffer);

		vulkanContext->vkDestroyAccelerationStructureKHR(vulkanContext->LogicalDevice, static_cast<VkAccelerationStructureKHR>(BLAS), nullptr);

		for (auto& uniformBuffer : Model_GPU_DataUniformBuffers)
		{
			if (uniformBuffer.buffer)
			{
				bufferManager->UnmapMemory(uniformBuffer);
				bufferManager->DestroyBuffer(uniformBuffer);
			}
		}

		Model_GPU_DataUniformBuffers.clear();
		Model_GPU_DataUniformBuffersMappedMem.clear();
		Instances.clear();
		GPU_InstancesData.clear();
	}
	
	vulkanContext->LogicalDevice.destroyDescriptorSetLayout(RayTracingDescriptorSetLayout);

	Drawable::Destructor();
}



