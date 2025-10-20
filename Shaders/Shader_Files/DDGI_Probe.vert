#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;       
layout(location = 3) in vec3 inTangent;

layout(push_constant) uniform PushConstants {
    mat4 view;
    mat4 proj;
}pc;

layout(set = 0,binding = 1) readonly  buffer StorageBufferObject {
    vec4 Position[2000];
}SBO;

layout(location = 0) out flat int ProbeIndex;
layout(location = 1) out vec2 TexCoord;

void main() {
    
     vec4 instancePos = SBO.Position[gl_InstanceIndex];
     vec4 worldPos    = vec4(inPosition, 1.0) + instancePos;

   ProbeIndex = gl_InstanceIndex;
   TexCoord = inTexCoord;

    gl_Position = pc.proj * pc.view * worldPos;
}

