#version 450

layout(binding = 0)  uniform sampler2D IrradianceAtlas;

layout(location = 0) out vec4 outColor;


layout(location = 0) in flat int  ProbeIndex;
layout(location = 1) in vec2 TexCoord;

void main() {

    vec4 probe_data = texture(IrradianceAtlas, TexCoord);
    outColor = vec4(probe_data.xyz,1);
}