#version 450
////VertexBuffer Data
layout(location = 0) in vec3 inPosition;

//Uniform Buffer Data bound to 0 *NOTE: look into sets* 
layout(set = 0,binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 BaseColor;
} ubo;

//
layout(location = 0) out vec4  BaseColor;

void main() {
    
 
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    BaseColor = ubo.BaseColor;
}