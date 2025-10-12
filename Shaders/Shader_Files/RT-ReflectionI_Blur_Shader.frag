#version 450

layout (binding = 0) uniform sampler2D samplerViewSpacePosition;
layout (binding = 1) uniform sampler2D samplerReflections;
layout (binding = 2) uniform sampler2D samplerMaterial;

layout (location = 0) in vec2 inTexCoord;           
layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConsts {
    int Direction; // 0 = horizontal, 1 = vertical
} pc;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(samplerReflections, 0));

    vec3 color      = textureLod(samplerReflections, inTexCoord,0).rgb;
    float depth     = textureLod(samplerViewSpacePosition, inTexCoord,0).b;
    float roughness = textureLod(samplerMaterial, inTexCoord,0).g;

    float blurStrength = mix(0.0, 2.5, roughness);   
    int blurRadius     = int(mix(0.0, 5, roughness)); 

    vec3 result = vec3(0.0);
    float totalWeight = 0.0;

    float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);


    bool horizontal = (pc.Direction == 0);

    float mainDepth = textureLod(samplerViewSpacePosition, inTexCoord,0).b;
    float depthScale = clamp(abs(mainDepth) * 0.05, 0.01, 1.2);

    for (int i = -blurRadius; i <= blurRadius; i++) {
        
        int idx = min(abs(i), 9); 

        vec2 offset = horizontal
            ? vec2(texelSize.x * i * blurStrength, 0.0)
            : vec2(0.0, texelSize.y * i * blurStrength);

        float offsetDepth = textureLod(samplerViewSpacePosition, inTexCoord + offset,0).b;
        float w = weights[idx];

        float depthDiff = abs(mainDepth - offsetDepth);

        if (depthDiff < depthScale) {
            result += textureLod(samplerReflections, inTexCoord + offset,0).rgb * w;
            totalWeight += w;
        }
    }

    if (totalWeight > 0.0)
        result /= totalWeight;

    outFragColor = vec4(color, 1.0);
}
