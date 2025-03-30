#version 450

layout(location = 0) in vec2 fragTexCoord;


layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform samplerCube skybox;

layout(location = 0) out vec4 outColor;

void main() {
  
    outColor = texture(texSampler, fragTexCoord);
}