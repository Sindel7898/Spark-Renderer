#version 450
////VertexBuffer Data
layout(location = 0) in vec3 inPosition;

//Uniform Buffer Data bound to 0 *NOTE: look into sets* 
layout(set = 0,binding = 0) uniform VertexUniformBufferObject {
    mat4 view;
    mat4 proj;
} vuob;

struct InstanceData{
       mat4 Model;
       vec4 Color;
};

layout(set = 0,binding = 1) uniform UniformBufferObject {
     InstanceData ModelInstance[300];
};

layout(location = 0) out vec4 Light_Color;   

void main() {
    
     mat4 model = ModelInstance[gl_InstanceIndex].Model;

    gl_Position = vuob.proj * vuob.view * model * vec4(inPosition, 1.0);

    Light_Color = ModelInstance[gl_InstanceIndex].Color;
}