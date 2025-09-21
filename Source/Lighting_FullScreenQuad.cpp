#include "Lighting_FullScreenQuad.h"
#include <stdexcept>
#include <chrono>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include "Light.h"
#include "Camera.h"
#include "RT_Shadows.h"

Lighting_FullScreenQuad::Lighting_FullScreenQuad(BufferManager* buffermanager, VulkanContext* vulkancontext,Camera* cameraref, vk::CommandPool commandpool, SkyBox* skyboxref, RT_Shadows* raytracingref): Drawable()
{
	camera = cameraref;
	bufferManager = buffermanager;
	vulkanContext = vulkancontext;
	commandPool   = commandpool;
	SkyBoxRef = skyboxref;
	raytracingRef = raytracingref;
	CreateVertexAndIndexBuffer();
	CreateUniformBuffer();
	createDescriptorSetLayout();
}


void Lighting_FullScreenQuad::CreateVertexAndIndexBuffer()
{

	VkDeviceSize VertexBufferSize = sizeof(quad[0]) * quad.size();
	vertexBufferData.BufferID = " Lighting FullScreen Quad Vertex Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&vertexBufferData, quad.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);

	VkDeviceSize indexBufferSize = sizeof(uint16_t) * quadIndices.size();
	indexBufferData.BufferID = " Lighting FullScreen Quad Index Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&indexBufferData,quadIndices.data(), indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer, commandPool, vulkanContext->graphicsQueue);

}

void Lighting_FullScreenQuad::CreateUniformBuffer()
{
	//////////////////////////////////////////////////////////////
	fragmentUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	FragmentUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

	VkDeviceSize FragmentuniformBufferSize = sizeof(LightUniformData) * 100;

	for (size_t i = 0; i < fragmentUniformBuffers.size(); i++)
	{
		BufferData bufferdata;

		bufferdata.BufferID = " Lighting FullScreen Quad Fragment Uniform Buffer" + i;
		bufferManager->CreateBuffer(&bufferdata,FragmentuniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
		fragmentUniformBuffers[i] = bufferdata;

		FragmentUniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
	}
}

void Lighting_FullScreenQuad::createDescriptorSetLayout()
{
	{
		//////// Create set for Final Lighting Pass ////////////
		vk::DescriptorSetLayoutBinding PositionSampleryLayout{};
		PositionSampleryLayout.binding = 0;
		PositionSampleryLayout.descriptorCount = 1;
		PositionSampleryLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		PositionSampleryLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding NormalSamplerLayout{};
		NormalSamplerLayout.binding = 1;
		NormalSamplerLayout.descriptorCount = 1;
		NormalSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		NormalSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding AlbedoSamplerLayout{};
		AlbedoSamplerLayout.binding = 2;
		AlbedoSamplerLayout.descriptorCount = 1;
		AlbedoSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		AlbedoSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding MaterialsSamplerLayout{};
		MaterialsSamplerLayout.binding = 3;
		MaterialsSamplerLayout.descriptorCount = 1;
		MaterialsSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		MaterialsSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding ReflectiveCubeSamplerLayout{};
		ReflectiveCubeSamplerLayout.binding = 4;
		ReflectiveCubeSamplerLayout.descriptorCount = 1;
		ReflectiveCubeSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		ReflectiveCubeSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding ReflectionMaskSamplerLayout{};
		ReflectionMaskSamplerLayout.binding = 5;
		ReflectionMaskSamplerLayout.descriptorCount = 1;
		ReflectionMaskSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		ReflectionMaskSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding ShadowMapSamplerLayout{};
		ShadowMapSamplerLayout.binding = 6;
		ShadowMapSamplerLayout.descriptorCount = 4;
		ShadowMapSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		ShadowMapSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding EmisiveSamplerLayout{};
		EmisiveSamplerLayout.binding = 7;
		EmisiveSamplerLayout.descriptorCount = 1;
		EmisiveSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		EmisiveSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;

		vk::DescriptorSetLayoutBinding LightUniformBufferLayout{};
		LightUniformBufferLayout.binding = 8;
		LightUniformBufferLayout.descriptorCount = 1;
		LightUniformBufferLayout.descriptorType = vk::DescriptorType::eUniformBuffer;
		LightUniformBufferLayout.stageFlags = vk::ShaderStageFlagBits::eFragment;


		std::array<vk::DescriptorSetLayoutBinding, 9> bindings = { PositionSampleryLayout,        // binding 0
																   NormalSamplerLayout,           // binding 1
																   AlbedoSamplerLayout,           // binding 2
			                                                       MaterialsSamplerLayout,
																   ReflectiveCubeSamplerLayout,
																   ReflectionMaskSamplerLayout,
			                                                       ShadowMapSamplerLayout,
			                                                       EmisiveSamplerLayout,
																   LightUniformBufferLayout       // binding 3
		};

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptorset layout!");
		}
	}

}

