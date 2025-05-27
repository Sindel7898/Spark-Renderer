#pragma once

struct Vertex {
    glm::vec2 position;
    glm::vec2 uv;
};

struct GBuffer {
    ImageData Position;
    ImageData Normal;
    ImageData SSAO;
    ImageData Albedo;
};

