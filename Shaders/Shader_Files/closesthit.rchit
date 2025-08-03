#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

struct HitPayload
{
  vec3 hitValue;
};

layout(location = 0) rayPayloadInEXT HitPayload inPayloadResults;

void main()
{
  inPayloadResults.hitValue = vec3(0, 0, 0);
}