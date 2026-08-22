#include "App.h"
#include "VulkanContext.h"
#include "BufferManager.h"
#include "Model.h"
#include "MeshLoader.h"
#include "Lighting_RTX.h"
#include "DynamicDiffuse_RTGI.h"
#include "ReSTIR_DI.h"
#include <vector>

void App::createTLAS()
{
	size_t totalPrimitiveCount = 0;
	for (const auto& model : Models) {
		totalPrimitiveCount += model->BLAS_Datas.size();
	}

	// Create instance Buffer
	TLAS_InstanceData.BufferID = "Scene TLAS InstanceData Buffer";
	size_t totalSize = sizeof(vk::AccelerationStructureInstanceKHR) * totalPrimitiveCount;

	bufferManger.CreateBuffer(&TLAS_InstanceData, totalSize,
		vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
		vk::BufferUsageFlagBits::eShaderDeviceAddress, commandPool, vulkanContext.graphicsQueue);

	UpdateTLASInstanceBuffer();

	vk::BufferDeviceAddressInfo InstanceInfo{};
	InstanceInfo.buffer = TLAS_InstanceData.buffer;

	vk::DeviceOrHostAddressConstKHR instanceDataDeviceAddresstance{};
	instanceDataDeviceAddresstance.deviceAddress = vulkanContext.LogicalDevice.getBufferAddress(InstanceInfo);

	vk::AccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.geometryType = vk::GeometryTypeKHR::eInstances;
	accelerationStructureGeometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
	accelerationStructureGeometry.geometry.instances.sType = vk::StructureType::eAccelerationStructureGeometryInstancesDataKHR;
	accelerationStructureGeometry.geometry.instances.arrayOfPointers = vk::False;
	accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddresstance;

	vk::AccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {};
	accelerationStructureBuildGeometryInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
	accelerationStructureBuildGeometryInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
	accelerationStructureBuildGeometryInfo.geometryCount = 1;
	accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

	uint32_t primitive_count = static_cast<uint32_t>(totalPrimitiveCount);

	VkAccelerationStructureBuildGeometryInfoKHR TEMP_ACCELERATION_INFO = accelerationStructureBuildGeometryInfo;
	VkAccelerationStructureBuildSizesInfoKHR TEMP_ACCELERATION_STRUCTURE_BUILD_SIZE{};
	TEMP_ACCELERATION_STRUCTURE_BUILD_SIZE.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	TEMP_ACCELERATION_STRUCTURE_BUILD_SIZE.pNext = nullptr;

	vulkanContext.vkGetAccelerationStructureBuildSizesKHR(vulkanContext.LogicalDevice,
		VkAccelerationStructureBuildTypeKHR::VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&TEMP_ACCELERATION_INFO, &primitive_count, &TEMP_ACCELERATION_STRUCTURE_BUILD_SIZE);

	vk::AccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = TEMP_ACCELERATION_STRUCTURE_BUILD_SIZE;

	// Create TLAS Buffer
	TLAS_Buffer.BufferID = "Scene TLAS Buffer";
	bufferManger.CreateDeviceBuffer(&TLAS_Buffer,
		accelerationStructureBuildSizesInfo.accelerationStructureSize,
		vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
		vk::BufferUsageFlagBits::eShaderDeviceAddress,
		commandPool,
		vulkanContext.graphicsQueue);

	vk::AccelerationStructureCreateInfoKHR accelerationStructureCreate_info{};
	accelerationStructureCreate_info.buffer = TLAS_Buffer.buffer;
	accelerationStructureCreate_info.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
	accelerationStructureCreate_info.type = vk::AccelerationStructureTypeKHR::eTopLevel;

	VkAccelerationStructureCreateInfoKHR TEMP_ACCELERATION_STRUCTURE_CREATE_INFO = accelerationStructureCreate_info;
	VkAccelerationStructureKHR TEMP_TLAS;
	vulkanContext.vkCreateAccelerationStructureKHR(vulkanContext.LogicalDevice, &TEMP_ACCELERATION_STRUCTURE_CREATE_INFO, nullptr, &TEMP_TLAS);
	TLAS = TEMP_TLAS;

	// Create TLAS Scratch Buffer
	TLAS_SCRATCH_Buffer.BufferID = "TLAS_ScratchBuffer Buffer";
	bufferManger.CreateDeviceBuffer(&TLAS_SCRATCH_Buffer,
		accelerationStructureBuildSizesInfo.buildScratchSize,
		vk::BufferUsageFlagBits::eStorageBuffer |
		vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
		vk::BufferUsageFlagBits::eShaderDeviceAddress,
		commandPool,
		vulkanContext.graphicsQueue);

	vk::BufferDeviceAddressInfo TLAS_ScratchBufferAdress;
	TLAS_ScratchBufferAdress.buffer = TLAS_SCRATCH_Buffer.buffer;

	accelerationStructureBuildGeometryInfo.dstAccelerationStructure = TLAS;
	accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = vulkanContext.LogicalDevice.getBufferAddress(TLAS_ScratchBufferAdress);
	accelerationStructureBuildGeometryInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;

	// Build TLAS
	vk::CommandBuffer cmd = bufferManger.CreateSingleUseCommandBuffer(commandPool);

	vk::AccelerationStructureBuildRangeInfoKHR BuildRangeInfo;
	BuildRangeInfo.firstVertex = 0;
	BuildRangeInfo.primitiveCount = primitive_count;
	BuildRangeInfo.primitiveOffset = 0;
	BuildRangeInfo.transformOffset = 0;

	VkAccelerationStructureBuildRangeInfoKHR tempRange = BuildRangeInfo;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &tempRange };

	VkAccelerationStructureBuildGeometryInfoKHR tempGeometryInfo = accelerationStructureBuildGeometryInfo;

	vulkanContext.vkCmdBuildAccelerationStructuresKHR(cmd, 1,
		&tempGeometryInfo,
		accelerationBuildStructureRangeInfos.data());

	bufferManger.SubmitAndDestoyCommandBuffer(commandPool, cmd, vulkanContext.graphicsQueue);
}

