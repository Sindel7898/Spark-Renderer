#version 450

layout(binding = 2) uniform sampler2D samplerColor;
layout(binding = 3) uniform sampler2D samplerNormal;
layout(binding = 4) uniform sampler2D samplerMetallicRoughness;
layout(binding = 5) uniform sampler2D samplerAO;
layout(binding = 6) uniform sampler2D samplerEmisive;


layout(location = 0) in vec4 inWorldPos;
layout(location = 1) in vec4 inViewPos;
layout(location = 2) in vec2 inTexCoord;

layout(location = 3) in mat3 inWorldTBN;
layout(location = 6) in mat3 inViewTBN;

layout(location = 9)  in vec4 inCurrClipPos;
layout(location = 10) in vec4 inPrevClipPos;


layout(location = 0) out vec4 outWorldPosition;
layout(location = 1) out vec4 outViewSpacePosition;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outViewSpaceNormal;
layout(location = 4) out vec4 outAlbedo;
layout(location = 5) out vec4 outEmissive;
layout(location = 6) out vec4 outMetallicRoughnessAO;
layout(location = 7) out vec4 outVelocity;

void main()
{
    // =======================
    // Positions
    // =======================
    outWorldPosition     = inWorldPos;
    outViewSpacePosition = inViewPos;

    // =======================
    // Normals
    // =======================
    vec3 normalTex = textureLod(samplerNormal, inTexCoord, 0).rgb * 2.0 - 1.0;

    vec3 worldNormal = normalize(inWorldTBN * normalTex);
    vec3 viewNormal  = normalize(inViewTBN  * normalTex);

    outNormal          = vec4(worldNormal, 1.0);
    outViewSpaceNormal = vec4(viewNormal,  1.0);

    // =======================
    // Material
    // =======================
    vec2 mr = textureLod(samplerMetallicRoughness, inTexCoord, 0).rg;
    float ao = textureLod(samplerAO, inTexCoord, 0).b;

    outMetallicRoughnessAO = vec4(mr, ao, 1.0);
    outAlbedo              = vec4(textureLod(samplerColor, inTexCoord, 0).rgb, 1.0);
    outEmissive            = vec4(textureLod(samplerEmisive, inTexCoord, 0).rgb * 3.0, 1.0);

    // =======================
    // Motion vectors
    // =======================
    vec3 NDCPos = (inCurrClipPos.xyz / inCurrClipPos.w).xyz;
    vec3 PrevNDCPos  = (inPrevClipPos.xyz / inPrevClipPos.w).xyz;

    outVelocity = vec4((NDCPos - PrevNDCPos).xyz, 1.0);
}
