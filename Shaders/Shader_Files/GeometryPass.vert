#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;       
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBiTangent;

//Uniform Buffer Data bound to 0 *NOTE: look into sets* 
layout(set = 0,binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) out vec3 Position;           
layout(location = 1) out vec2 fragTexCoord;           
layout(location = 2) out mat3 TBN; 

void main() {
    
    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    Position = worldPos.xyz;

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    
    mat3 normalMatrix = transpose(inverse(mat3(ubo.model)));

    vec3 T    = normalize(inTangent);
    vec3 B    = normalize(inBiTangent);
    vec3 N    = normalize(inNormal);

    TBN = mat3(T, B, N);
    fragTexCoord = inTexCoord;
}