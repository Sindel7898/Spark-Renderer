#version 450

layout (binding = 0) uniform sampler2D LightingReflectionTexture;
layout (binding = 1) uniform sampler2D GITexture;                

layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outFragColor;


void main() {
    vec3 directLight = texture(LightingReflectionTexture, inTexCoord).rgb;
    vec3 gi = texture(GITexture, inTexCoord).rgb;

    vec3 hdrColor = directLight + gi;

    outFragColor = vec4(hdrColor, 1.0); 
}
