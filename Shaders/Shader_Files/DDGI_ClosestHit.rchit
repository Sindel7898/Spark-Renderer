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

layout(set = 0, binding = 7) uniform Transformation {
    mat4 WorldMatrix[10];
} Transformations;


struct LightData{
    vec4    positionAndLightType;
    vec4    colorAndAmbientStrength;
    vec4    CameraPositionAndLightIntensity;
};

layout(set = 0, binding = 11) uniform LightUniformBuffer {

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


    mat3 normalMatrix  = transpose(inverse(mat3(Transformations.WorldMatrix[objectID])));
    vec3 WorldN        = normalize(normalMatrix * Normal);
    vec3 WorldT        = normalize(normalMatrix * Tangent);
    vec3 WorldB        = cross(WorldN,WorldT);

    mat3 WorldSpaceTBN = mat3(WorldT, WorldB, WorldN);

    vec3  Albedo     = texture(Albedo_AssetImages         [nonuniformEXT(primitiveID)], TexCoord).rgb;
    float Metallic  = texture(MetalicRoughness_AssetImages[nonuniformEXT(primitiveID)], TexCoord).r;
    float Roughness = texture(MetalicRoughness_AssetImages[nonuniformEXT(primitiveID)], TexCoord).r;

    vec3 NormalTexture = texture(Normal_AssetImages[nonuniformEXT(primitiveID)], TexCoord).rgb * 2.0 - vec3(1.0);
    vec3 tnorm = normalize(WorldSpaceTBN * NormalTexture);
    Normal = tnorm;
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    vec3  LightDir = vec3(1,1,1);
    const float Constant   = 1.0;
    const float Linear     = 0.09;
    const float Quadratic  = 0.032;
    
    vec3 radiance          = vec3(0.0);
    vec3 totalLighting     = vec3(0.0);

    vec4  WorldPos  =  Transformations.WorldMatrix[objectID] * vec4(VertexPosition,1);
    vec3  ViewDir    = -normalize(gl_WorldRayDirectionEXT);

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
  
     float NdotL = max(dot(Normal, LightDir), 0.0);        
             Lo +=  Albedo * radiance * NdotL;
  
  
     totalLighting +=  ((Lo) * light.CameraPositionAndLightIntensity.a);
  }


     payload.Color    = totalLighting;
     payload.Distance = gl_HitTEXT;
     payload.Normal   = Normal;

 }