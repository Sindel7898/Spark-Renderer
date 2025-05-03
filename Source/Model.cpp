#include "Model.h"
#include <stdexcept>
#include <chrono>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

Model::Model(const std::string filepath, std::string modelindex, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger)
	      : Drawable()
{
	FilePath = filepath;
	ModelIndex = modelindex;
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


	//AssetManager::GetInstance().meshloader->LoadModel(filepath);

	LoadTextures();
	CreateVertexAndIndexBuffer();
	CreateUniformBuffer();
	createDescriptorSetLayout();
	//CreateBottomLevelAccelerationStructure();
}

void Model::LoadTextures()
{
	std::vector<StoredImageData> ModelTextures = AssetManager::GetInstance().GetStoredImageData(FilePath);

	StoredImageData AlbedoImageData = ModelTextures[0];
	albedoTextureData = bufferManager->CreateTextureImage(AlbedoImageData.imageData, AlbedoImageData.imageWidth, AlbedoImageData.imageHeight,vk::Format::eR8G8B8A8Srgb , commandPool, vulkanContext->graphicsQueue);

	StoredImageData NormalImageData = ModelTextures[1];
	normalTextureData = bufferManager->CreateTextureImage(NormalImageData.imageData, NormalImageData.imageWidth, NormalImageData.imageHeight, vk::Format::eR8G8B8A8Unorm, commandPool, vulkanContext->graphicsQueue);


}

