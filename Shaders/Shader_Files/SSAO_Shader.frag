#version 450

layout (binding = 0) uniform sampler2D samplerposition;
layout (binding = 1) uniform sampler2D samplerNormal;

layout(location = 0) in vec2 inTexCoord;           


layout (location = 0) out float outSSAO_Color;

void main() {

    outSSAO_Color = 0.7f;
}