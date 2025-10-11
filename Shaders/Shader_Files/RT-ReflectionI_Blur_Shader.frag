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

    vec3 color      = texture(samplerReflections, inTexCoord).rgb;
    float depth     = texture(samplerViewSpacePosition, inTexCoord).b;
    float roughness = texture(samplerMaterial, inTexCoord).r;

    // --- Blur modulation based on roughness ---
    float blurStrength = mix(0.25, 2.5, roughness);   // step size per sample
    int blurRadius     = int(mix(2.0, 10.0, roughness)); // number of samples each side

    vec3 result = vec3(0.0);
    float totalWeight = 0.0;

    // --- 10-tap Gaussian weights (symmetric) ---
    float weights[10] = float[](
        0.196482,  // 0
        0.176032,  // 1
        0.146356,  // 2
        0.111280,  // 3
        0.075026,  // 4
        0.043546,  // 5
        0.021937,  // 6
        0.009745,  // 7
        0.003993,  // 8
        0.001396   // 9
    );

    bool horizontal = (pc.Direction == 0);

    float mainDepth = texture(samplerViewSpacePosition, inTexCoord).b;
    float depthScale = clamp(abs(mainDepth) * 0.05, 0.01, 1.2);

    // --- Variable-radius Gaussian blur ---
    for (int i = -blurRadius; i <= blurRadius; i++) {
        int idx = min(abs(i), 9); // clamp to max defined weight
        vec2 offset = horizontal
            ? vec2(texelSize.x * i * blurStrength, 0.0)
            : vec2(0.0, texelSize.y * i * blurStrength);

        float offsetDepth = texture(samplerViewSpacePosition, inTexCoord + offset).b;
        float w = weights[idx];

        float depthDiff = abs(mainDepth - offsetDepth);
        if (depthDiff < depthScale) {
            result += texture(samplerReflections, inTexCoord + offset).rgb * w;
            totalWeight += w;
        }
    }

    if (totalWeight > 0.0)
        result /= totalWeight;

    outFragColor = vec4(result, 1.0);
}
