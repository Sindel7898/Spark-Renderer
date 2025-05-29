#version 450

layout (binding = 0) uniform sampler2D samplerposition;
layout (binding = 1) uniform sampler2D samplerNormal;
layout (binding = 2) uniform sampler2D samplerNoise;

layout (binding = 3) uniform KernelData{
     
     mat4 CameraProjMatrix;
     mat4 CameraViewMatrix;
     vec3 samples[64];
}KD;

layout(location = 0) in vec2 inTexCoord;           


layout (location = 0) out vec4 outFragcolor;

const vec2 noiseScale = vec2(2560/4.0, 1440/4.0);  

void main() {
    
    vec3 WorldFragPos       = texture(samplerposition,inTexCoord).xyz;
    vec3 Normal             = texture(samplerNormal, inTexCoord).xyz;

    mat3 normalMatrix = mat3(KD.CameraViewMatrix);

    vec4 ViewSpaceFragPos = KD.CameraViewMatrix * vec4(WorldFragPos,1.0f);
    vec3 ViewSpaceNormal  =  normalize(normalMatrix * Normal);

    vec3 randomVec     = texture(samplerNoise,inTexCoord * noiseScale).xyz;

    vec3 tangent   = normalize(randomVec - ViewSpaceNormal * dot(randomVec,ViewSpaceNormal));
    vec3 bitangent = cross(ViewSpaceNormal,tangent);
    mat3 TBN       = mat3(tangent, bitangent, ViewSpaceNormal);  

    float occlusion = 0.0f;
    
    for(int i = 0; i < 22; ++i){
        vec3 samplePos = ViewSpaceFragPos.xyz + (TBN * KD.samples[i]) * 1.0;
        
        vec4 offset = KD.CameraProjMatrix * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;               // perspective divide
        offset.xyz  = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0  

        ////////////////////////////////////////////////////////////////
        vec3 offsetFragPos          = texture(samplerposition,offset.xy).xyz;
        vec4 offsetViewSpaceFragPos = KD.CameraViewMatrix * vec4(offsetFragPos.xyz,1.0f);
        float sampleDepth           = -offsetViewSpaceFragPos.z;

        float rangeCheck = smoothstep(0.0, 1.0, 1.0 / abs(ViewSpaceFragPos.z - sampleDepth)); 
        occlusion += (sampleDepth >= ViewSpaceFragPos.z + 0.025 ? 1.0 : 0.0) * rangeCheck;

    }

    occlusion = 1.0 - (occlusion / 22);
   
    outFragcolor =  vec4(occlusion,occlusion,occlusion,1.0f);
}