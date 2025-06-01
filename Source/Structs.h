#pragma once

struct Vertex {
    glm::vec2 position;
    glm::vec2 uv;
};

struct GBuffer {
    ImageData Position;
    ImageData ViewSpacePosition;
    ImageData Normal;
    ImageData ViewSpaceNormal;
    ImageData SSAO;
    ImageData SSAOBlured;
    ImageData Materials;
    ImageData Albedo;
};

