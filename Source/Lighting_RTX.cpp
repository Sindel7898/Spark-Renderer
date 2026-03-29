#include "Lighting_RTX.h"
#include <stdexcept>
#include <chrono>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include "Light.h"
#include "Camera.h"

Lighting_RTX::Lighting_RTX(BufferManager* buffermanager, VulkanContext* vulkancontext, Camera* cameraref, vk::CommandPool commandpool, SkyBox* skyboxref)
{
    camera = cameraref;
    bufferManager = buffermanager;
    vulkanContext = vulkancontext;
    commandPool = commandpool;
    SkyBoxRef = skyboxref;

    CreateUniformBuffer();
    createDescriptorSetLayout();
}

Lighting_RTX::~Lighting_RTX() {
    CleanUp();
}

void Lighting_RTX::CreateUniformBuffer()
{
    UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    UniformBuffersMappedMem.resize(MAX_FRAMES_IN_FLIGHT);

    VkDeviceSize FragmentuniformBufferSize = sizeof(LightUniformData) * MAX_LIGHT_COUNT;

    for (size_t i = 0; i < UniformBuffers.size(); i++)
    {
        BufferData bufferdata;
        bufferdata.BufferID = "Lighting Uniform Buffer " + std::to_string(i);
        bufferManager->CreateBuffer(&bufferdata, FragmentuniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, commandPool, vulkanContext->graphicsQueue);
        UniformBuffers[i] = bufferdata;
        UniformBuffersMappedMem[i] = bufferManager->MapMemory(bufferdata);
    }
}

void Lighting_RTX::CreateStorageImage() {
    swapchainextent = vk::Extent3D(vulkanContext->swapchainExtent.width, vulkanContext->swapchainExtent.height, 1);

    ResultingStorageImage.ImageID = "Lighting with rt Pass Image";
    bufferManager->CreateImage(&ResultingStorageImage, swapchainextent, vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, false);
    ResultingStorageImage.imageView = bufferManager->CreateImageView(&ResultingStorageImage, vk::Format::eR16G16B16A16Sfloat, vk::ImageAspectFlagBits::eColor);
    ResultingStorageImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

    PTGI_StorageImage.ImageID = "PTGI Pass Image";
    bufferManager->CreateImage(&PTGI_StorageImage, swapchainextent, vk::Format::eR32G32B32A32Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, false);
    PTGI_StorageImage.imageView = bufferManager->CreateImageView(&PTGI_StorageImage, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor);
    PTGI_StorageImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);

    Prev_Frame_PTGI_StorageImage.ImageID = "Last Frame Pass Image";
    bufferManager->CreateImage(&Prev_Frame_PTGI_StorageImage, swapchainextent, vk::Format::eR32G32B32A32Sfloat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, false);
    Prev_Frame_PTGI_StorageImage.imageView = bufferManager->CreateImageView(&Prev_Frame_PTGI_StorageImage, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor);
    Prev_Frame_PTGI_StorageImage.imageSampler = bufferManager->CreateImageSampler(vk::SamplerAddressMode::eClampToEdge);
}

void Lighting_RTX::DestroyStorageImage() {
    bufferManager->DestroyImage(ResultingStorageImage);
    bufferManager->DestroyImage(PTGI_StorageImage);
    bufferManager->DestroyImage(Prev_Frame_PTGI_StorageImage);
}

