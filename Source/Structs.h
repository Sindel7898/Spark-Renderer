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