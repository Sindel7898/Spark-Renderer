#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 1) uniform sampler2D Albedo_AssetImages[];
layout(set = 0, binding = 2) uniform sampler2D Normal_AssetImages[];

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

layout(set = 0, binding = 5) buffer IndexBufferSSBO {
    uint Indices[];
} indexBuffer;

layout(set = 0, binding = 6) buffer VertexBufferSSBO {
    Vertex vertices[];
} vertexBuffer;

layout(set = 0, binding = 7) buffer VertexIndexOffsetBufferSSBO {
    VertexAndIndexOffsets Offsets[];

} OffsetBuffer;

layout(set = 0, binding = 8) uniform Transformation {

    mat4 WorldMatrix[10];
} Transformations;


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


    mat3 normalMatrix  = mat3(Transformations.WorldMatrix[objectID]);
    vec3 WorldN        = normalize(normalMatrix * Normal);
    vec3 WorldT        = normalize(normalMatrix * Tangent);
    vec3 WorldB        = cross(WorldN,WorldT);

    mat3 WorldSpaceTBN = mat3(WorldT, WorldB, WorldN);

    vec2 hitUV = attribs.hitUV;

    vec3 Albedo = texture(Albedo_AssetImages[nonuniformEXT(primitiveID)], TexCoord).rgb;

    vec3 NormalTexture = texture(Normal_AssetImages[nonuniformEXT(primitiveID)], TexCoord).rgb * 2.0 - vec3(1.0);
    vec3 tnorm = normalize(WorldSpaceTBN * NormalTexture);


     payload.Color    = Albedo;
     payload.Distance = gl_RayTmaxEXT;
     payload.Normal   = tnorm;

 }
