#pragma once

struct Vertex {
    glm::vec2 position;
    glm::vec2 uv;
};

struct GBuffer {
    ImageData Position;
    ImageData LightSpacePosition;
    ImageData Normal;
    ImageData Albedo;
};

