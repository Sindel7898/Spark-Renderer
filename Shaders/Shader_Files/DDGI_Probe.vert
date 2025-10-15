#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;       
layout(location = 3) in vec3 inTangent;

layout(push_constant) uniform PushConstants {
    mat4 view;
    mat4 proj;
}pc;

layout(set = 0,binding = 2) uniform UniformBufferObject {
    mat4 model[1000];
}ubo;

void main() {
    
 
    gl_Position = pc.proj * pc.view * ubo.model[gl_InstanceIndex] * vec4(inPosition, 1.0);
}

