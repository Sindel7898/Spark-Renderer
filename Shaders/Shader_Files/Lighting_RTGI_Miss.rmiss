#version 460
#extension GL_EXT_ray_tracing : require

struct RTGIPayload {
       vec3 normal;
       vec3 radiance;
       vec3 position;
       vec3 albedo;
};

layout(location = 1) rayPayloadInEXT RTGIPayload   RTGIpayload;

void main() {
  RTGIpayload.normal   = vec3(0.0);
  RTGIpayload.radiance   = vec3(0.0);
  RTGIpayload.position   = vec3(0.0);
  RTGIpayload.albedo   = vec3(0.0);
}