void Lighting_FullScreenQuad::createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, GBuffer* Gbuffer, ImageData* ReflectionMask)
{

	GbufferRef = Gbuffer;
	ReflectionMaskRef = ReflectionMask;

	// create sets from the pool based on the layout
	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocinfo;
	allocinfo.descriptorPool = descriptorpool;
	allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocinfo.pSetLayouts = layouts.data();

	DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, DescriptorSets.data());

	UpdateDescrptorSets();
}

void Lighting_FullScreenQuad::UpdateDescrptorSets()
{
	//specifies what exactly to send
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		/////////////////////////////////////////////////////////////////////////////////////
		vk::DescriptorImageInfo PositionimageInfo{};
		PositionimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		PositionimageInfo.imageView = GbufferRef->Position.imageView;
		PositionimageInfo.sampler = GbufferRef->Position.imageSampler;

		vk::WriteDescriptorSet PositionSamplerdescriptorWrite{};
		PositionSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		PositionSamplerdescriptorWrite.dstBinding = 0;
		PositionSamplerdescriptorWrite.dstArrayElement = 0;
		PositionSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		PositionSamplerdescriptorWrite.descriptorCount = 1;
		PositionSamplerdescriptorWrite.pImageInfo = &PositionimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////
		;
		vk::DescriptorImageInfo NormalimageInfo{};
		NormalimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		NormalimageInfo.imageView = GbufferRef->Normal.imageView;
		NormalimageInfo.sampler = GbufferRef->Normal.imageSampler;

		vk::WriteDescriptorSet NormalSamplerdescriptorWrite{};
		NormalSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		NormalSamplerdescriptorWrite.dstBinding = 1;
		NormalSamplerdescriptorWrite.dstArrayElement = 0;
		NormalSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		NormalSamplerdescriptorWrite.descriptorCount = 1;
		NormalSamplerdescriptorWrite.pImageInfo = &NormalimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////

		vk::DescriptorImageInfo AlbedoimageInfo{};
		AlbedoimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		AlbedoimageInfo.imageView = GbufferRef->Albedo.imageView;
		AlbedoimageInfo.sampler = GbufferRef->Albedo.imageSampler;

		vk::WriteDescriptorSet  AlbedoSamplerdescriptorWrite{};
		AlbedoSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		AlbedoSamplerdescriptorWrite.dstBinding = 2;
		AlbedoSamplerdescriptorWrite.dstArrayElement = 0;
		AlbedoSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		AlbedoSamplerdescriptorWrite.descriptorCount = 1;
		AlbedoSamplerdescriptorWrite.pImageInfo = &AlbedoimageInfo;
		/////////////////////////////////////////////////////////////////////////////////////

		vk::DescriptorImageInfo MaterialsimageInfo{};
		MaterialsimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		MaterialsimageInfo.imageView = GbufferRef->Materials.imageView;
		MaterialsimageInfo.sampler = GbufferRef->Materials.imageSampler;

		vk::WriteDescriptorSet MaterialsSamplerdescriptorWrite{};
		MaterialsSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		MaterialsSamplerdescriptorWrite.dstBinding = 3;
		MaterialsSamplerdescriptorWrite.dstArrayElement = 0;
		MaterialsSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		MaterialsSamplerdescriptorWrite.descriptorCount = 1;
		MaterialsSamplerdescriptorWrite.pImageInfo = &MaterialsimageInfo;


		vk::DescriptorImageInfo ReflectiveCubeimageInfo{};
		ReflectiveCubeimageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		ReflectiveCubeimageInfo.imageView = SkyBoxRef->SkyBoxImages[SkyBoxRef->SkyBoxIndex].imageView;
		ReflectiveCubeimageInfo.sampler   = SkyBoxRef->SkyBoxImages[SkyBoxRef->SkyBoxIndex].imageSampler;

		vk::WriteDescriptorSet ReflectiveCubeSamplerdescriptorWrite{};
		ReflectiveCubeSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		ReflectiveCubeSamplerdescriptorWrite.dstBinding = 4;
		ReflectiveCubeSamplerdescriptorWrite.dstArrayElement = 0;
		ReflectiveCubeSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		ReflectiveCubeSamplerdescriptorWrite.descriptorCount = 1;
		ReflectiveCubeSamplerdescriptorWrite.pImageInfo = &ReflectiveCubeimageInfo;


		vk::DescriptorImageInfo ReflectionMaskimageInfo{};
		ReflectionMaskimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		ReflectionMaskimageInfo.imageView = ReflectionMaskRef->imageView;
		ReflectionMaskimageInfo.sampler = ReflectionMaskRef->imageSampler;

		vk::WriteDescriptorSet ReflectionMaskSamplerdescriptorWrite{};
		ReflectionMaskSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		ReflectionMaskSamplerdescriptorWrite.dstBinding = 5;
		ReflectionMaskSamplerdescriptorWrite.dstArrayElement = 0;
		ReflectionMaskSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		ReflectionMaskSamplerdescriptorWrite.descriptorCount = 1;
		ReflectionMaskSamplerdescriptorWrite.pImageInfo = &ReflectionMaskimageInfo;


		std::vector<vk::DescriptorImageInfo>ShadowImagesInfos;

		for (int i = 0; i < raytracingRef->ShadowPassImages.size(); i++)
		{

			vk::DescriptorImageInfo StoreageImageInfo{};
			StoreageImageInfo.imageLayout = vk::ImageLayout::eGeneral;
			StoreageImageInfo.imageView = raytracingRef->ShadowPassImages[i].imageView;
			StoreageImageInfo.sampler = raytracingRef->ShadowPassImages[i].imageSampler;

			ShadowImagesInfos.push_back(StoreageImageInfo);
		}

		vk::WriteDescriptorSet StoreageImagSamplerdescriptorWrite{};
		StoreageImagSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		StoreageImagSamplerdescriptorWrite.dstBinding = 6;
		StoreageImagSamplerdescriptorWrite.dstArrayElement = 0;
		StoreageImagSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		StoreageImagSamplerdescriptorWrite.descriptorCount = ShadowImagesInfos.size();
		StoreageImagSamplerdescriptorWrite.pImageInfo = ShadowImagesInfos.data();




		vk::DescriptorImageInfo EmisiveimageInfo{};
		EmisiveimageInfo.imageLayout = vk::ImageLayout::eGeneral;
		EmisiveimageInfo.imageView = GbufferRef->Emissive.imageView;
		EmisiveimageInfo.sampler = GbufferRef->Emissive.imageSampler;

		vk::WriteDescriptorSet EmisiveSamplerdescriptorWrite{};
		EmisiveSamplerdescriptorWrite.dstSet = DescriptorSets[i];
		EmisiveSamplerdescriptorWrite.dstBinding = 7;
		EmisiveSamplerdescriptorWrite.dstArrayElement = 0;
		EmisiveSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		EmisiveSamplerdescriptorWrite.descriptorCount = 1;
		EmisiveSamplerdescriptorWrite.pImageInfo = &EmisiveimageInfo;

		/////////////////////////////////////////////////////////////////////////////////////

		vk::DescriptorBufferInfo LightUniformBufferInfo;
		LightUniformBufferInfo.buffer = fragmentUniformBuffers[i].buffer;
		LightUniformBufferInfo.offset = 0;
		LightUniformBufferInfo.range = sizeof(LightUniformData) * 100;

		vk::WriteDescriptorSet LightUniformBufferDescriptorWrite{};
		LightUniformBufferDescriptorWrite.dstSet = DescriptorSets[i];
		LightUniformBufferDescriptorWrite.dstBinding = 8;
		LightUniformBufferDescriptorWrite.dstArrayElement = 0;
		LightUniformBufferDescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		LightUniformBufferDescriptorWrite.descriptorCount = 1;
		LightUniformBufferDescriptorWrite.pBufferInfo = &LightUniformBufferInfo;

		std::array<vk::WriteDescriptorSet, 9> descriptorWrites = {
																	PositionSamplerdescriptorWrite,        // binding 0
																	NormalSamplerdescriptorWrite,          // binding 1
																	AlbedoSamplerdescriptorWrite,          // binding 2
																	MaterialsSamplerdescriptorWrite,
																	ReflectiveCubeSamplerdescriptorWrite,
																	ReflectionMaskSamplerdescriptorWrite,
																	StoreageImagSamplerdescriptorWrite,
																	EmisiveSamplerdescriptorWrite,
																	LightUniformBufferDescriptorWrite      // binding 3
		};

		vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}

