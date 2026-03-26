#version 460
#extension GL_GOOGLE_include_directive : require
#include "DDGI_Include.glsl"

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_ray_query : require 

const float PI = 3.14159265359;

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 0, binding = 1)  uniform sampler2D          Albedo_AssetImages[];
layout(set = 0, binding = 2)  uniform sampler2D          Normal_AssetImages[];
layout(set = 0, binding = 3)  uniform sampler2D          MetalicRoughness_AssetImages[];
layout(set = 0, binding = 15) uniform sampler2D          Emmisive_AssetImages[];

layout(set = 0, binding = 13) uniform sampler2D  IrradianceStorageImage;
layout(set = 0, binding = 14) uniform sampler2D  VisibilityStorageImage;

struct Vertex { vec4 position_Padding; vec4 texCoord_Padding; vec4 normal_Padding; vec4 tangent_Padding; };
struct VertexAndIndexOffsets { uint VertexOffset; uint IndexOffset; uint MaterialIndex; uint padding; };
layout(set = 0, binding = 4) buffer IndexBufferSSBO { uint Indices[]; } indexBuffer;
layout(set = 0, binding = 5) buffer VertexBufferSSBO { Vertex vertices[]; } vertexBuffer;
layout(set = 0, binding = 6) buffer VertexIndexOffsetBufferSSBO { VertexAndIndexOffsets Offsets[]; } OffsetBuffer;

struct Transformations { mat4 WorldMatrix; mat4 Inverese_Transposed_WorldMatrix; };
layout(set = 0, binding = 7) uniform Transformation { Transformations transformations[100]; };

struct LightData { vec4 positionAndLightType; vec4 colorAndAmbientStrength; vec4 CameraPositionAndLightIntensity; };
layout(set = 0, binding = 11) uniform LightUniformBuffer { LightData lights[1000]; };

layout(push_constant) uniform PushConstant {
    vec4 GridBaseLocation_ScreenSizeWidth;
    vec4 ProbeSpacing_ScreenSizeHeight;
    vec4 ProbeCount;
    int  AtlasWidthSize;  
    int  ProbeSideLength; 
    int  GutterSize;      
    int  RaysPerProbe;
    vec4 UseInfiniteBounce_infinite_bounces_multiplier_DDGIMODE_LightCount;
} pc;

struct Payload { vec3 Color; float Distance; vec4 Hit_Padding; };
struct Shadow_Payload { int Shadow; };

layout(location = 0) rayPayloadInEXT Payload payload;
layout(location = 1) rayPayloadEXT Shadow_Payload shadow_Payload;

hitAttributeEXT vec2 attribs;

