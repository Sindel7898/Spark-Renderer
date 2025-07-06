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
    vec4 IsReflective;
} ubo;

layout(location = 0) out vec4 WorldSpacPosition;   
layout(location = 1) out vec4 ViewSpacePosition;   
layout(location = 2) out vec2 fragTexCoord;           
layout(location = 3) out mat3 WorldSpaceTBN; 
layout(location = 6) out mat3 ViewSpaceTBN; 
layout(location = 9) out vec4 IsReflective; 

void main() {
    
    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    vec4 viewPos = ubo.view * worldPos;
   
    WorldSpacPosition = worldPos;
    ViewSpacePosition = viewPos;

    gl_Position = ubo.proj * viewPos;

    mat3 normalMatrix = transpose(inverse(mat3(ubo.model)));

    vec3 worldT  = normalize(vec3(normalMatrix * inTangent  ));
    vec3 worldB  = normalize(vec3(normalMatrix * inBiTangent));
    vec3 worldN  = normalize(vec3(normalMatrix * inNormal   ));

    WorldSpaceTBN = mat3(worldT, worldB, worldN);


    mat3 ViewSpacenormalMatrix = transpose(inverse(mat3(ubo.view)));

    vec3 vT = normalize(vec3(ViewSpacenormalMatrix * worldT));
    vec3 vB = normalize(vec3(ViewSpacenormalMatrix * worldB));
    vec3 vN = normalize(vec3(ViewSpacenormalMatrix * worldN));

    ViewSpaceTBN = mat3(vT, vB, vN);

    fragTexCoord = inTexCoord;
    IsReflective = ubo.IsReflective;

}