void Lighting_RTX::createDescriptorSetLayout()
{
    vk::DescriptorSetLayoutBinding PositionSamplerLayout{};
    PositionSamplerLayout.binding = 0;
    PositionSamplerLayout.descriptorCount = 1;
    PositionSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    PositionSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

    vk::DescriptorSetLayoutBinding NormalSamplerLayout{};
    NormalSamplerLayout.binding = 1;
    NormalSamplerLayout.descriptorCount = 1;
    NormalSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    NormalSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

    vk::DescriptorSetLayoutBinding AlbedoSamplerLayout{};
    AlbedoSamplerLayout.binding = 2;
    AlbedoSamplerLayout.descriptorCount = 1;
    AlbedoSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    AlbedoSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

    vk::DescriptorSetLayoutBinding MaterialsSamplerLayout{};
    MaterialsSamplerLayout.binding = 3;
    MaterialsSamplerLayout.descriptorCount = 1;
    MaterialsSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    MaterialsSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

    vk::DescriptorSetLayoutBinding ReflectiveCubeSamplerLayout{};
    ReflectiveCubeSamplerLayout.binding = 4;
    ReflectiveCubeSamplerLayout.descriptorCount = 1;
    ReflectiveCubeSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    ReflectiveCubeSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

    vk::DescriptorSetLayoutBinding EmisiveSamplerLayout{};
    EmisiveSamplerLayout.binding = 5;
    EmisiveSamplerLayout.descriptorCount = 1;
    EmisiveSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    EmisiveSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

    vk::DescriptorSetLayoutBinding ResultingImageLayout{};
    ResultingImageLayout.binding = 6;
    ResultingImageLayout.descriptorCount = 1;
    ResultingImageLayout.descriptorType = vk::DescriptorType::eStorageImage;
    ResultingImageLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

    vk::DescriptorSetLayoutBinding LightUniformBufferLayout{};
    LightUniformBufferLayout.binding = 7;
    LightUniformBufferLayout.descriptorCount = 1;
    LightUniformBufferLayout.descriptorType = vk::DescriptorType::eUniformBuffer;
    LightUniformBufferLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR;

    vk::DescriptorSetLayoutBinding TLASLayout{};
    TLASLayout.binding = 8;
    TLASLayout.descriptorCount = 1;
    TLASLayout.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
    TLASLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR;

    uint32_t albedoCount = bufferManager->AllScene_Albedo_Images.size();
    vk::DescriptorSetLayoutBinding Scene_AlbedoSamplerLayout{};
    Scene_AlbedoSamplerLayout.binding = 9;
    Scene_AlbedoSamplerLayout.descriptorCount = albedoCount;
    Scene_AlbedoSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    Scene_AlbedoSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

    uint32_t normalCount = bufferManager->AllScene_Normal_Images.size();
    vk::DescriptorSetLayoutBinding Scene_NormalSamplerLayout{};
    Scene_NormalSamplerLayout.binding = 10;
    Scene_NormalSamplerLayout.descriptorCount = normalCount;
    Scene_NormalSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    Scene_NormalSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

    uint32_t roughCount = bufferManager->AllScene_MetalicRoughness_Images.size();
    vk::DescriptorSetLayoutBinding Scene_MetalicRoughnessSamplerLayout{};
    Scene_MetalicRoughnessSamplerLayout.binding = 11;
    Scene_MetalicRoughnessSamplerLayout.descriptorCount = roughCount;
    Scene_MetalicRoughnessSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    Scene_MetalicRoughnessSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

    uint32_t emissiveCount = bufferManager->AllScene_Emissive_Images.size();
    vk::DescriptorSetLayoutBinding Scene_EmmisiveSamplerLayout{};
    Scene_EmmisiveSamplerLayout.binding = 12;
    Scene_EmmisiveSamplerLayout.descriptorCount = emissiveCount;
    Scene_EmmisiveSamplerLayout.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    Scene_EmmisiveSamplerLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

    vk::DescriptorSetLayoutBinding Scene_IndexStorage_BuffersLayout{};
    Scene_IndexStorage_BuffersLayout.binding = 13;
    Scene_IndexStorage_BuffersLayout.descriptorCount = 1;
    Scene_IndexStorage_BuffersLayout.descriptorType = vk::DescriptorType::eStorageBuffer;
    Scene_IndexStorage_BuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

    vk::DescriptorSetLayoutBinding Scene_VertexStorage_BuffersLayout{};
    Scene_VertexStorage_BuffersLayout.binding = 14;
    Scene_VertexStorage_BuffersLayout.descriptorCount = 1;
    Scene_VertexStorage_BuffersLayout.descriptorType = vk::DescriptorType::eStorageBuffer;
    Scene_VertexStorage_BuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

    vk::DescriptorSetLayoutBinding Scene_offsetStorage_BuffersLayout{};
    Scene_offsetStorage_BuffersLayout.binding = 15;
    Scene_offsetStorage_BuffersLayout.descriptorCount = 1;
    Scene_offsetStorage_BuffersLayout.descriptorType = vk::DescriptorType::eStorageBuffer;
    Scene_offsetStorage_BuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

    vk::DescriptorSetLayoutBinding Scene_trasnformation_BuffersLayout{};
    Scene_trasnformation_BuffersLayout.binding = 16;
    Scene_trasnformation_BuffersLayout.descriptorCount = 1;
    Scene_trasnformation_BuffersLayout.descriptorType = vk::DescriptorType::eUniformBuffer;
    Scene_trasnformation_BuffersLayout.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR;

    vk::DescriptorSetLayoutBinding PT_ResultingImageLayout{};
    PT_ResultingImageLayout.binding = 17;
    PT_ResultingImageLayout.descriptorCount = 1;
    PT_ResultingImageLayout.descriptorType = vk::DescriptorType::eStorageImage;
    PT_ResultingImageLayout.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

    std::array<vk::DescriptorSetLayoutBinding, 18> bindings = {
        PositionSamplerLayout, NormalSamplerLayout, AlbedoSamplerLayout, MaterialsSamplerLayout,
        ReflectiveCubeSamplerLayout, EmisiveSamplerLayout, ResultingImageLayout,
        LightUniformBufferLayout, TLASLayout, Scene_AlbedoSamplerLayout, Scene_NormalSamplerLayout,
        Scene_MetalicRoughnessSamplerLayout, Scene_EmmisiveSamplerLayout,
        Scene_IndexStorage_BuffersLayout, Scene_VertexStorage_BuffersLayout,
		Scene_offsetStorage_BuffersLayout, Scene_trasnformation_BuffersLayout,PT_ResultingImageLayout
    };

    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vulkanContext->LogicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create descriptorset layout!");
    }
}

