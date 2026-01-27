#version 460
#extension GL_EXT_ray_tracing : require

struct RTGIPayload {
       vec4 normal;
       vec4 radiance;
       vec4 position;
       vec4 albedo;
};

layout(location = 1) rayPayloadInEXT RTGIPayload   RTGIpayload;

void main() {
  RTGIpayload.normal     = vec4(0.0);
  RTGIpayload.radiance   = vec4(0.0);
  RTGIpayload.position   = vec4(0.0);
  RTGIpayload.albedo     = vec4(0.0);
}