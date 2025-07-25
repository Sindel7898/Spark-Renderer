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

	position = glm::vec3(1.0f,1.0f,1.0f);
	rotation = glm::vec3(90.0f,0.0f,0.0f);
	scale =    glm::vec3(2.0f, 2.0f, 2.0f);

	transformMatrices.modelMatrix = glm::mat4(1.0f);
	transformMatrices.modelMatrix = glm::translate(transformMatrices.modelMatrix, position);
	transformMatrices.modelMatrix = glm::rotate(transformMatrices.modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	transformMatrices.modelMatrix = glm::rotate(transformMatrices.modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	transformMatrices.modelMatrix = glm::rotate(transformMatrices.modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	transformMatrices.modelMatrix = glm::scale(transformMatrices.modelMatrix, scale);

	CreateVertexAndIndexBuffer();
	CreateBLAS();
	LoadTextures();
	CreateUniformBuffer();
	createDescriptorSetLayout();
}

void Model::LoadTextures()
{

	std::vector<StoredImageData> ModelTextures = AssetManager::GetInstance().GetStoredImageData(FilePath);


	StoredImageData AlbedoImageData = ModelTextures[0];
	vk::DeviceSize AlbedoImagesize = AlbedoImageData.imageWidth * AlbedoImageData.imageHeight * 4;

	albedoTextureData = bufferManager->CreateTextureImage(AlbedoImageData.imageData, AlbedoImagesize, AlbedoImageData.imageWidth, AlbedoImageData.imageHeight,vk::Format::eR8G8B8A8Srgb , commandPool, vulkanContext->graphicsQueue);

	StoredImageData NormalImageData = ModelTextures[1];
	vk::DeviceSize NormalImagesize = NormalImageData.imageWidth * NormalImageData.imageHeight * 4;

	normalTextureData = bufferManager->CreateTextureImage(NormalImageData.imageData, NormalImagesize, NormalImageData.imageWidth, NormalImageData.imageHeight, vk::Format::eR8G8B8A8Unorm, commandPool, vulkanContext->graphicsQueue);


	StoredImageData MetallicRoughnessImageData = ModelTextures[2];
	vk::DeviceSize  MetallicRoughnessImagesize = MetallicRoughnessImageData.imageWidth * MetallicRoughnessImageData.imageHeight * 4;

	MetallicRoughnessTextureData = bufferManager->CreateTextureImage(MetallicRoughnessImageData.imageData, MetallicRoughnessImagesize, MetallicRoughnessImageData.imageWidth, MetallicRoughnessImageData.imageHeight, vk::Format::eR8G8B8A8Unorm, commandPool, vulkanContext->graphicsQueue);


}

void Model::CreateVertexAndIndexBuffer()
{

	storedModelData = &AssetManager::GetInstance().GetStoredModelData(FilePath);

	transformMatrices.modelMatrix = storedModelData->modelMatrix;

	VkDeviceSize VertexBufferSize = sizeof(storedModelData->VertexData[0]) * storedModelData->VertexData.size();
	vertexBufferData.BufferID = "Model Vertex Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&vertexBufferData,storedModelData->VertexData.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(uint32_t) * storedModelData->IndexData.size();
	indexBufferData.BufferID = "Model Index Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&indexBufferData,storedModelData->IndexData.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, commandPool, vulkanContext->graphicsQueue);

}


void Model::CreateBLAS()
{
	vk::BufferDeviceAddressInfo VertexBufferDeviceAdressesInfo;
	VertexBufferDeviceAdressesInfo.buffer = vertexBufferData.buffer;

	vk::BufferDeviceAddressInfo IndexDeviceAdressesInfo;
	IndexDeviceAdressesInfo.buffer = indexBufferData.buffer;

	//Add an offset so we always point to the address of the verticies
	auto verticiesAddress = vulkanContext->LogicalDevice.getBufferAddress(VertexBufferDeviceAdressesInfo);
	
	//Triangle Data
	vk::AccelerationStructureGeometryTrianglesDataKHR BLAS_TriangleData {};
	BLAS_TriangleData.vertexFormat = vk::Format::eR32G32B32Sfloat;
	BLAS_TriangleData.vertexData.deviceAddress = verticiesAddress;
	BLAS_TriangleData.vertexStride = sizeof(ModelVertex);
	BLAS_TriangleData.maxVertex    = storedModelData->VertexData.size() - 1;
	BLAS_TriangleData.indexData.deviceAddress = vulkanContext->LogicalDevice.getBufferAddress(IndexDeviceAdressesInfo);
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

	// Create Scratch buffer and BLAS buffer based on the size calculated  
	BLAS_ScratchBuffer.BufferID = "Model BLAS_ScratchBuffer Buffer";
	bufferManager->CreateDeviceBuffer(&BLAS_ScratchBuffer,
		                               ASBuildSizeInfo.buildScratchSize,
		                               vk::BufferUsageFlagBits::eStorageBuffer|
		                               vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
		                               vk::BufferUsageFlagBits::eShaderDeviceAddress,
		                               commandPool,
		                               vulkanContext->graphicsQueue);

	BLAS_Buffer.BufferID = "Model bottomLevelASBuffer Buffer";
	bufferManager->CreateDeviceBuffer(&BLAS_Buffer,
		                              ASBuildSizeInfo.accelerationStructureSize, 
		                              vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
		                              vk::BufferUsageFlagBits::eShaderDeviceAddress, 
		                              commandPool,
		                              vulkanContext->graphicsQueue);


	
	vk::AccelerationStructureCreateInfoKHR AccelerationStructureCreateInfo;
	AccelerationStructureCreateInfo.buffer = BLAS_Buffer.buffer;
	AccelerationStructureCreateInfo.offset = 0;
	AccelerationStructureCreateInfo.size = ASBuildSizeInfo.accelerationStructureSize;
	AccelerationStructureCreateInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;

	//Temp hold
	VkAccelerationStructureCreateInfoKHR tempASCI = AccelerationStructureCreateInfo;
	VkAccelerationStructureKHR tempBLAS           = BLAS;

	vulkanContext->vkCreateAccelerationStructureKHR(vulkanContext->LogicalDevice,&tempASCI,nullptr, &tempBLAS);

	//Reassighn vulkan hpp types
	AccelerationStructureCreateInfo = tempASCI;
	BLAS = tempBLAS;


	vk::BufferDeviceAddressInfo BLAS_ScratchBufferAdress;
	BLAS_ScratchBufferAdress.buffer = BLAS_ScratchBuffer.buffer;


	AccelerationStructureBuildGeometryInfo.dstAccelerationStructure  = BLAS;
	AccelerationStructureBuildGeometryInfo.scratchData.deviceAddress = vulkanContext->LogicalDevice.getBufferAddress(BLAS_ScratchBufferAdress);

	vk::CommandBuffer cmd =  bufferManager->CreateSingleUseCommandBuffer(commandPool);

	VkAccelerationStructureBuildRangeInfoKHR tempRange = BuildRangeInfo;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &tempRange };

	VkAccelerationStructureBuildGeometryInfoKHR tempGeometryInfo = AccelerationStructureBuildGeometryInfo;

	vulkanContext->vkCmdBuildAccelerationStructuresKHR(cmd, 1,
		                                               &tempGeometryInfo, 
		                                               accelerationBuildStructureRangeInfos.data());

	bufferManager->SubmitAndDestoyCommandBuffer(commandPool, cmd, vulkanContext->graphicsQueue);
}

void Model::CreateUniformBuffer()
{
	{
		vertexUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		VertexUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

		VkDeviceSize VertexuniformBufferSize = sizeof(ModelData);

		for (size_t i = 0; i < vertexUniformBuffers.size(); i++)
		{
			BufferData bufferdata;
			bufferdata.BufferID = "Model Vertex Uniform Buffer" + i;
			bufferManager->CreateBuffer(&bufferdata,VertexuniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
			vertexUniformBuffers[i] = bufferdata;

			VertexUniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
		}
	}
}



void Model::createDescriptorSetLayout()
{
	vk::DescriptorSetLayoutBinding VertexUniformBufferBinding{};
	VertexUniformBufferBinding.binding = 0;
	VertexUniformBufferBinding.descriptorCount = 1;
	VertexUniformBufferBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	VertexUniformBufferBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

	vk::DescriptorSetLayoutBinding AlbedoSamplerLayout{};
	AlbedoSamplerLayout.binding = 1;
	AlbedoSamplerLayout.descriptorCount = 1;
	AlbedoSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	AlbedoSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutBinding NormalSamplerLayout{};
	NormalSamplerLayout.binding = 2;
	NormalSamplerLayout.descriptorCount = 1;
	NormalSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	NormalSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutBinding MetallicRoughnessSamplerLayout{};
	MetallicRoughnessSamplerLayout.binding = 3;
	MetallicRoughnessSamplerLayout.descriptorCount = 1;
	MetallicRoughnessSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	MetallicRoughnessSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

	std::array<vk::DescriptorSetLayoutBinding, 4> bindings = { VertexUniformBufferBinding,
															   AlbedoSamplerLayout,NormalSamplerLayout,MetallicRoughnessSamplerLayout,
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
	     
	     	vk::DescriptorBufferInfo vertexbufferInfo{};
	     	vertexbufferInfo.buffer = vertexUniformBuffers[i].buffer;
	     	vertexbufferInfo.offset = 0;
	     	vertexbufferInfo.range = sizeof(ModelData);
	     
	     	vk::WriteDescriptorSet VertexUniformdescriptorWrite{};
	     	VertexUniformdescriptorWrite.dstSet = DescriptorSets[i];
	     	VertexUniformdescriptorWrite.dstBinding = 0;
	     	VertexUniformdescriptorWrite.dstArrayElement = 0;
	     	VertexUniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
	     	VertexUniformdescriptorWrite.descriptorCount = 1;
	     	VertexUniformdescriptorWrite.pBufferInfo = &vertexbufferInfo;
	     	;
	     	/////////////////////////////////////////////////////////////////////////////////////
	     	vk::DescriptorImageInfo imageInfo{};
	     	imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	     	imageInfo.imageView = albedoTextureData.imageView;
	     	imageInfo.sampler = albedoTextureData.imageSampler;
	     
	     	vk::WriteDescriptorSet SamplerdescriptorWrite{};
	     	SamplerdescriptorWrite.dstSet = DescriptorSets[i];
	     	SamplerdescriptorWrite.dstBinding = 1;
	     	SamplerdescriptorWrite.dstArrayElement = 0;
	     	SamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	     	SamplerdescriptorWrite.descriptorCount = 1;
	     	SamplerdescriptorWrite.pImageInfo = &imageInfo;
	     	/////////////////////////////////////////////////////////////////////////////////////
	     
	     	vk::DescriptorImageInfo NormalimageInfo{};
	     	NormalimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	     	NormalimageInfo.imageView = normalTextureData.imageView;
	     	NormalimageInfo.sampler = normalTextureData.imageSampler;
	     
	     	vk::WriteDescriptorSet NormalSamplerdescriptorWrite{};
	     	NormalSamplerdescriptorWrite.dstSet = DescriptorSets[i];
	     	NormalSamplerdescriptorWrite.dstBinding = 2;
	     	NormalSamplerdescriptorWrite.dstArrayElement = 0;
	     	NormalSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	     	NormalSamplerdescriptorWrite.descriptorCount = 1;
	     	NormalSamplerdescriptorWrite.pImageInfo = &NormalimageInfo;
	     	/////////////////////////////////////////////////////////////////////////////////////
	     
			vk::DescriptorImageInfo MetallicRoughnessimageInfo{};
			MetallicRoughnessimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			MetallicRoughnessimageInfo.imageView = MetallicRoughnessTextureData.imageView;
			MetallicRoughnessimageInfo.sampler   = MetallicRoughnessTextureData.imageSampler;

			vk::WriteDescriptorSet MetallicRoughnessSamplerdescriptorWrite{};
			MetallicRoughnessSamplerdescriptorWrite.dstSet = DescriptorSets[i];
			MetallicRoughnessSamplerdescriptorWrite.dstBinding = 3;
			MetallicRoughnessSamplerdescriptorWrite.dstArrayElement = 0;
			MetallicRoughnessSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			MetallicRoughnessSamplerdescriptorWrite.descriptorCount = 1;
			MetallicRoughnessSamplerdescriptorWrite.pImageInfo = &MetallicRoughnessimageInfo;

			/////////////////////////////////////////////////////////////////////////////////////


	     	std::array<vk::WriteDescriptorSet, 4> descriptorWrites{ VertexUniformdescriptorWrite,
	     															SamplerdescriptorWrite,NormalSamplerdescriptorWrite,MetallicRoughnessSamplerdescriptorWrite };
	     
	     	vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	     }
	}

}


void Model::UpdateUniformBuffer(uint32_t currentImage)
{
	Drawable::UpdateUniformBuffer(currentImage);
	
	transformMatrices.viewMatrix = camera->GetViewMatrix();
	transformMatrices.projectionMatrix = camera->GetProjectionMatrix();
	transformMatrices.projectionMatrix[1][1] *= -1;
	
	ModelData modelData;
	modelData.transformMatrices = transformMatrices;
	modelData.bCubeMapReflection_bScreenSpaceReflectionWithPadding = glm::vec4(bCubeMapReflection, bScreenSpaceReflection, 0.0f, 0.0f);

	memcpy(VertexUniformBuffersMappedMem[currentImage], &modelData, sizeof(modelData));
}


void Model::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };
	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint32);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(storedModelData->IndexData.size(), 1, 0, 0, 0);
}

void Model::CubeMapReflectiveSwitch(bool breflective)
{
	if (breflective)
	{
		bCubeMapReflection = 1;
	}
	else {
		bCubeMapReflection = 0;
	}
}

void Model::ScreenSpaceReflectiveSwitch(bool breflective)
{
	if (breflective)
	{
		bScreenSpaceReflection = 1;
	}
	else {
		bScreenSpaceReflection = 0;
	}
}

void Model::CleanUp()
{
	if (bufferManager)
	{
		storedModelData = nullptr;
		bufferManager->DestroyImage(albedoTextureData);
		bufferManager->DestroyImage(normalTextureData);
		bufferManager->DestroyImage(MetallicRoughnessTextureData);

	    bufferManager->DestroyBuffer(BLAS_Buffer);
		bufferManager->DestroyBuffer(BLAS_ScratchBuffer);

		vulkanContext->vkDestroyAccelerationStructureKHR(vulkanContext->LogicalDevice, static_cast<VkAccelerationStructureKHR>(BLAS), nullptr);
	}
	
	Drawable::Destructor();
}



