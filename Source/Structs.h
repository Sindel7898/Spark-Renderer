#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/fwd.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <vk_mem_alloc.h>

struct IDdata {
    int instance;
    // bool IsActive; // not needed anymore 
};

struct BufferData {

    std::string BufferID;
    vk::Buffer buffer{};
    vk::DeviceSize size{};
    vk::BufferUsageFlags usage{};
    VmaAllocation allocation{};
};

struct ImageData {
    std::string ImageID;
    vk::Image image{};
    vk::ImageView imageView{};
    vk::Sampler imageSampler{};
    VmaAllocation allocation{};
    uint32_t  miplevels = 1;
};


struct ImageTransitionData {
    vk::ImageLayout oldlayout{};
    vk::ImageLayout newlayout{};
    vk::AccessFlags SourceAccessflag = vk::AccessFlagBits::eNone;
    vk::AccessFlags DestinationAccessflag = vk::AccessFlagBits::eNone;
    vk::PipelineStageFlags SourceOnThePipeline = vk::PipelineStageFlagBits::eNone;
    vk::PipelineStageFlags DestinationOnThePipeline = vk::PipelineStageFlagBits::eNone;
    vk::ImageAspectFlags AspectFlag = vk::ImageAspectFlagBits::eColor;
    int BaseMipLevel = 0;
    int LevelCount = 1;
    int LayerCount = 1;

};

struct VertexAndIndexOffsets {

    uint32_t VertexOffset;
    uint32_t IndexOffset;
};

struct PaddedModelVertex {

    glm::vec4 vert_Padding;
    glm::vec4 text_Padding;
    glm::vec4 normal_Padding;
    glm::vec4 tangent_Padding;
};

struct Vertex {
    glm::vec2 position;
    glm::vec2 uv;
};

struct GBuffer {
    ImageData Position;
    ImageData ViewSpacePosition;
    ImageData Normal;
    ImageData ViewSpaceNormal;
    ImageData Materials;
    ImageData Albedo;
    ImageData Emissive;

};

struct TransformMatrices {
    alignas(16) glm::mat4 modelMatrix;
    alignas(16) glm::mat4 viewMatrix;
    alignas(16) glm::mat4 projectionMatrix;
};

struct InstanceTransformMatrices {
    alignas(16) glm::mat4 viewMatrix;
    alignas(16) glm::mat4 projectionMatrix;
};

struct VertexUniformData
{
    TransformMatrices  transformMatrices;
    glm::mat4 LightViewMatrix;
    glm::mat4 LightProjectionMatrix;

};

struct alignas(16) LightUniformData {
    alignas(16) glm::vec4  lightPositionAndLightType;
    alignas(16) glm::vec4  colorAndAmbientStrength;
    alignas(16) glm::vec4  CameraPositionAndLightIntensity;
};
