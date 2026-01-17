#version 460
#extension GL_GOOGLE_include_directive : require
#include "DDGI_Include.glsl"

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_ray_query : require 

const float PI = 3.14159265359;

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;

layout(set = 0, binding = 1)  uniform sampler2D         Albedo_AssetImages[];
layout(set = 0, binding = 2)  uniform sampler2D         Normal_AssetImages[];
layout(set = 0, binding = 3)  uniform sampler2D         MetalicRoughness_AssetImages[];
layout(set = 0, binding = 15) uniform sampler2D         Emmisive_AssetImages[];

layout(set = 0, binding = 13) uniform sampler2D  IrradianceStorageImage;
layout(set = 0, binding = 14) uniform sampler2D  VisibilityStorageImage;

struct Vertex {
    vec4 position_Padding;
    vec4 texCoord_Padding;
    vec4 normal_Padding;
    vec4 tangent_Padding;
};

struct VertexAndIndexOffsets {
    uint VertexOffset;
    uint IndexOffset;
};

layout(set = 0, binding = 4) buffer IndexBufferSSBO {
    uint Indices[];
} indexBuffer;

layout(set = 0, binding = 5) buffer VertexBufferSSBO {
    Vertex vertices[];
} vertexBuffer;

layout(set = 0, binding = 6) buffer VertexIndexOffsetBufferSSBO {
    VertexAndIndexOffsets Offsets[];
} OffsetBuffer;

struct Transformations {
    mat4 WorldMatrix;
    mat4 Inverese_Transposed_WorldMatrix;
};

layout(set = 0, binding = 7) uniform Transformation {
    Transformations transformations[100];
};

struct LightData {
    vec4  positionAndLightType;
    vec4  colorAndAmbientStrength;
    vec4  CameraPositionAndLightIntensity;
};

layout(set = 0, binding = 11) uniform LightUniformBuffer {
    LightData lights[1000];
};

layout(push_constant) uniform PushConstant {
    vec4 GridBaseLocation_ScreenSizeWidth;
    vec4 ProbeSpacing_ScreenSizeHeight;
    vec4 ProbeCount;
    int   AtlasWidthSize;  
    int   ProbeSideLength; 
    int   GutterSize;      
    int   NumRays;
    vec4 UseInfiniteBounce_infinite_bounces_multiplier_LightCount;
} pc;

struct Payload {
   vec3   Color;
    float Distance;
    int   Hit;
};

struct Shadow_Payload {
      int Shadow;
};

layout(location = 0) rayPayloadInEXT Payload payload;
layout(location = 1) rayPayloadEXT Shadow_Payload shadow_Payload;

hitAttributeEXT vec2 attribs;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