void App::UpdateTLAS()
{
	UpdateTLASInstanceBuffer();

	size_t totalPrimitiveCount = 0;
	for (const auto& model : Models) {
		totalPrimitiveCount += model->BLAS_Datas.size();
	}

	vk::BufferDeviceAddressInfo instanceInfo{};
	instanceInfo.buffer = TLAS_InstanceData.buffer;

	vk::DeviceOrHostAddressConstKHR instanceDeviceAddress{};
	instanceDeviceAddress.deviceAddress = vulkanContext.LogicalDevice.getBufferAddress(instanceInfo);

	vk::AccelerationStructureGeometryKHR geometry{};
	geometry.geometryType = vk::GeometryTypeKHR::eInstances;
	geometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
	geometry.geometry.instances.sType = vk::StructureType::eAccelerationStructureGeometryInstancesDataKHR;
	geometry.geometry.instances.arrayOfPointers = VK_FALSE;
	geometry.geometry.instances.data = instanceDeviceAddress;

	vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{};
	buildInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
	buildInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace |
		vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
	buildInfo.geometryCount = 1;
	buildInfo.pGeometries = &geometry;
	buildInfo.mode = vk::BuildAccelerationStructureModeKHR::eUpdate;
	buildInfo.srcAccelerationStructure = TLAS;
	buildInfo.dstAccelerationStructure = TLAS;

	vk::BufferDeviceAddressInfo scratchAddrInfo{};
	scratchAddrInfo.buffer = TLAS_SCRATCH_Buffer.buffer;
	buildInfo.scratchData.deviceAddress = vulkanContext.LogicalDevice.getBufferAddress(scratchAddrInfo);

	vk::AccelerationStructureBuildRangeInfoKHR buildRange{};
	buildRange.primitiveCount = static_cast<uint32_t>(totalPrimitiveCount);
	buildRange.primitiveOffset = 0;
	buildRange.firstVertex = 0;
	buildRange.transformOffset = 0;

	VkAccelerationStructureBuildRangeInfoKHR tempRange = buildRange;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> rangeInfos = { &tempRange };

	vk::CommandBuffer cmd = bufferManger.CreateSingleUseCommandBuffer(commandPool);
	VkAccelerationStructureBuildGeometryInfoKHR tempBuildInfo = buildInfo;

	vulkanContext.vkCmdBuildAccelerationStructuresKHR(cmd, 1, &tempBuildInfo, rangeInfos.data());
	bufferManger.SubmitAndDestoyCommandBuffer(commandPool, cmd, vulkanContext.graphicsQueue);
}

