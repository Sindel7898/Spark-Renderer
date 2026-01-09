#version 460
#extension GL_EXT_ray_tracing : require

struct ShadowPayload {
       int visibility;
};

layout(location = 0) rayPayloadInEXT ShadowPayload shadowPayload;

void main() {
  shadowPayload.visibility = 1;
}