void Lighting_RTX::createDescriptorSetsBasedOnGBuffer(vk::DescriptorPool descriptorpool, GBuffer* Gbuffer, vk::AccelerationStructureKHR* TLAS)
{
    TLASr = TLAS;
    GbufferRef = Gbuffer;

    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

    vk::DescriptorSetAllocateInfo allocinfo;
    allocinfo.descriptorPool = descriptorpool;
    allocinfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocinfo.pSetLayouts = layouts.data();

    DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    if (vulkanContext->LogicalDevice.allocateDescriptorSets(&allocinfo, DescriptorSets.data()) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    UpdateDescrptorSets();
}

void Lighting_RTX::UpdateDescrptorSets()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

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

        vk::DescriptorImageInfo AlbedoimageInfo{};
        AlbedoimageInfo.imageLayout = vk::ImageLayout::eGeneral;
        AlbedoimageInfo.imageView = GbufferRef->Albedo.imageView;
        AlbedoimageInfo.sampler = GbufferRef->Albedo.imageSampler;

        vk::WriteDescriptorSet AlbedoSamplerdescriptorWrite{};
        AlbedoSamplerdescriptorWrite.dstSet = DescriptorSets[i];
        AlbedoSamplerdescriptorWrite.dstBinding = 2;
        AlbedoSamplerdescriptorWrite.dstArrayElement = 0;
        AlbedoSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        AlbedoSamplerdescriptorWrite.descriptorCount = 1;
        AlbedoSamplerdescriptorWrite.pImageInfo = &AlbedoimageInfo;

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

        if (SkyBoxRef && !SkyBoxRef->SkyBoxImages.empty()) {
            ReflectiveCubeimageInfo.imageView = SkyBoxRef->SkyBoxImages[SkyBoxRef->SkyBoxIndex].imageView;
            ReflectiveCubeimageInfo.sampler = SkyBoxRef->SkyBoxImages[SkyBoxRef->SkyBoxIndex].imageSampler;
        }

        vk::WriteDescriptorSet ReflectiveCubeSamplerdescriptorWrite{};
        ReflectiveCubeSamplerdescriptorWrite.dstSet = DescriptorSets[i];
        ReflectiveCubeSamplerdescriptorWrite.dstBinding = 4;
        ReflectiveCubeSamplerdescriptorWrite.dstArrayElement = 0;
        ReflectiveCubeSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        ReflectiveCubeSamplerdescriptorWrite.descriptorCount = 1;
        ReflectiveCubeSamplerdescriptorWrite.pImageInfo = &ReflectiveCubeimageInfo;

        vk::DescriptorImageInfo EmisiveimageInfo{};
        EmisiveimageInfo.imageLayout = vk::ImageLayout::eGeneral;
        EmisiveimageInfo.imageView = GbufferRef->Emissive.imageView;
        EmisiveimageInfo.sampler = GbufferRef->Emissive.imageSampler;

        vk::WriteDescriptorSet EmisiveSamplerdescriptorWrite{};
        EmisiveSamplerdescriptorWrite.dstSet = DescriptorSets[i];
        EmisiveSamplerdescriptorWrite.dstBinding = 5;
        EmisiveSamplerdescriptorWrite.dstArrayElement = 0;
        EmisiveSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        EmisiveSamplerdescriptorWrite.descriptorCount = 1;
        EmisiveSamplerdescriptorWrite.pImageInfo = &EmisiveimageInfo;

        vk::DescriptorImageInfo ResultingimageInfo{};
        ResultingimageInfo.imageLayout = vk::ImageLayout::eGeneral;
        ResultingimageInfo.imageView = ResultingStorageImage.imageView;
        ResultingimageInfo.sampler = ResultingStorageImage.imageSampler;

        vk::WriteDescriptorSet ResultingdescriptorWrite{};
        ResultingdescriptorWrite.dstSet = DescriptorSets[i];
        ResultingdescriptorWrite.dstBinding = 6;
        ResultingdescriptorWrite.dstArrayElement = 0;
        ResultingdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
        ResultingdescriptorWrite.descriptorCount = 1;
        ResultingdescriptorWrite.pImageInfo = &ResultingimageInfo;

        vk::DescriptorBufferInfo LightUniformBufferInfo;
        LightUniformBufferInfo.buffer = UniformBuffers[i].buffer;
        LightUniformBufferInfo.offset = 0;
        LightUniformBufferInfo.range = sizeof(LightUniformData) * MAX_LIGHT_COUNT; 

        vk::WriteDescriptorSet LightUniformBufferDescriptorWrite{};
        LightUniformBufferDescriptorWrite.dstSet = DescriptorSets[i];
        LightUniformBufferDescriptorWrite.dstBinding = 7;
        LightUniformBufferDescriptorWrite.dstArrayElement = 0;
        LightUniformBufferDescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
        LightUniformBufferDescriptorWrite.descriptorCount = 1;
        LightUniformBufferDescriptorWrite.pBufferInfo = &LightUniformBufferInfo;

        vk::WriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
        descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
        descriptorAccelerationStructureInfo.pAccelerationStructures = TLASr;

        vk::WriteDescriptorSet TLAS_descriptorWrite{};
        TLAS_descriptorWrite.dstSet = DescriptorSets[i];
        TLAS_descriptorWrite.dstBinding = 8;
        TLAS_descriptorWrite.dstArrayElement = 0;
        TLAS_descriptorWrite.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
        TLAS_descriptorWrite.descriptorCount = 1;
        TLAS_descriptorWrite.pNext = &descriptorAccelerationStructureInfo;

        // --- ASSET IMAGES (ALBEDO) ---
        std::vector<vk::DescriptorImageInfo> AssetImagesInfos;
        for (auto* imageData : bufferManager->AllScene_Albedo_Images) {
            if (imageData) {
                vk::DescriptorImageInfo ASSETImageInfo{};
                ASSETImageInfo.imageLayout = vk::ImageLayout::eGeneral;
                ASSETImageInfo.imageView = imageData->imageView;
                ASSETImageInfo.sampler = imageData->imageSampler;
                AssetImagesInfos.push_back(ASSETImageInfo);
            }
        }

        vk::WriteDescriptorSet AssetImagSamplerdescriptorWrite{};
        AssetImagSamplerdescriptorWrite.dstSet = DescriptorSets[i];
        AssetImagSamplerdescriptorWrite.dstBinding = 9;
        AssetImagSamplerdescriptorWrite.dstArrayElement = 0;
        AssetImagSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        AssetImagSamplerdescriptorWrite.descriptorCount = static_cast<uint32_t>(AssetImagesInfos.size());
        AssetImagSamplerdescriptorWrite.pImageInfo = AssetImagesInfos.data();

        // --- ASSET IMAGES (NORMAL) ---
        std::vector<vk::DescriptorImageInfo> NormalImageAssetImagesInfos;
        for (auto* imageData : bufferManager->AllScene_Normal_Images) {
            if (imageData) {
                vk::DescriptorImageInfo NormalASSETImageInfo{};
                NormalASSETImageInfo.imageLayout = vk::ImageLayout::eGeneral;
                NormalASSETImageInfo.imageView = imageData->imageView;
                NormalASSETImageInfo.sampler = imageData->imageSampler;
                NormalImageAssetImagesInfos.push_back(NormalASSETImageInfo);
            }
        }

        vk::WriteDescriptorSet NormalAssetImagSamplerdescriptorWrite{};
        NormalAssetImagSamplerdescriptorWrite.dstSet = DescriptorSets[i];
        NormalAssetImagSamplerdescriptorWrite.dstBinding = 10;
        NormalAssetImagSamplerdescriptorWrite.dstArrayElement = 0;
        NormalAssetImagSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        NormalAssetImagSamplerdescriptorWrite.descriptorCount = static_cast<uint32_t>(NormalImageAssetImagesInfos.size());
        NormalAssetImagSamplerdescriptorWrite.pImageInfo = NormalImageAssetImagesInfos.data();

        // --- ASSET IMAGES (ROUGHNESS) ---
        std::vector<vk::DescriptorImageInfo> MetalicRoughnessImageAssetImagesInfos;
        for (auto* imageData : bufferManager->AllScene_MetalicRoughness_Images) {
            if (imageData) {
                vk::DescriptorImageInfo MetalicRoughnessASSETImageInfo{};
                MetalicRoughnessASSETImageInfo.imageLayout = vk::ImageLayout::eGeneral;
                MetalicRoughnessASSETImageInfo.imageView = imageData->imageView;
                MetalicRoughnessASSETImageInfo.sampler = imageData->imageSampler;
                MetalicRoughnessImageAssetImagesInfos.push_back(MetalicRoughnessASSETImageInfo);
            }
        }

        vk::WriteDescriptorSet MetalicRoughnessAssetImagSamplerdescriptorWrite{};
        MetalicRoughnessAssetImagSamplerdescriptorWrite.dstSet = DescriptorSets[i];
        MetalicRoughnessAssetImagSamplerdescriptorWrite.dstBinding = 11;
        MetalicRoughnessAssetImagSamplerdescriptorWrite.dstArrayElement = 0;
        MetalicRoughnessAssetImagSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        MetalicRoughnessAssetImagSamplerdescriptorWrite.descriptorCount = static_cast<uint32_t>(MetalicRoughnessImageAssetImagesInfos.size());
        MetalicRoughnessAssetImagSamplerdescriptorWrite.pImageInfo = MetalicRoughnessImageAssetImagesInfos.data();

        // --- ASSET IMAGES (EMISSIVE) ---
        std::vector<vk::DescriptorImageInfo> EmmisiveImageAssetImagesInfos;
        for (auto* imageData : bufferManager->AllScene_Emissive_Images) {
            if (imageData) {
                vk::DescriptorImageInfo EmmisiveASSETImageInfo{};
                EmmisiveASSETImageInfo.imageLayout = vk::ImageLayout::eGeneral;
                EmmisiveASSETImageInfo.imageView = imageData->imageView;
                EmmisiveASSETImageInfo.sampler = imageData->imageSampler;
                EmmisiveImageAssetImagesInfos.push_back(EmmisiveASSETImageInfo);
            }
        }

        vk::WriteDescriptorSet EmmisiveAssetImagSamplerdescriptorWrite{};
        EmmisiveAssetImagSamplerdescriptorWrite.dstSet = DescriptorSets[i];
        EmmisiveAssetImagSamplerdescriptorWrite.dstBinding = 12;
        EmmisiveAssetImagSamplerdescriptorWrite.dstArrayElement = 0;
        EmmisiveAssetImagSamplerdescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        EmmisiveAssetImagSamplerdescriptorWrite.descriptorCount = static_cast<uint32_t>(EmmisiveImageAssetImagesInfos.size());
        EmmisiveAssetImagSamplerdescriptorWrite.pImageInfo = EmmisiveImageAssetImagesInfos.data();

        // --- BUFFERS ---
        vk::DescriptorBufferInfo IndexStorageBuffersInfo{};
        if (!bufferManager->AllScene_IndexStorageBuffers.empty()) {
            IndexStorageBuffersInfo.buffer = bufferManager->AllScene_IndexStorageBuffers[0].buffer;
            IndexStorageBuffersInfo.range = sizeof(uint32_t) * bufferManager->AllScene_IndexGeometryData.size();
        }

        vk::WriteDescriptorSet IndexStorageBufferdescriptorWrite{};
        IndexStorageBufferdescriptorWrite.dstSet = DescriptorSets[i];
        IndexStorageBufferdescriptorWrite.dstBinding = 13;
        IndexStorageBufferdescriptorWrite.dstArrayElement = 0;
        IndexStorageBufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
        IndexStorageBufferdescriptorWrite.descriptorCount = 1;
        IndexStorageBufferdescriptorWrite.pBufferInfo = &IndexStorageBuffersInfo;

        vk::DescriptorBufferInfo VertexStorageBuffersInfo{};
        if (!bufferManager->AllScene_VertexStorageBuffers.empty()) {
            VertexStorageBuffersInfo.buffer = bufferManager->AllScene_VertexStorageBuffers[0].buffer;
            VertexStorageBuffersInfo.range = sizeof(PaddedModelVertex) * bufferManager->AllScene_VertexGeometryData.size();
        }

        vk::WriteDescriptorSet VertexStorageBufferdescriptorWrite{};
        VertexStorageBufferdescriptorWrite.dstSet = DescriptorSets[i];
        VertexStorageBufferdescriptorWrite.dstBinding = 14;
        VertexStorageBufferdescriptorWrite.dstArrayElement = 0;
        VertexStorageBufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
        VertexStorageBufferdescriptorWrite.descriptorCount = 1;
        VertexStorageBufferdescriptorWrite.pBufferInfo = &VertexStorageBuffersInfo;

        vk::DescriptorBufferInfo OffsetStorageBuffersInfo{};
        if (!bufferManager->AllScene_OffsetStorageBuffers.empty()) {
            OffsetStorageBuffersInfo.buffer = bufferManager->AllScene_OffsetStorageBuffers[0].buffer;
            OffsetStorageBuffersInfo.range = sizeof(VertexAndIndexOffsets) * bufferManager->AllScene_VertexAndIndexOffsets.size();
        }

        vk::WriteDescriptorSet OffsetStorageBufferdescriptorWrite{};
        OffsetStorageBufferdescriptorWrite.dstSet = DescriptorSets[i];
        OffsetStorageBufferdescriptorWrite.dstBinding = 15;
        OffsetStorageBufferdescriptorWrite.dstArrayElement = 0;
        OffsetStorageBufferdescriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
        OffsetStorageBufferdescriptorWrite.descriptorCount = 1;
        OffsetStorageBufferdescriptorWrite.pBufferInfo = &OffsetStorageBuffersInfo;

        vk::DescriptorBufferInfo TransformUniformBuffersInfo{};
        TransformUniformBuffersInfo.buffer = bufferManager->AllScene_TransformationUniformBuffers[i].buffer;
        TransformUniformBuffersInfo.offset = 0;
        TransformUniformBuffersInfo.range = sizeof(GlobalTransformationMatrices) * 100;

        vk::WriteDescriptorSet TransformUniformBufferdescriptorWrite{};
        TransformUniformBufferdescriptorWrite.dstSet = DescriptorSets[i];
        TransformUniformBufferdescriptorWrite.dstBinding = 16;
        TransformUniformBufferdescriptorWrite.dstArrayElement = 0;
        TransformUniformBufferdescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
        TransformUniformBufferdescriptorWrite.descriptorCount = 1;
        TransformUniformBufferdescriptorWrite.pBufferInfo = &TransformUniformBuffersInfo;

        vk::DescriptorImageInfo PTGI_ResultingimageInfo{};
        PTGI_ResultingimageInfo.imageLayout = vk::ImageLayout::eGeneral;
        PTGI_ResultingimageInfo.imageView = PTGI_StorageImage.imageView;
        PTGI_ResultingimageInfo.sampler = PTGI_StorageImage.imageSampler;

        vk::WriteDescriptorSet PTGI_ResultingdescriptorWrite{};
        PTGI_ResultingdescriptorWrite.dstSet = DescriptorSets[i];
        PTGI_ResultingdescriptorWrite.dstBinding = 17;
        PTGI_ResultingdescriptorWrite.dstArrayElement = 0;
        PTGI_ResultingdescriptorWrite.descriptorType = vk::DescriptorType::eStorageImage;
        PTGI_ResultingdescriptorWrite.descriptorCount = 1;
        PTGI_ResultingdescriptorWrite.pImageInfo = &PTGI_ResultingimageInfo;

        std::array<vk::WriteDescriptorSet, 18> descriptorWrites = {
            PositionSamplerdescriptorWrite, NormalSamplerdescriptorWrite, AlbedoSamplerdescriptorWrite,
            MaterialsSamplerdescriptorWrite, ReflectiveCubeSamplerdescriptorWrite, EmisiveSamplerdescriptorWrite,
            ResultingdescriptorWrite, LightUniformBufferDescriptorWrite, TLAS_descriptorWrite,
            AssetImagSamplerdescriptorWrite, NormalAssetImagSamplerdescriptorWrite,
            MetalicRoughnessAssetImagSamplerdescriptorWrite, EmmisiveAssetImagSamplerdescriptorWrite,
            IndexStorageBufferdescriptorWrite, VertexStorageBufferdescriptorWrite,
			OffsetStorageBufferdescriptorWrite, TransformUniformBufferdescriptorWrite,PTGI_ResultingdescriptorWrite
        };

        vulkanContext->LogicalDevice.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
}


