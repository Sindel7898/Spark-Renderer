#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 1) rayPayloadInEXT int Shadow;

void main() {
    Shadow = 1; 
}