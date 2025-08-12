#include "SkyBox.h"
#include <stdexcept>
#include <memory>
#include <chrono>

SkyBox::SkyBox(VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* cameraref, BufferManager* buffermanger)
{
	vulkanContext = vulkancontext;
	commandPool = commandpool;
	camera = cameraref;
	bufferManager = buffermanger;
	SkyBoxIndex = 0;
	LastSkyBoxIndex = 0;
	bSkyBoxUpdate = false;

	{
		std::array<const char*, 6> filePaths{
		"../Textures/Skybox/DaySky/px.png",  // +X (Right)
		"../Textures/Skybox/DaySky/nx.png",  // -X (Left)
		"../Textures/Skybox/DaySky/py.png",  // +Y (Top)
		"../Textures/Skybox/DaySky/ny.png",  // -Y (Bottom)
		"../Textures/Skybox/DaySky/pz.png",  // +Z (Front)
		"../Textures/Skybox/DaySky/nz.png"   // -Z (Back)
		};

		ImageData SkyBox1;
		SkyBox1.ImageID = "skybox 1 Cube Map Texture";
		bufferManager->CreateCubeMap(&SkyBox1, filePaths, commandPool, vulkanContext->graphicsQueue);
		SkyBoxImages.push_back(SkyBox1);
	}

	{
		std::array<const char*, 6> filePaths{
		"../Textures/Skybox/Church/px.jpg",  // +X (Right)
		"../Textures/Skybox/Church/nx.jpg",  // -X (Left)
		"../Textures/Skybox/Church/py.jpg",  // +Y (Top)
		"../Textures/Skybox/Church/ny.jpg",  // -Y (Bottom)
		"../Textures/Skybox/Church/pz.jpg",  // +Z (Front)
		"../Textures/Skybox/Church/nz.jpg"   // -Z (Back)
		};

		ImageData SkyBox2;
		SkyBox2.ImageID = "skybox 2 Cube Map Texture";
		bufferManager->CreateCubeMap(&SkyBox2, filePaths, commandPool, vulkanContext->graphicsQueue);
		SkyBoxImages.push_back(SkyBox2);
	}

	{
		std::array<const char*, 6> filePaths{
		"../Textures/Skybox/Room/px.jpg",  // +X (Right)
		"../Textures/Skybox/Room/nx.jpg",  // -X (Left)
		"../Textures/Skybox/Room/py.jpg",  // +Y (Top)
		"../Textures/Skybox/Room/ny.jpg",  // -Y (Bottom)
		"../Textures/Skybox/Room/pz.jpg",  // +Z (Front)
		"../Textures/Skybox/Room/nz.jpg"   // -Z (Back)
		};

		ImageData SkyBox3;
		SkyBox3.ImageID = "skybox 3 Cube Map Texture";
		bufferManager->CreateCubeMap(&SkyBox3, filePaths, commandPool, vulkanContext->graphicsQueue);
		SkyBoxImages.push_back(SkyBox3);
	}

	CreateUniformBuffer();
	CreateVertexAndIndexBuffer();
	createDescriptorSetLayout();
}


void SkyBox::CreateVertexAndIndexBuffer()
{
	VkDeviceSize VertexBufferSize = sizeof(vertices.data()[0]) * vertices.size();
	vertexBufferData.BufferID = "SkyBox Vertex Buffer";
	bufferManager->CreateGPUOptimisedBuffer(&vertexBufferData,vertices.data(), VertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, commandPool, vulkanContext->graphicsQueue);
}

void SkyBox::CreateUniformBuffer()
{
	vertexUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	VertexUniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

	VkDeviceSize UniformBufferSize = sizeof(TransformMatrices);

	for (size_t i = 0; i < vertexUniformBuffers.size(); i++)
	{
		BufferData bufferdata;
		bufferdata.BufferID = "SkyBox Uniform Buffer";

	 bufferManager->CreateBuffer(&bufferdata,UniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
		vertexUniformBuffers[i] = bufferdata;

		VertexUniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
	}
}


void SkyBox::createDescriptorSetLayout()
{
	vk::DescriptorSetLayoutBinding UniformBufferBinding{};
	UniformBufferBinding.binding = 0;
	UniformBufferBinding.descriptorCount = 1;
	UniformBufferBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	UniformBufferBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

	vk::DescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;


	std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { UniformBufferBinding, samplerLayoutBinding };

	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();


	if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess)
	{
		throw std::runtime_error("Failed to create descriptorset layout!");
	}
}

void SkyBox::createDescriptorSets(vk::DescriptorPool descriptorpool)
{
	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocinfo;
	allocinfo.descriptorPool = descriptorpool;
	allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocinfo.pSetLayouts = layouts.data();

	DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, DescriptorSets.data());

	UpdateDescriptorSets();
}

void SkyBox::UpdateDescriptorSets()
{


	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		vk::DescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = vertexUniformBuffers[i].buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(TransformMatrices);

		vk::DescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.imageView = SkyBoxImages[SkyBoxIndex].imageView;
		imageInfo.sampler = SkyBoxImages[SkyBoxIndex].imageSampler;

		vk::WriteDescriptorSet UniformdescriptorWrite{};
		UniformdescriptorWrite.dstSet = DescriptorSets[i];
		UniformdescriptorWrite.dstBinding = 0;
		UniformdescriptorWrite.dstArrayElement = 0;
		UniformdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		UniformdescriptorWrite.descriptorCount = 1;
		UniformdescriptorWrite.pBufferInfo = &bufferInfo;

		vk::WriteDescriptorSet SamplerdescriptorWrite{};
		SamplerdescriptorWrite.dstSet = DescriptorSets[i];
		SamplerdescriptorWrite.dstBinding = 1;
		SamplerdescriptorWrite.dstArrayElement = 0;
		SamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		SamplerdescriptorWrite.descriptorCount = 1;
		SamplerdescriptorWrite.pImageInfo = &imageInfo;

		std::array<vk::WriteDescriptorSet, 2> descriptorWrites{ UniformdescriptorWrite ,SamplerdescriptorWrite };

		vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}

void SkyBox::UpdateUniformBuffer(uint32_t currentImage)
{
	if (SkyBoxIndex != LastSkyBoxIndex)
	{
		UpdateDescriptorSets();
		LastSkyBoxIndex = SkyBoxIndex;
		bSkyBoxUpdate = true;
	}

	Drawable::UpdateUniformBuffer(currentImage);

	transformMatrices.modelMatrix = glm::mat4(1.0f);
	transformMatrices.viewMatrix = camera->GetViewMatrix();
	transformMatrices.projectionMatrix = camera->GetProjectionMatrix();
	transformMatrices.projectionMatrix[1][1] *= -1;

	memcpy(VertexUniformBuffersMappedMem[currentImage], &transformMatrices, sizeof(transformMatrices));
}

void SkyBox::Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex)
{
	vk::DeviceSize offsets[] = { 0 };
	vk::Buffer VertexBuffers[] = { vertexBufferData.buffer };
	commandbuffer.bindVertexBuffers(0, 1, VertexBuffers, offsets);
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);
	commandbuffer.draw(vertices.size(), 1, 0, 0);
}

void SkyBox::CleanUp()
{
	
    bufferManager->DestroyImage(SkyBoxImages[0]);
	bufferManager->DestroyImage(SkyBoxImages[1]);
	Drawable::Destructor();
}


