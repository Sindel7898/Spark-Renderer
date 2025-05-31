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

layout(location = 0) out vec4 Position;   
layout(location = 1) out vec4 ViewSpacePosition;   
layout(location = 2) out vec2 fragTexCoord;           
layout(location = 3) out mat3 TBN; 
layout(location = 6) out mat3 ViewSpaceTBN; 

void main() {
    
    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    vec4 viewPos = ubo.view * worldPos;
   
    Position = worldPos;
    ViewSpacePosition = viewPos;

    gl_Position = ubo.proj * viewPos;
    
    mat3 normalMatrix = transpose(inverse(mat3(ubo.model * ubo.view)));

    vec3 T = normalize(vec3(normalMatrix * inTangent  ));
    vec3 B = normalize(vec3(normalMatrix * inBiTangent));
    vec3 N = normalize(vec3(normalMatrix * inNormal   ));

    TBN = mat3(T, B, N);

    mat3 viewMatrix3 = mat3(ubo.view);
    ViewSpaceTBN =  TBN * viewMatrix3;

    fragTexCoord = inTexCoord;
}