void Lighting_FullScreenQuad::UpdateUniformBuffer(uint32_t currentImage, std::vector<std::shared_ptr<Light>>& lightref)
{
	if (SkyBoxRef->bSkyBoxUpdate)
	{
		UpdateDescrptorSets();
		SkyBoxRef->bSkyBoxUpdate = false;
	}

	std::vector<LightUniformData> lightDataspack;
	lightDataspack.reserve(lightref.size());

	for (int  i = 0; i < lightref.size(); i++)
	{
		Drawable::UpdateUniformBuffer(currentImage);

		if (lightref[i])
		{
			LightUniformData LightData;
			LightData.lightPositionAndLightType = glm::vec4(lightref[i]->position,lightref[i]->lightType);
			LightData.colorAndAmbientStrength  = glm::vec4(lightref[i]->color, lightref[i]->ambientStrength);
			LightData.CameraPositionAndLightIntensity = glm::vec4(camera->GetPosition().x, 
				                                                  camera->GetPosition().y,
				                                                  camera->GetPosition().z, 
				                                                  lightref[i]->lightIntensity);

			glm::mat4 projection = lightref[i]->ProjectionMatrix;
			projection[1][1] *= -1;
			LightData.LightViewProjMatrix = projection * lightref[i]->ViewMatrix;

			lightDataspack.push_back(LightData);
		}
	}

	memcpy(FragmentUniformBuffersMappedMem[currentImage], lightDataspack.data(), lightDataspack.size() * sizeof(LightUniformData));

}

void Lighting_FullScreenQuad::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };
	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindIndexBuffer(indexBufferData.buffer, 0, vk::IndexType::eUint16);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.drawIndexed(quadIndices.size(), 1, 0, 0, 0);
}

void Lighting_FullScreenQuad::CleanUp()
{

	Drawable::Destructor();
}

