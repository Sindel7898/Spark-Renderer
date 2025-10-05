#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

// Bindless texture array (variable descriptor count)
layout(set = 0, binding = 1) uniform sampler2D Albedo_AssetImages[];
layout(set = 0, binding = 2) uniform sampler2D Normal_AssetImages[];


struct Payload {
    vec3 Color;
};

layout(location = 0) rayPayloadInEXT Payload payload;

hitAttributeEXT struct HitAttribute {
    vec2 hitUV;
    vec3 normal;
    vec3 tangent;
} attribs;


void main() {
    uint blasIndex = gl_InstanceCustomIndexEXT;

    vec2 hitUV = attribs.hitUV;
    vec3 normal = normalize(attribs.normal);
    vec3 tangent = normalize(attribs.tangent);

    vec3 Albedo = texture(Albedo_AssetImages[nonuniformEXT(blasIndex)], hitUV).rgb;
    vec3 geometricNormal = gl_WorldRayDirectionEXT;

    payload.Color = Albedo;
}
