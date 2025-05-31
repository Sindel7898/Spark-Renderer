#version 450

layout (binding = 0) uniform sampler2D samplerposition;
layout (binding = 1) uniform sampler2D samplerNormal;
layout (binding = 2) uniform sampler2D samplerNoise;

layout (binding = 3) uniform KernelData{
     
     mat4 CameraProjMatrix;
     mat4 CameraViewMatrix;
     vec4 KernelSizeRadiusBiasAndPadding;
     vec4 samples[64];
}KD;

layout(location = 0) in vec2 inTexCoord;           


layout (location = 0) out vec4 outFragcolor;

const vec2 noiseScale = vec2(2560 /4.0, 1440 /4.0);  

void main() {
    
    vec3 ViewSpaceFragPos  = texture(samplerposition,inTexCoord).xyz;
    vec3 ViewSpaceNormal   = texture(samplerNormal, inTexCoord).xyz;
    vec3 randomVec         = texture(samplerNoise,inTexCoord * noiseScale).xyz;

    vec3 tangent   = normalize(randomVec - ViewSpaceNormal * dot(randomVec,ViewSpaceNormal));
    vec3 bitangent = cross(ViewSpaceNormal,tangent);
    mat3 TBN       = mat3(tangent, bitangent, ViewSpaceNormal);  

    float occlusion = 0.0f;
    float radius = KD.KernelSizeRadiusBiasAndPadding.y;

  for(int i = 0; i < KD.KernelSizeRadiusBiasAndPadding.x; ++i){

       vec3 samplePos = TBN * KD.samples[i].xyz;
            samplePos = ViewSpaceFragPos + samplePos * radius;

       vec4 offset = KD.CameraProjMatrix * vec4(samplePos, 1.0);
       offset.xyz /= offset.w;               
       offset.xyz  = offset.xyz * 0.5 + 0.5; 
       
       float  sampleDepth  = texture(samplerposition, offset.xy).z;
       
       float rangeCheck = smoothstep(0.0, 1.0, radius/ abs(ViewSpaceFragPos.z - sampleDepth));
       

       if(sampleDepth >= ViewSpaceFragPos.z + KD.KernelSizeRadiusBiasAndPadding.z) {
               occlusion += (1.0 * rangeCheck);
           }
}

    occlusion = 1.0 - (occlusion / KD.KernelSizeRadiusBiasAndPadding.x);
   
    outFragcolor =  vec4(occlusion,occlusion,occlusion,1.0f);
}