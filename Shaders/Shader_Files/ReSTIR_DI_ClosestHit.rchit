#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
const float PI = 3.14159265359;

layout(set = 0, binding = 8)   uniform sampler2D         Albedo_AssetImages[];
layout(set = 0, binding = 9)   uniform sampler2D         Normal_AssetImages[];
layout(set = 0, binding = 10)  uniform sampler2D         MetalicRoughness_AssetImages[];
layout(set = 0, binding = 11)  uniform sampler2D         Emmisive_AssetImages[];

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

layout(set = 0, binding = 12) buffer IndexBufferSSBO {
    uint Indices[];
} indexBuffer;

layout(set = 0, binding = 13) buffer VertexBufferSSBO {
    Vertex vertices[];
} vertexBuffer;

layout(set = 0, binding = 14) buffer VertexIndexOffsetBufferSSBO {
    VertexAndIndexOffsets Offsets[];

} OffsetBuffer;

layout(set = 0, binding = 15) uniform Transformation {
    mat4 WorldMatrix[10];
} Transformations;


struct UnifiedPayload {
    int  rayType; 
    vec3 data;
    vec3 Emissive;   
};

layout(location = 0) rayPayloadInEXT UnifiedPayload payload;

hitAttributeEXT struct HitAttribute {
    vec2 hitUV;
} attribs;

void main()
{
    
    if (payload.rayType == 0){     
    
       payload.data.x = 0.0;
       return;
    }


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
   
   
        vec2 TexCoord = 
          v0.texCoord_Padding.xy * bary.x +
          v1.texCoord_Padding.xy * bary.y +
          v2.texCoord_Padding.xy * bary.z;
   
   
       vec3  Albedo     = texture(Albedo_AssetImages          [nonuniformEXT(primitiveID)], TexCoord).rgb;
       vec3 Emissive    = texture(Emmisive_AssetImages        [nonuniformEXT(primitiveID)], TexCoord).rgb;
       payload.data = vec3(0.0);
       payload.Emissive =  Emissive;

 }