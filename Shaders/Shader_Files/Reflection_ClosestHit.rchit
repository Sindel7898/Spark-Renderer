#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
const float PI = 3.14159265359;

layout(set = 0, binding = 1) uniform sampler2D Albedo_AssetImages[];
layout(set = 0, binding = 2) uniform sampler2D Normal_AssetImages[];
layout(set = 0, binding = 3) uniform sampler2D MetalicRoughness_AssetImages[];

struct Vertex {
    vec4 position_Padding;
    vec4 texCoord_Padding;
    vec4 normal_Padding;
    vec4 tangent_Padding;
};

struct VertexAndIndexOffsets {

    uint     VertexOffset;
    uint     IndexOffset;
    uint     MaterialIndex; 
    uint     Padding;       
};

layout(set = 0, binding = 6) buffer IndexBufferSSBO {
    uint Indices[];
} indexBuffer;

layout(set = 0, binding = 7) buffer VertexBufferSSBO {
    Vertex vertices[];
} vertexBuffer;

layout(set = 0, binding = 8) buffer VertexIndexOffsetBufferSSBO {
    VertexAndIndexOffsets Offsets[];

} OffsetBuffer;

struct Transformations {
    mat4 WorldMatrix;
    mat4 Inverese_Transposed_WorldMatrix;
};

layout(set = 0, binding = 9) uniform Transformation {
    Transformations transformations[1000];
};

struct LightData{
    vec4    positionAndLightType;
    vec4    colorAndAmbientStrength;
    vec4    CameraPositionAndLightIntensity;
};

layout(set = 0, binding = 10) uniform LightUniformBuffer {

   LightData lights[4];
};


struct Payload {
    vec3 Color;
    float Distance;
    vec3 Normal;
};

layout(location = 0) rayPayloadInEXT Payload payload;

hitAttributeEXT struct HitAttribute {
    vec2 hitUV;

} attribs;

///////////////////////////////////////////////////////////////////

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float safeRoughness = max(roughness, 0.05);
    float a      = safeRoughness*safeRoughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

///////////////////////////////////////////////////////////////////
void main()
{
    uint packed  = gl_InstanceCustomIndexEXT;

    uint objectID   = packed >> 12;
    uint primitiveID = packed & 0xFFF;

    VertexAndIndexOffsets offsets = OffsetBuffer.Offsets[primitiveID];
    
    uint baseIndex = 3 * gl_PrimitiveID + offsets.IndexOffset;

    ivec3 index = ivec3(
        indexBuffer.Indices[baseIndex  + 0],
        indexBuffer.Indices[baseIndex  + 1],
        indexBuffer.Indices[baseIndex  + 2]
    );

    Vertex v0 = vertexBuffer.vertices[index.x];
    Vertex v1 = vertexBuffer.vertices[index.y];
    Vertex v2 = vertexBuffer.vertices[index.z];

    vec3 bary = vec3(1.0 - attribs.hitUV.x - attribs.hitUV.y, attribs.hitUV.x, attribs.hitUV.y);

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
    vec3 WorldB        = cross(WorldN,WorldT);

    mat3 WorldSpaceTBN = mat3(WorldT, WorldB, WorldN);

    vec2 hitUV = attribs.hitUV;
    uint matIndex = offsets.MaterialIndex;

    vec3  Albedo     = texture(Albedo_AssetImages          [nonuniformEXT(offsets.MaterialIndex)], TexCoord).rgb;
    float Metallic  = texture(MetalicRoughness_AssetImages[nonuniformEXT (offsets.MaterialIndex)], TexCoord).r;
    float Roughness = texture(MetalicRoughness_AssetImages[nonuniformEXT (offsets.MaterialIndex)], TexCoord).g;

    vec3 NormalTexture = texture(Normal_AssetImages[nonuniformEXT(offsets.MaterialIndex)], TexCoord).rgb * 2.0 - vec3(1.0);
    vec3 tnorm = normalize(WorldSpaceTBN * NormalTexture);

    Normal = tnorm;

    ///////////////////////////////////////////////
    vec3  LightDir = vec3(1,1,1);
    const float Constant   = 1.0;
    const float Linear     = 0.09;
    const float Quadratic  = 0.032;
    
    vec3 radiance          = vec3(0.0);
    vec3 totalLighting     = vec3(0.0);

    vec4 WorldPos = transformations[objectID].WorldMatrix * vec4(VertexPosition, 1.0);

    vec3 ViewDir    = -normalize(gl_WorldRayDirectionEXT);

    vec3 F0          = vec3(0.04); 

  for (int i = 0; i < 4; i++) {

    LightData light = lights[i];


     vec3 Lo      = vec3(0.0);


      if(light.positionAndLightType.w < 0.5){

         LightDir = normalize(-light.positionAndLightType.xyz);
         radiance = light.colorAndAmbientStrength.rgb ;

       }
      else if (light.positionAndLightType.w > 0.5){
               
           vec3 LightPos   = light.positionAndLightType.xyz;
               LightDir    = normalize(LightPos - WorldPos.xyz);
         float Distance    = length(LightPos -  WorldPos.xyz);
         float Attenuation = 1.0 / (Constant + Linear * Distance + Quadratic * (Distance * Distance));
               radiance    = light.colorAndAmbientStrength.rgb * Attenuation;
       }  

         F0          = mix(F0, Albedo, Metallic);
    vec3 halfwayDir  = normalize(LightDir + ViewDir);
    
    vec3 F    = fresnelSchlick(max(dot(Normal, ViewDir), 0.0), F0);//Calculates how much light is reflected vs. refracted on a surface based on the view angle.
    float NDF = DistributionGGX(Normal, halfwayDir, Roughness); //describes how microfacet normal are distributed on a rough surface       
    float G   = GeometrySmith(Normal, ViewDir, LightDir, Roughness);// models shadowing and masking

    vec3  numerator    = NDF * G * F;
    float denominator  = 4.0 * max(dot(Normal, ViewDir), 0.0) * max(dot(Normal, LightDir), 0.0)  + 0.0001;
    vec3  specular     = (numerator / denominator) ;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - Metallic;	
    vec3 diffuse = kD * (Albedo / PI);

     float NdotL = max(dot(Normal, LightDir), 0.0);        
             Lo += (diffuse + specular) * radiance * NdotL;


     totalLighting +=  ((Lo) * light.CameraPositionAndLightIntensity.a);
  }

     payload.Color    = totalLighting.xyz;
     payload.Distance = gl_RayTmaxEXT;
     payload.Normal   = Normal;

 }