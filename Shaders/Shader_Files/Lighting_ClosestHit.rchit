#version 460
#extension GL_GOOGLE_include_directive : require
#include "DDGI_Include.glsl"

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_ray_query : require 

const float PI = 3.14159265359;

layout(set = 0, binding = 9)  uniform sampler2D          Albedo_AssetImages[];
layout(set = 0, binding = 10)  uniform sampler2D         Normal_AssetImages[];
layout(set = 0, binding = 11)  uniform sampler2D         MetalicRoughness_AssetImages[];
layout(set = 0, binding = 12)  uniform sampler2D         Emmisive_AssetImages[];

struct Vertex {
    vec4 position_Padding;
    vec4 texCoord_Padding;
    vec4 normal_Padding;
    vec4 tangent_Padding;
};

struct VertexAndIndexOffsets {
    uint VertexOffset;
    uint IndexOffset;
    uint MaterialIndex;
    uint padding;
};

layout(set = 0, binding = 13) buffer IndexBufferSSBO {
    uint Indices[];
} indexBuffer;

layout(set = 0, binding = 14) buffer VertexBufferSSBO {
    Vertex vertices[];
} vertexBuffer;

layout(set = 0, binding = 15) buffer VertexIndexOffsetBufferSSBO {
    VertexAndIndexOffsets Offsets[];
} OffsetBuffer;

struct Transformations {
    mat4 WorldMatrix;
    mat4 Inverese_Transposed_WorldMatrix;
};

layout(set = 0, binding = 16) uniform Transformation {
    Transformations transformations[100];
};

struct LightData{
    vec4    positionAndLightType;
    vec4    colorAndAmbientStrength;
    vec4    CameraPositionAndLightIntensity;

};

layout(set = 0, binding = 7) uniform LightUniformBuffer {
         LightData lights[4];
};

layout(binding  = 8) uniform accelerationStructureEXT topLevelAS;

layout(push_constant) uniform PushConstant{
    vec2   ScreenSize;
	int    LightCount;
	int    FrameIndex;
    vec4   GI_Solution_Padding;
}PC;

struct RTGIPayload {
       vec4 normal;
       vec4 radiance;
       vec4 position;
       vec4 albedo_Hit;
};

struct ShadowPayload {
       int visibility;
};

layout(location = 0) rayPayloadEXT  ShadowPayload shadowPayload;
layout(location = 1) rayPayloadInEXT RTGIPayload payload;

hitAttributeEXT vec2 attribs;


void main()
{
    vec3  Radiance    = vec3(0.0);
    vec3  HitNormal   = vec3(0.0);
    vec3  HitPosition = vec3(0.0);
    float Distance    = 0;

    uint packed = gl_InstanceCustomIndexEXT;
    uint meshBufferID = packed & 0xFFF;
    uint objectID   = packed >> 12;

    VertexAndIndexOffsets offsets = OffsetBuffer.Offsets[meshBufferID];
    uint baseIndex = offsets.IndexOffset + (3 * gl_PrimitiveID);
    
    uint i0 = indexBuffer.Indices[baseIndex + 0];
    uint i1 = indexBuffer.Indices[baseIndex + 1];
    uint i2 = indexBuffer.Indices[baseIndex + 2];
    
    // Use the indices to get actual vertex data
    Vertex v0 = vertexBuffer.vertices[i0];
    Vertex v1 = vertexBuffer.vertices[i1];
    Vertex v2 = vertexBuffer.vertices[i2];


    vec3 bary = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    
    vec3 VertexPosition = 
        v0.position_Padding.xyz * bary.x +
        v1.position_Padding.xyz * bary.y +
        v2.position_Padding.xyz * bary.z;
    
    vec3 Normal = normalize(
        v0.normal_Padding.xyz * bary.x +
        v1.normal_Padding.xyz * bary.y +
        v2.normal_Padding.xyz * bary.z
    );
    
    vec3 Tangent = normalize(
        v0.tangent_Padding.xyz * bary.x +
        v1.tangent_Padding.xyz * bary.y +
        v2.tangent_Padding.xyz * bary.z
    );
    

  vec2 TexCoord = 
    v0.texCoord_Padding.xy * bary.x +
    v1.texCoord_Padding.xy * bary.y +
    v2.texCoord_Padding.xy * bary.z;
    
     mat3 normalMatrix  = mat3(transformations[objectID].Inverese_Transposed_WorldMatrix);

     vec3 WorldN        = normalize(normalMatrix * Normal);
     vec3 WorldT        = normalize(normalMatrix * Tangent);
     vec3 WorldB        = cross(WorldN, WorldT);
     mat3 WorldSpaceTBN = mat3(WorldT, WorldB, WorldN);
    
      uint matID = offsets.MaterialIndex;
     vec3  Albedo     = texture(Albedo_AssetImages          [nonuniformEXT(matID)], TexCoord).rgb;
     float Metallic   = texture(MetalicRoughness_AssetImages[nonuniformEXT(matID)], TexCoord).r;
     float Roughness  = texture(MetalicRoughness_AssetImages[nonuniformEXT(matID)], TexCoord).r;
     vec3  Emissive   = texture(Emmisive_AssetImages        [nonuniformEXT(matID)], TexCoord).rgb;

     vec3 textureMap = texture(Normal_AssetImages[nonuniformEXT(matID)], TexCoord).rgb;
    
     vec3 NormalTexture = textureMap * 2.0 - vec3(1.0);
     HitNormal = normalize(WorldSpaceTBN * NormalTexture);
 
     vec4 WorldPos =  transformations[objectID].WorldMatrix * vec4(VertexPosition,1.0);
     HitPosition   =  WorldPos.xyz;

 
vec3 directLighting = vec3(0.0);

    for (int i = 0; i < PC.LightCount; i++) {
        LightData light = lights[i];

        const float Constant = 1.0;
        const float Linear = 0.09;
        const float Quadratic = 0.032;

        vec3 Radiance = vec3(0.0);
        vec3 LightDir = vec3(0.0);
        vec3 Lo = vec3(0.0);
        float tMax = 10000.0;
        float Distance = 0.0; 

        if (light.positionAndLightType.w < 0.5) {
           LightDir = normalize(-light.positionAndLightType.xyz);
           Radiance = light.colorAndAmbientStrength.rgb;
        } else if (light.positionAndLightType.w > 0.5) {
           vec3 LightPos = light.positionAndLightType.xyz;
           LightDir = normalize(LightPos - HitPosition);
           Distance = length(LightPos - HitPosition);
           float Attenuation = 1.0 / (Constant + Linear * Distance + Quadratic * (Distance * Distance));
           Radiance = light.colorAndAmbientStrength.rgb * Attenuation;
           tMax = Distance;
        }  
    
        vec3 diffuse = Albedo / PI;
        float NdotL = max(dot(HitNormal, LightDir), 0.0);        
        Lo += (diffuse) * Radiance * NdotL;

        vec3 shadowOrigin = HitPosition + (HitNormal * 0.001); 
    
        shadowPayload.visibility = 0; 

        traceRayEXT(
            topLevelAS, 
            gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 
            0xFF, 
            0, 0, 0, 
            shadowOrigin, 
            0.001, 
            LightDir, 
            tMax, 
            0 
        );

        directLighting += (Lo * lights[i].CameraPositionAndLightIntensity.a) * shadowPayload.visibility;
    }

    payload.radiance = vec4(directLighting + Emissive,0); 
    payload.normal   = vec4(HitNormal.xyz,0);
    payload.position = vec4(HitPosition,0);
    payload.albedo   = vec4(Albedo,1);
}