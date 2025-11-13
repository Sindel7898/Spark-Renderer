#version 460
#extension GL_EXT_ray_tracing : require

struct Payload {
    vec3  Color;
    float Distance;
    vec3  Normal;
    int   Hit;
    vec3  HitPosition;
};

layout(location = 0) rayPayloadInEXT Payload payload;

void main() {
    payload.Color     =  vec3(0);
    payload.Distance  =  1000.0f;
    payload.Hit       =  0;
    payload.Normal    =  vec3(0);
    payload.HitPosition     =  vec3(0);

}