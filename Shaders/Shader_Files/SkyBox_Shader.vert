#version 450
////VertexBuffer Data
layout(location = 0) in vec3 inPosition;

//Uniform Buffer Data bound to 0 *NOTE: look into sets* 
layout(set = 0,binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

//
layout(location = 0) out vec3  texCoord;

void main() {
    
    mat4 viewNoTranslation  = mat4(mat3(ubo.view));
    vec4 pos = ubo.proj * viewNoTranslation  *  vec4(inPosition, 1.0);
    gl_Position = pos;
    gl_Position.z = gl_Position.w; 

    texCoord = inPosition;

}