#version 460
#extension GL_EXT_ray_tracing : require

struct HitPayload
{
  bool Shadowed;
};

layout(location = 0) rayPayloadInEXT HitPayload inPayloadResults;

void main()
{
  inPayloadResults.Shadowed = false;

}