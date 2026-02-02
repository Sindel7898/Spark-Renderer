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
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outViewSpaceNormal;
layout(location = 3) out vec4 outAlbedo;
layout(location = 4) out vec4 outEmissive;
layout(location = 5) out vec4 outMetallicRoughnessAO;
layout(location = 6) out vec4 outVelocity;
layout(location = 7) out vec4 outSpecularAlbedo;

vec3 EnvBRDFApprox2(vec3 SpecularColor, float alpha, float NoV)
{
 NoV = abs(NoV);
 // [Ray Tracing Gems, Chapter 32]
 vec4 X;
 X.x = 1.f;
 X.y = NoV;
 X.z = NoV * NoV;
 X.w = NoV * X.z;
 vec4 Y;
 Y.x = 1.f;
 Y.y = alpha;
 Y.z = alpha * alpha;
 Y.w = alpha * Y.z;
 
 mat2 M1 = mat2(0.99044f, -1.28514f, 1.29678f, -0.755907f);
 mat3 M2 = mat3(1.f, 2.92338f, 59.4188f, 20.3225f, -27.0302f,222.592f, 121.563f, 626.13f, 316.627f);
 mat2 M3 = mat2(0.0365463f, 3.32707, 9.0632f, -9.04756);
 mat3 M4 = mat3(1.f, 3.59685f, -1.36772f, 9.04401f, -16.3174f,9.22949f, 5.56589f, 19.7886f, -20.2123f);

 float bias  = dot(M1 * X.xy, Y.xy) * (1.0 / dot(M2 * X.xyw, Y.xyw));
 float scale = dot(M3 * X.xy, Y.xy) * (1.0  /dot(M4 * X.xzw, Y.xyw));
 // This is a hack for specular reflectance of 0
bias *= clamp(SpecularColor.g * 50.0, 0.0, 1.0);
return SpecularColor * max(0.0, scale) + max(0.0, bias);
}

void main()
{
    // =======================
    // Positions
    // =======================
    outWorldPosition     = inWorldPos;
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
    vec2  mr = textureLod(samplerMetallicRoughness, inTexCoord, 0).rg;
    float ao = textureLod(samplerAO, inTexCoord, 0).b;

    outMetallicRoughnessAO = vec4(mr, ao, 1.0);
    outAlbedo              = vec4(textureLod(samplerColor, inTexCoord, 0).rgb, 1.0);
    outEmissive            = vec4(textureLod(samplerEmisive, inTexCoord, 0).rgb * 3.0, 1.0);

    // =======================
    // Motion vectors
    // =======================
    vec3 UV      = (inCurrClipPos.xyz / inCurrClipPos.w).xyz * 0.5 + 0.5;
    vec3 PrevUV  = (inPrevClipPos.xyz / inPrevClipPos.w).xyz * 0.5 + 0.5;

    outVelocity = vec4((UV - PrevUV).xyz, 1.0);


    // =======================
    vec3 Albedo     = textureLod(samplerColor, inTexCoord, 0).rgb;
    float Metallic  = mr.r;
    float Roughness = mr.g;
    vec3 F0         = mix(vec3(0.04), Albedo, Metallic);

    vec3 V      = normalize(-inViewPos.xyz);
    float NdotV = max(dot(viewNormal, V), 0.0001);

    outSpecularAlbedo = vec4(EnvBRDFApprox2(F0, Roughness * Roughness, NdotV), 1.0);
}
