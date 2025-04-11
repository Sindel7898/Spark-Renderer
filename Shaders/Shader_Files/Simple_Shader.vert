#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;       
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBiTangent;

layout(set = 0,binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

//Vertex Shader Outputs

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragPosition;
layout(location = 2) out vec3 normal;           
layout(location = 3) out vec3 Tangent;           
layout(location = 4) out vec3 BiTangent;           
layout(location = 5) out mat3 TBN; 


void main() {

    // Compute the final position of the vertex in clip space
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    
    // Transform position to world space for lighting calculations
    fragPosition = vec3(ubo.model * vec4(inPosition, 1.0));     

    normal       = normalize(inNormal);
    Tangent      = normalize(inTangent);
    BiTangent    = normalize(inBiTangent);
    
    TBN = mat3(inTangent, inBiTangent, inNormal);
    fragTexCoord = inTexCoord;
}