void main()
{
    vec3 Radiance    = vec3(0.0);
    vec3 HitNormal   = vec3(0.0);
    vec3 HitPosition = vec3(0.0);
    float Distance   = 0;

    if(gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT) { 
        Distance = gl_RayTminEXT + gl_HitTEXT; 
        Distance *= -0.2; 
    } else {
        uint packed      = gl_InstanceCustomIndexEXT;
        uint objectID    = packed >> 12;
        uint primitiveID = packed & 0xFFF;
    
        VertexAndIndexOffsets offsets = OffsetBuffer.Offsets[primitiveID];
        uint baseIndex = 3 * gl_PrimitiveID + offsets.IndexOffset;
    
        ivec3 index = ivec3(
            indexBuffer.Indices[baseIndex + 0],
            indexBuffer.Indices[baseIndex + 1],
            indexBuffer.Indices[baseIndex + 2]
        );
    
        Vertex v0 = vertexBuffer.vertices[index.x];
        Vertex v1 = vertexBuffer.vertices[index.y];
        Vertex v2 = vertexBuffer.vertices[index.z];
    
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
    
    float tangentSign = v0.tangent_Padding.w * bary.x +
                    v1.tangent_Padding.w * bary.y +
                    v2.tangent_Padding.w * bary.z;

        vec2 TexCoord = 
            v0.texCoord_Padding.xy * bary.x +
            v1.texCoord_Padding.xy * bary.y +
            v2.texCoord_Padding.xy * bary.z;
    
        mat3 normalMatrix  = mat3(transformations[objectID].Inverese_Transposed_WorldMatrix);

        vec3 WorldN        = normalize(normalMatrix * Normal);
        vec3 WorldT        = normalize(normalMatrix * Tangent);
        vec3 WorldB = cross(WorldN, WorldT) * (tangentSign < 0.0 ? -1.0 : 1.0);
        mat3 WorldSpaceTBN = mat3(WorldT, WorldB, WorldN);
    
        vec3  Albedo     = texture(Albedo_AssetImages          [nonuniformEXT(primitiveID)], TexCoord).rgb;
        float Metallic   = texture(MetalicRoughness_AssetImages[nonuniformEXT(primitiveID)], TexCoord).r;
        float Roughness  = texture(MetalicRoughness_AssetImages[nonuniformEXT(primitiveID)], TexCoord).r;
        vec3  Emissive   = texture(Emmisive_AssetImages        [nonuniformEXT(primitiveID)], TexCoord).rgb;

        vec3 NormalTexture = texture(Normal_AssetImages[nonuniformEXT(primitiveID)], TexCoord).rgb * 2.0 - vec3(1.0);
        HitNormal = normalize(WorldSpaceTBN * NormalTexture);
        HitNormal = Normal;
        vec4 WorldPos = transformations[objectID].WorldMatrix * vec4(VertexPosition, 1.0);

        HitPosition = WorldPos.xyz;

        // --- Lighting ---
        vec3 LightDir = vec3(1,1,1);
        const float Constant  = 1.0;
        const float Linear    = 0.09;
        const float Quadratic = 0.032;
        
        for (int i = 0; i < pc.UseInfiniteBounce_infinite_bounces_multiplier_LightCount.w; i++) {
            LightData light = lights[i];
            vec3 Lo = vec3(0.0);
            vec3 radiance = vec3(0.0);
            float lightDist = 10000.0;
            
            if(light.positionAndLightType.w < 0.5) { // Directional
                LightDir = normalize(-light.positionAndLightType.xyz);
                radiance = light.colorAndAmbientStrength.rgb;
                lightDist = 10000.0;
            } else { // Point
                vec3 LightPos = light.positionAndLightType.xyz;
                vec3 lightVec = LightPos - WorldPos.xyz;
                LightDir = normalize(lightVec);
                lightDist = length(lightVec);
                
                float Attenuation = 1.0 / (Constant + Linear * lightDist + Quadratic * (lightDist * lightDist));
                radiance = light.colorAndAmbientStrength.rgb * Attenuation;
            }  
            
             vec3  ViewDir    = normalize(light.CameraPositionAndLightIntensity.xyz -  WorldPos.xyz);

             vec3 F0          = vec3(0.04); 
                  F0          = mix(F0, Albedo, Metallic);
             vec3 halfwayDir  = normalize(LightDir + ViewDir);
                 float HdotV  = max(dot(halfwayDir, ViewDir), 0.0);
            
             vec3 F    = fresnelSchlick(HdotV, F0);

             vec3 kS = F;
             vec3 kD = vec3(1.0) - kS;
             kD *= 1.0 - Metallic;
            
            vec3 diffuse = kD * (Albedo / PI);

            float NdotL = max(dot(HitNormal, LightDir), 0.0);

            vec3 shadowOrigin   = HitPosition + (WorldN * 0.05);
            const uint rayFlags = gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
            const uint cullMask = 0xFF;
            const float tMin    = 0.001;
            const float tMax    = 1000.0;

            shadow_Payload.Shadow = 0;
            traceRayEXT(topLevelAS, rayFlags, cullMask, 0, 0, 1, shadowOrigin, tMin, LightDir, tMax, 1);
         
              Lo += ((diffuse * radiance * light.CameraPositionAndLightIntensity.a)) * shadow_Payload.Shadow;
              Radiance += Lo;
        }
        
        //Radiance += Emissive * 3;

            int UseInfiniteBounce = int(pc.UseInfiniteBounce_infinite_bounces_multiplier_LightCount.x);
        
              if(UseInfiniteBounce > 0.5) {
              
                    vec3 GI  = SampleIrradiance(
                                          IrradianceStorageImage,
                                          VisibilityStorageImage,
                                          HitPosition,
                                          HitNormal,
                                          pc.GridBaseLocation_ScreenSizeWidth.xyz,
                                          pc.ProbeSpacing_ScreenSizeHeight.xyz,
                                          ivec3(pc.ProbeCount.xyz),
                                          // Irradiance Params
                                          pc.ProbeSideLength,
                                          pc.GutterSize,
                                          pc.AtlasWidthSize,
                                          // Visibility Params
                                          pc.ProbeSideLength,   //Same values are being passed cus as of now they are both the same size. but this can be changed
                                          pc.GutterSize,
                                          pc.AtlasWidthSize) * pc.UseInfiniteBounce_infinite_bounces_multiplier_LightCount.y;

             if (any(greaterThan(GI, vec3(0)))) {
                 Radiance += GI * Albedo;
             }
        }

        Distance = gl_RayTminEXT + gl_HitTEXT;
    }

    payload.Color       = Radiance;
    payload.Distance    = Distance;
    payload.Hit         = 1;
}