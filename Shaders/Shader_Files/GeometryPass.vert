#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;       
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBiTangent;

layout(set = 0,binding = 0) uniform VertexUniformBufferObject {
      mat4 view;
      mat4 proj;
      mat4 Model;
}vuob;

struct InstanceData{
       mat4 Model;
       vec4 bCubeMapReflection_bScreenSpaceReflection_Padding;
};

layout(set = 0,binding = 1) uniform UniformBufferObject {
     InstanceData ModelInstance[2];
};



layout(location = 0) out vec4 WorldSpacPosition;   
layout(location = 1) out vec4 ViewSpacePosition;   
layout(location = 2) out vec2 fragTexCoord;           
layout(location = 3) out mat3 WorldSpaceTBN; 
layout(location = 6) out mat3 ViewSpaceTBN; 
layout(location = 9) out vec4 bCubeMapReflection_bScreenSpaceReflection_Padding; 

void main() {
    
    mat4 model = ModelInstance[gl_InstanceIndex].Model;
    //mat4 model = vuob.Model;

    vec4 worldPos = model * vec4(inPosition, 1.0);
    vec4 viewPos = vuob.view * worldPos;
   
    WorldSpacPosition = worldPos;
    ViewSpacePosition = viewPos;

    gl_Position = vuob.proj * viewPos;

    mat3 normalMatrix = transpose(inverse(mat3(model)));

    vec3 worldT  = normalize(vec3(normalMatrix * inTangent  ));
    vec3 worldB  = normalize(vec3(normalMatrix * inBiTangent));
    vec3 worldN  = normalize(vec3(normalMatrix * inNormal   ));

    WorldSpaceTBN = mat3(worldT, worldB, worldN);


    mat3 ViewSpacenormalMatrix = transpose(inverse(mat3(vuob.view)));

    vec3 vT = normalize(vec3(ViewSpacenormalMatrix * inTangent));
    vec3 vB = normalize(vec3(ViewSpacenormalMatrix * inBiTangent));
    vec3 vN = normalize(vec3(ViewSpacenormalMatrix * inNormal));

    ViewSpaceTBN = mat3(vT, vB, vN);

    fragTexCoord = inTexCoord;
    bCubeMapReflection_bScreenSpaceReflection_Padding = ModelInstance[gl_InstanceIndex].bCubeMapReflection_bScreenSpaceReflection_Padding;
}