void App::UpdateTLASInstanceBuffer()
{
	std::vector<vk::AccelerationStructureInstanceKHR> Instances;
	uint32_t transformIndex = 0;

	for (size_t i = 0; i < Models.size(); i++)
	{
		if (!Models[i]) continue;
		glm::mat4 modelInstanceTransform = Models[i]->Instances[0]->GetTransformationMatrix();

		for (size_t j = 0; j < Models[i]->BLAS_Datas.size(); j++)
		{
			vk::AccelerationStructureDeviceAddressInfoKHR blasinfo{};
			blasinfo.accelerationStructure = Models[i]->BLAS_Datas[j].BLAS;

			VkAccelerationStructureDeviceAddressInfoKHR Temp = blasinfo;

			Node* node = Models[i]->BLAS_Datas[j].node;
			glm::mat4 finalMatrix = node ? node->GetWorldMatrix(modelInstanceTransform) : modelInstanceTransform;

			VkTransformMatrixKHR transformMatrix = {
				finalMatrix[0][0], finalMatrix[1][0], finalMatrix[2][0], finalMatrix[3][0],
				finalMatrix[0][1], finalMatrix[1][1], finalMatrix[2][1], finalMatrix[3][1],
				finalMatrix[0][2], finalMatrix[1][2], finalMatrix[2][2], finalMatrix[3][2],
			};

			uint32_t globalPrimIndex = Models[i]->BLAS_Datas[j].GlobalPrimitiveIndex;
			uint32_t packedID = (transformIndex << 12) | (globalPrimIndex & 0xFFF);

			vk::AccelerationStructureInstanceKHR instance{};
			instance.transform = transformMatrix;
			instance.instanceCustomIndex = packedID;
			instance.mask = 0xFF;
			instance.instanceShaderBindingTableRecordOffset = 0;
			instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
			instance.accelerationStructureReference = vulkanContext.vkGetAccelerationStructureDeviceAddressKHR(vulkanContext.LogicalDevice, &Temp);

			Instances.push_back(instance);
			transformIndex++;
		}
	}
	if (!Instances.empty())
	{
		bufferManger.CopyDataToBuffer(Instances.data(), TLAS_InstanceData);
	}
}

void App::DestroyTLAS()
{
	if (TLAS_Buffer.buffer) {
		bufferManger.DestroyBuffer(TLAS_Buffer);
		bufferManger.DestroyBuffer(TLAS_SCRATCH_Buffer);
		bufferManger.DestroyBuffer(TLAS_InstanceData);
		vulkanContext.vkDestroyAccelerationStructureKHR(vulkanContext.LogicalDevice, TLAS, nullptr);
	}
}

void App::UpdateRayTracingDescriptors()
{
	if (lighting_RTX) {
		lighting_RTX->createDescriptorSetsBasedOnGBuffer(DescriptorPool, &gbuffer, &TLAS);
	}

	bool ddgiRecreated = false;
	if (dynamicDiffuse_RTGI) {
		ddgiRecreated = dynamicDiffuse_RTGI->UpdateUniformBuffer(DescriptorPool, TLAS, lighting_RTX->UniformBuffers, gbuffer, true, lights.size());
	}

	if (Restir_DI) {
		Restir_DI->createDescriptorSetsBasedOnGBuffer(DescriptorPool, &TLAS);

		if (ddgiRecreated) {
			Restir_DI->createDescriptorDDGIATLAS(DescriptorPool);
		}
	}

	if (ddgiRecreated) {
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

		Sampled_GI_ID = ImGui_ImplVulkan_AddTexture(
			dynamicDiffuse_RTGI->Probe_Sampled_GI_Image.imageSampler,
			dynamicDiffuse_RTGI->Probe_Sampled_GI_Image.imageView,
			VK_IMAGE_LAYOUT_GENERAL
		);
	}
}
