#version 450

layout (binding = 0) uniform sampler2D Lighting_ReflectionTexture;
layout (binding = 1) uniform sampler2D ShadowTexture;

layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outFragColor;

void main() {
    vec3 litImage = texture(Lighting_ReflectionTexture, inTexCoord).rgb;
    float shadowFactor = texture(ShadowTexture, inTexCoord).r;

    vec3 finalImage = litImage * shadowFactor;

    outFragColor = vec4(finalImage, 1.0);
}
