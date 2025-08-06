#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

layout(binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1) uniform sampler2D  Position;
layout(binding = 2) uniform sampler2D  Normal;

layout(binding = 5) uniform LightInformation{
  vec4 LightDirection;
}Light_UBO;

struct HitPayload
{
  vec2 UV;
  bool Shadowed;

};

layout(location = 0) rayPayloadInEXT HitPayload inPayloadResults;

void main()
{
    vec2 UV = inPayloadResults.UV;

    vec3 hitNormal   = texture(Normal,   UV).xyz;
    vec3 hitPosition = texture(Position, UV).xyz;

    // Directional light casts rays in opposite direction
    vec3 lightDir = -normalize(Light_UBO.LightDirection.xyz);

    inPayloadResults.Shadowed = true;

    float tmin = 0.001;
    float tmax = 10000.0; // Infinite distance for directional light
    vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    origin += 0.001;

    traceRayEXT(
        topLevelAS,
        gl_RayFlagsTerminateOnFirstHitEXT | 
        gl_RayFlagsOpaqueEXT | 
        gl_RayFlagsSkipClosestHitShaderEXT,
        0xFF,
        0, 0, 1, // SBT offset for shadow ray hit group
        origin, tmin,
        lightDir, tmax,
        0// payload location
    );
}