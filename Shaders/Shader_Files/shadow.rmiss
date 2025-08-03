#version 460
#extension GL_EXT_ray_tracing : require

struct HitPayload
{
  vec3 hitValue;
};

layout(location = 0) rayPayloadInEXT HitPayload inPayloadResults;

void main()
{
  inPayloadResults.hitValue = vec3(1,1,1);
}