void Lighting_RTX::UpdateUniformBuffer(uint32_t currentImage, const std::vector<std::unique_ptr<Light>>& lightref)
{
    if (SkyBoxRef && SkyBoxRef->bSkyBoxUpdate)
    {
        UpdateDescrptorSets();
        SkyBoxRef->bSkyBoxUpdate = false;
    }

    std::vector<LightUniformData> lightDataspack;
    lightDataspack.reserve(lightref.size());

    size_t count = std::min(lightref.size(), (size_t)MAX_LIGHT_COUNT);

    for (size_t i = 0; i < count; i++)
    {
        if (lightref[i])
        {
            LightUniformData LightData;
            LightData.lightPositionAndLightType = glm::vec4(lightref[i]->position, lightref[i]->lightType);
            LightData.colorAndAmbientStrength = glm::vec4(lightref[i]->color, lightref[i]->ambientStrength);
            LightData.CameraPositionAndLightIntensity = glm::vec4(camera->GetPosition().x,
                camera->GetPosition().y,
                camera->GetPosition().z,
                lightref[i]->lightIntensity);
            lightDataspack.push_back(LightData);
        }
    }

    LightCount = static_cast<int>(lightDataspack.size());
    
    memset(UniformBuffersMappedMem[currentImage], 0, sizeof(LightUniformData) * MAX_LIGHT_COUNT);

    if (LightCount > 0) {
        memcpy(UniformBuffersMappedMem[currentImage], lightDataspack.data(), lightDataspack.size() * sizeof(LightUniformData));
    }
}

