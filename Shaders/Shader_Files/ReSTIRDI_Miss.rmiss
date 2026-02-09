#version 460
#extension GL_EXT_ray_tracing : require


struct UnifiedPayload {
    vec4 data;  
    vec4 Emissive;  
    vec4 Position;
    vec4 Normal;
    vec4 Albedo;
};

layout(location = 0) rayPayloadInEXT UnifiedPayload payload;

void main() {

     payload.data     =  vec4(0);
     payload.Emissive =  vec4(0);
     payload.Position =  vec4(0);
     payload.Normal   =  vec4(0);
     payload.Albedo   =  vec4(0);
}