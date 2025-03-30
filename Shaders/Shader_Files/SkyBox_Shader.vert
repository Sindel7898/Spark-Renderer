#version 450
////VertexBuffer Data
layout(location = 0) in vec3 inPosition;

//Uniform Buffer Data bound to 0 *NOTE: look into sets* 
layout(set = 0,binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

//
layout(location = 0) out vec3  texCoord;

void main() {
    
    mat4 viewRotOnly = mat4(mat3(ubo.view));
    vec4 pos = ubo.proj * viewRotOnly *  vec4(inPosition, 1.0);
    gl_Position = pos.xyww;
    texCoord = inPosition;

}