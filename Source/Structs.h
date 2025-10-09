#pragma once
#include "Drawable.h"

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
