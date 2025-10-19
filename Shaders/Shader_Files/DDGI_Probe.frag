#version 450

layout(binding = 0)  uniform sampler2D IrradianceAtlas;

layout(location = 0) out vec4 outColor;

void main() {

    outColor = vec4(1);
}