uint32_t Lighting_RTX::alignedSize(uint32_t value, uint32_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

void Lighting_RTX::Draw(BufferData RayGenBuffer, BufferData RayHitBuffer, BufferData RayMisBuffer, vk::CommandBuffer commandbuffer, vk::PipelineLayout pipelinelayout, uint32_t imageIndex)
{
    vk::BufferDeviceAddressInfo raygenShaderBindingTableDeviceAdressesInfo;
    raygenShaderBindingTableDeviceAdressesInfo.buffer = RayGenBuffer.buffer;

    vk::BufferDeviceAddressInfo missShaderBindingTableDeviceAdressesInfo;
    missShaderBindingTableDeviceAdressesInfo.buffer = RayMisBuffer.buffer;

    vk::BufferDeviceAddressInfo hitShaderBindingTableDeviceAdressesInfo;
    hitShaderBindingTableDeviceAdressesInfo.buffer = RayHitBuffer.buffer;

    auto raygenShaderBindingTableAdress = vulkanContext->LogicalDevice.getBufferAddress(raygenShaderBindingTableDeviceAdressesInfo);
    auto missShaderBindingTableAdress = vulkanContext->LogicalDevice.getBufferAddress(missShaderBindingTableDeviceAdressesInfo);
    auto hitShaderBindingTableAdress = vulkanContext->LogicalDevice.getBufferAddress(hitShaderBindingTableDeviceAdressesInfo);

    const uint32_t handleSizeAligned = alignedSize(
        vulkanContext->RayTracingPipelineProperties.shaderGroupHandleSize,
        vulkanContext->RayTracingPipelineProperties.shaderGroupHandleAlignment);

    vk::StridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
    raygenShaderSbtEntry.deviceAddress = raygenShaderBindingTableAdress;
    raygenShaderSbtEntry.stride = handleSizeAligned;
    raygenShaderSbtEntry.size = handleSizeAligned;

    vk::StridedDeviceAddressRegionKHR missShaderSbtEntry{};
    missShaderSbtEntry.deviceAddress = missShaderBindingTableAdress;
    missShaderSbtEntry.stride = handleSizeAligned;
    missShaderSbtEntry.size = handleSizeAligned;

    vk::StridedDeviceAddressRegionKHR hitShaderSbtEntry{};
    hitShaderSbtEntry.deviceAddress = hitShaderBindingTableAdress;
    hitShaderSbtEntry.stride = handleSizeAligned;
    hitShaderSbtEntry.size = handleSizeAligned;

    vk::StridedDeviceAddressRegionKHR callableShaderSbtEntry{};

    VkStridedDeviceAddressRegionKHR  TEMP_raygenShaderSbtEntry = static_cast<VkStridedDeviceAddressRegionKHR>(raygenShaderSbtEntry);
    VkStridedDeviceAddressRegionKHR  TEMP_missShaderSbtEntry = static_cast<VkStridedDeviceAddressRegionKHR>(missShaderSbtEntry);;
    VkStridedDeviceAddressRegionKHR  TEMP_hitShaderSbtEntry = static_cast<VkStridedDeviceAddressRegionKHR>(hitShaderSbtEntry);;
    VkStridedDeviceAddressRegionKHR  TEMP_callableShaderSbtEntry = static_cast<VkStridedDeviceAddressRegionKHR>(callableShaderSbtEntry);;


    int width = vulkanContext->swapchainExtent.width;
    int height = vulkanContext->swapchainExtent.height;
    int depth = 1;

    Lightin_RTX_PC pc;
    pc.LightCount = LightCount;
    pc.ScreenSize = glm::vec2(width, height);
    pc.FrameIndex = vulkanContext->frameIndex;
    pc.GI_Solution_Index_Padding = glm::vec4(GISolutionIndex, 0, 0, 0);
    commandbuffer.pushConstants(pipelinelayout, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR, 0, sizeof(Lightin_RTX_PC), &pc);
    commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, pipelinelayout, 0, 1, &DescriptorSets[imageIndex], 0, nullptr);

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

void Lighting_RTX::CleanUp()
{
    if (bufferManager)
    {
        for (auto& RayGen_Buffer : UniformBuffers)
        {
            if (RayGen_Buffer.buffer)
            {
                bufferManager->UnmapMemory(RayGen_Buffer);
                bufferManager->DestroyBuffer(RayGen_Buffer);
            }
        }

        if (vulkanContext && vulkanContext->LogicalDevice) {
            vulkanContext->LogicalDevice.destroyDescriptorSetLayout(descriptorSetLayout);
        }

        UniformBuffers.clear();
    }
}