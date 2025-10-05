#version 460
#extension GL_EXT_ray_tracing : require


struct Payload {
    vec3 Color;
    float Distance;
    vec3 Normal;
};

layout(location = 0) rayPayloadInEXT Payload payload;

void main() {
    payload.Color     = vec3(0,0,0);
    payload.Normal    = vec3(0,0,0);
    payload.Distance  = -1.0f;

}