void main()
{
    vec3  HitNormal   = vec3(0.0);
    vec3  HitPosition = vec3(0.0);
    float Distance    = 0;

    vec3 Le_Emissive  = vec3(0.0);
    vec3 L_Direct     = vec3(0.0);
    vec3 L_Indirect   = vec3(0.0);

     if (gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT) {
        Distance = gl_RayTminEXT + gl_HitTEXT;
        Distance *= -0.1;        
    }
    else {

        uint packed = gl_InstanceCustomIndexEXT;
        uint meshBufferID = packed & 0xFFF;
        uint objectID   = packed >> 12;

        VertexAndIndexOffsets offsets = OffsetBuffer.Offsets[meshBufferID];
        uint baseIndex = offsets.IndexOffset + (3 * gl_PrimitiveID);
        uint i0 = indexBuffer.Indices[baseIndex + 0];
        uint i1 = indexBuffer.Indices[baseIndex + 1];
        uint i2 = indexBuffer.Indices[baseIndex + 2];
        Vertex v0 = vertexBuffer.vertices[i0];
        Vertex v1 = vertexBuffer.vertices[i1];
        Vertex v2 = vertexBuffer.vertices[i2];

        vec3 bary = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
        vec3 VertexPosition = v0.position_Padding.xyz * bary.x + v1.position_Padding.xyz * bary.y + v2.position_Padding.xyz * bary.z;
        vec3 Normal = normalize(v0.normal_Padding.xyz * bary.x + v1.normal_Padding.xyz * bary.y + v2.normal_Padding.xyz * bary.z);
        vec3 Tangent = normalize(v0.tangent_Padding.xyz * bary.x + v1.tangent_Padding.xyz * bary.y + v2.tangent_Padding.xyz * bary.z);
        vec2 TexCoord = v0.texCoord_Padding.xy * bary.x + v1.texCoord_Padding.xy * bary.y + v2.texCoord_Padding.xy * bary.z;
        
        mat3 normalMatrix  = mat3(transformations[objectID].Inverese_Transposed_WorldMatrix);
        vec3 WorldN        = normalize(normalMatrix * Normal);
        vec3 WorldT        = normalize(normalMatrix * Tangent);
        vec3 WorldB        = cross(WorldN, WorldT);
        mat3 WorldSpaceTBN = mat3(WorldT, WorldB, WorldN);
        
        uint matID = offsets.MaterialIndex;
        vec3  Albedo     = texture(Albedo_AssetImages[nonuniformEXT(matID)], TexCoord).rgb;
        vec3  EmissiveTex= texture(Emmisive_AssetImages[nonuniformEXT(matID)], TexCoord).rgb;
        
        vec3 textureMap = texture(Normal_AssetImages[nonuniformEXT(matID)], TexCoord).rgb;
        vec3 NormalTexture = textureMap * 2.0 - vec3(1.0);
        HitNormal = normalize(WorldSpaceTBN * NormalTexture);
        vec4 WorldPos = transformations[objectID].WorldMatrix * vec4(VertexPosition,1.0);
        HitPosition   = WorldPos.xyz;


        Le_Emissive = EmissiveTex;


        for (int i = 0; i < pc.UseInfiniteBounce_infinite_bounces_multiplier_DDGIMODE_LightCount.w; i++) {
            LightData light = lights[i];
            vec3 LightDir;
            float lightDist;
            vec3 radiance;

            if(light.positionAndLightType.w < 0.5) { // Directional
                LightDir = normalize(-light.positionAndLightType.xyz);
                radiance = light.colorAndAmbientStrength.rgb;
                lightDist = 10000.0;
            } else { // Point
                vec3 lightVec = light.positionAndLightType.xyz - WorldPos.xyz;
                LightDir = normalize(lightVec);
                lightDist = length(lightVec);

                float att = 1.0 / (1.0 + 0.09 * lightDist + 0.032 * lightDist * lightDist);
                radiance = light.colorAndAmbientStrength.rgb * att;
            }

            float NdotL = max(dot(HitNormal, LightDir), 0.0);
            
            if (NdotL > 0.0) {

                vec3 shadowOrigin = HitPosition + (WorldN * 0.05);
                shadow_Payload.Shadow = 0;
                traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 0, 0, 1, shadowOrigin, 0.0, LightDir, lightDist, 1);
                
                if(int(pc.UseInfiniteBounce_infinite_bounces_multiplier_DDGIMODE_LightCount.z) == 0){
                  L_Direct  += (Albedo / PI)  * NdotL * shadow_Payload.Shadow * radiance *  light.CameraPositionAndLightIntensity.a;
                }else{
                    L_Direct += NdotL * shadow_Payload.Shadow * radiance *  light.CameraPositionAndLightIntensity.a;
                  //L_Direct  += (Albedo / PI)  * NdotL * shadow_Payload.Shadow * radiance *  light.CameraPositionAndLightIntensity.a;

                }                
            }
        }

        
        int UseInfiniteBounce = int(pc.UseInfiniteBounce_infinite_bounces_multiplier_DDGIMODE_LightCount.x);
        if(UseInfiniteBounce > 0.5) {
             vec3 BiasedPos = HitPosition + (WorldN * 0.5);
             vec3 CameraPos = lights[0].CameraPositionAndLightIntensity.xyz;

             vec3 Irradiance = SampleIrradiance(
                 IrradianceStorageImage, VisibilityStorageImage, BiasedPos, WorldN,
                 pc.GridBaseLocation_ScreenSizeWidth.xyz, pc.ProbeSpacing_ScreenSizeHeight.xyz,
                 ivec3(pc.ProbeCount.xyz), pc.ProbeSideLength, pc.GutterSize, pc.AtlasWidthSize,
                 pc.ProbeSideLength, pc.GutterSize, pc.AtlasWidthSize, CameraPos
             );

             L_Indirect = (Albedo / PI) * Irradiance  * pc.UseInfiniteBounce_infinite_bounces_multiplier_DDGIMODE_LightCount.y;
        }

        Distance = gl_RayTminEXT + gl_HitTEXT;
    }

    
    payload.Color = Le_Emissive + L_Direct + L_Indirect;
    
    payload.Distance = Distance;
    payload.Hit_Padding = vec4(1,0,0,0);
}