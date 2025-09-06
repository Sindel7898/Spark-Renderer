#version 460
#extension GL_EXT_ray_tracing : require

struct ReflectionPayload {
    vec3 Color;
};

layout(location = 0) rayPayloadInEXT ReflectionPayload reflPayload;

void main() {
    reflPayload.Color = vec3(0,0,0);
}