void Model::CreateVertexAndIndexBuffer()
{

	storedModelData = AssetManager::GetInstance().GetStoredModelData(ModelIndex);

	transformMatrices.modelMatrix = storedModelData.modelMatrix;

	VkDeviceSize VertexBufferSize = sizeof(storedModelData.VertexData[0]) * storedModelData.VertexData.size();
	vertexBufferData = bufferManager->CreateGPUOptimisedBuffer(storedModelData.VertexData.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(uint32_t) * storedModelData.IndexData.size();
	indexBufferData = bufferManager->CreateGPUOptimisedBuffer(storedModelData.IndexData.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer, commandPool, vulkanContext->graphicsQueue);

}

//void Model::CreateBottomLevelAccelerationStructure()
//{		
//	vk::BufferDeviceAddressInfoKHR vertexBufferAddressInfo{};
//	vertexBufferAddressInfo.buffer = vertexBufferData.buffer;
//	vk::DeviceAddress vertexAddress = vulkanContext->LogicalDevice.getBufferAddress(&vertexBufferAddressInfo);
//
//
//	vk::BufferDeviceAddressInfoKHR IndexBufferAddressInfo{};
//	IndexBufferAddressInfo.buffer = indexBufferData.buffer;
//	vk::DeviceAddress  indexAddress = vulkanContext->LogicalDevice.getBufferAddress(&IndexBufferAddressInfo);
//
//	uint32_t numTriangles = static_cast<uint32_t>(storedModelData.IndexData.size()) / 3;
//
//	vk::AccelerationStructureGeometryKHR accelerationStructureGeometry{};
//	accelerationStructureGeometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
//	accelerationStructureGeometry.geometryType = vk::GeometryTypeKHR::eTriangles;
//	accelerationStructureGeometry.geometry.triangles.sType = vk::StructureType::eAccelerationStructureGeometryTrianglesDataKHR;
//	accelerationStructureGeometry.geometry.triangles.vertexFormat = vk::Format::eR32G32B32Sfloat;
//	accelerationStructureGeometry.geometry.triangles.vertexData = vertexAddress;
//	accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(storedModelData.VertexData[0]);
//	accelerationStructureGeometry.geometry.triangles.maxVertex = static_cast<uint32_t>(storedModelData.VertexData.size()) - 1;
//	accelerationStructureGeometry.geometry.triangles.indexType = vk::IndexType::eUint32;
//	accelerationStructureGeometry.geometry.triangles.indexData = indexAddress;
//	accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
//
//	vk::AccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
//	accelerationStructureBuildGeometryInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
//	accelerationStructureBuildGeometryInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
//	accelerationStructureBuildGeometryInfo.geometryCount = 1;
//	accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
//
//	vk::AccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfoKHR;
//	
//	vulkanContext->vkGetAccelerationStructureBuildSizesKHR(vulkanContext->LogicalDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
//		                                                  (VkAccelerationStructureBuildGeometryInfoKHR*)&accelerationStructureBuildGeometryInfo,
//		                                                  &numTriangles,
//		                                                  (VkAccelerationStructureBuildSizesInfoKHR*)&accelerationStructureBuildSizesInfoKHR);
//
//
//	bottomLevelASBuffer = bufferManager->CreateBuffer(accelerationStructureBuildSizesInfoKHR.accelerationStructureSize,
//		                                                        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | 
//		                                                        vk::BufferUsageFlagBits::eShaderDeviceAddress, 
//		                                                        commandPool, vulkanContext->graphicsQueue);
//
//	vk::AccelerationStructureCreateInfoKHR  accelerationStructureCreate_info{};
//	accelerationStructureCreate_info.buffer = bottomLevelASBuffer.buffer;
//	accelerationStructureCreate_info.size   = bottomLevelASBuffer.size;
//	accelerationStructureCreate_info.type   = vk::AccelerationStructureTypeKHR::eBottomLevel;
//
//    vulkanContext->vkCreateAccelerationStructureKHR(vulkanContext->LogicalDevice, (VkAccelerationStructureCreateInfoKHR*)&accelerationStructureCreate_info, nullptr, (VkAccelerationStructureKHR*)&bottomLevelAS);
//	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//	scratchBuffer = bufferManager->CreateBuffer(accelerationStructureBuildSizesInfoKHR.buildScratchSize,
//		vk::BufferUsageFlagBits::eStorageBuffer |
//		vk::BufferUsageFlagBits::eShaderDeviceAddress,
//		commandPool, vulkanContext->graphicsQueue);
//
//	vk::BufferDeviceAddressInfoKHR StorageBufferAddressInfo{};
//	StorageBufferAddressInfo.buffer = scratchBuffer.buffer;
//
//	vk::DeviceAddress  StorageAddress = vulkanContext->LogicalDevice.getBufferAddress(&StorageBufferAddressInfo);
//
//	vk::AccelerationStructureBuildGeometryInfoKHR acceleraitonBuildGeometryInfo{};
//	acceleraitonBuildGeometryInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
//	acceleraitonBuildGeometryInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
//	acceleraitonBuildGeometryInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
//	acceleraitonBuildGeometryInfo.dstAccelerationStructure = bottomLevelAS;
//	acceleraitonBuildGeometryInfo.geometryCount = 1;
//	acceleraitonBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
//	acceleraitonBuildGeometryInfo.scratchData.deviceAddress = StorageAddress;
//
//	vk::AccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
//	accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
//	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
//	accelerationStructureBuildRangeInfo.firstVertex = 0;
//	accelerationStructureBuildRangeInfo.transformOffset = 0;
//
//	std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };
//
//	vk::CommandBuffer commandbuffer =   bufferManager->CreateSingleUseCommandBuffer(commandPool);
//
//	vulkanContext->vkCmdBuildAccelerationStructuresKHR(
//		commandbuffer,1,
//		reinterpret_cast<const VkAccelerationStructureBuildGeometryInfoKHR*>(&acceleraitonBuildGeometryInfo),
//		reinterpret_cast<const VkAccelerationStructureBuildRangeInfoKHR* const*>(accelerationBuildStructureRangeInfos.data()));
//
//
//	bufferManager->SubmitAndDestoyCommandBuffer(commandPool, commandbuffer, vulkanContext->graphicsQueue);
//}

void Model::CreateUniformBuffer()
{
	vertexUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	VertexUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

	VkDeviceSize VertexuniformBufferSize = sizeof(TransformMatrices);

	for (size_t i = 0; i < vertexUniformBuffers.size(); i++)
	{

		BufferData bufferdata = bufferManager->CreateBuffer(VertexuniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
		vertexUniformBuffers[i] = bufferdata;

		VertexUniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
	}

	//////////////////////////////////////////////////////////////
	fragmentUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	FragmentUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);
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


	std::array<vk::DescriptorSetLayoutBinding, 3> bindings = { VertexUniformBufferBinding,
															   AlbedoSamplerLayout,NormalSamplerLayout, };

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

		vk::DescriptorBufferInfo vertexbufferInfo{};
		vertexbufferInfo.buffer = vertexUniformBuffers[i].buffer;
		vertexbufferInfo.offset = 0;
		vertexbufferInfo.range = sizeof(TransformMatrices);

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


		std::array<vk::WriteDescriptorSet, 3> descriptorWrites{ VertexUniformdescriptorWrite,
																SamplerdescriptorWrite,NormalSamplerdescriptorWrite };

		vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}


void Model::UpdateUniformBuffer(uint32_t currentImage, Light* lightref)
{
	Drawable::UpdateUniformBuffer(currentImage, lightref);
	
	transformMatrices.viewMatrix = camera->GetViewMatrix();
	transformMatrices.projectionMatrix = camera->GetProjectionMatrix();
	transformMatrices.projectionMatrix[1][1] *= -1;

	memcpy(VertexUniformBuffersMappedMem[currentImage], &transformMatrices, sizeof(transformMatrices));
}

void Model::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };
	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint32);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(storedModelData.IndexData.size(), 1, 0, 0, 0);
}

void Model::CleanUp()
{
	if (bufferManager)
	{
		bufferManager->DestroyImage(albedoTextureData);
		bufferManager->DestroyImage(normalTextureData);
		/*bufferManager->DestroyBuffer(bottomLevelASBuffer);
		bufferManager->DestroyBuffer(scratchBuffer);*/
	}
	
	Drawable::Destructor();
}

//uint64_t Model::GetBLASAddressInfo()
//{
//	vk::AccelerationStructureDeviceAddressInfoKHR addressInfo{};
//	addressInfo.accelerationStructure = bottomLevelAS;
//	return vulkanContext->vkGetAccelerationStructureDeviceAddressKHR(vulkanContext->LogicalDevice, (VkAccelerationStructureDeviceAddressInfoKHR*)&addressInfo);
//}


