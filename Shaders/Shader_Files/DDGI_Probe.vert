#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;       
layout(location = 3) in vec3 inTangent;

layout(push_constant) uniform PushConstants {
    mat4 view;
    mat4 proj;
}pc;

layout(set = 0,binding = 2) readonly  buffer StorageBufferObject {
    vec4 Position[2000];
}SBO;

void main() {
    
     vec4 instancePos = SBO.Position[gl_InstanceIndex];
     vec4 worldPos    = vec4(inPosition, 1.0) + instancePos;

 
    gl_Position = pc.proj * pc.view * worldPos;
}

