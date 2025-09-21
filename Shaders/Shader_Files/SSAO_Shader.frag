#version 450

layout (binding = 0) uniform sampler2D samplerposition;
layout (binding = 1) uniform sampler2D samplerNormal;
layout (binding = 2) uniform sampler2D samplerNoise;

layout (binding = 3) uniform KernelData{
     
     mat4 CameraProjMatrix;
     mat4 CameraViewMatrix;
     vec4 KernelSizeRadiusBiasAndBool;
     vec4 samples[32];
}KD;

layout(location = 0) in vec2 inTexCoord;           


layout (location = 0) out vec4 outFragcolor;

void main() {
    
    float occlusion = 0.0f;

    if(KD.KernelSizeRadiusBiasAndBool.w == 1){
        
        vec2 TextureSize       = vec2(textureSize(samplerposition,0));
        vec3 ViewSpaceFragPos  = textureLod(samplerposition,inTexCoord,0).rgb;
        vec3 ViewSpaceNormal   = textureLod(samplerNormal, inTexCoord,0).rgb;
        vec3 randomVec         = textureLod(samplerNoise,inTexCoord * TextureSize,0).rgb;
        
        vec3 tangent   = normalize(randomVec - ViewSpaceNormal * dot(randomVec,ViewSpaceNormal));
        vec3 bitangent = cross(ViewSpaceNormal,tangent);
        mat3 TBN       = mat3(tangent, bitangent, ViewSpaceNormal);  
        
        float radius = KD.KernelSizeRadiusBiasAndBool.y;

       for(int i = 0; i < KD.KernelSizeRadiusBiasAndBool.x; ++i){
       
           vec3 samplePos = TBN * KD.samples[i].xyz;
                samplePos = ViewSpaceFragPos + samplePos * radius;
       
           vec4 offset = KD.CameraProjMatrix * vec4(samplePos, 1.0);
           offset.xyz /= offset.w;               
           offset.xyz  = offset.xyz * 0.5 + 0.5; 
           
           float  sampleDepth  = textureLod(samplerposition, offset.xy,0).z;
           
           float rangeCheck = smoothstep(0.0, 1.0, radius/ abs(ViewSpaceFragPos.z - sampleDepth));
          
           if(sampleDepth >= ViewSpaceFragPos.z + KD.KernelSizeRadiusBiasAndBool.z) {
                   occlusion += (1.0 * rangeCheck);
               }
          }

        occlusion = 1.0 - (occlusion / KD.KernelSizeRadiusBiasAndBool.x);
    }
 
     outFragcolor =  vec4(occlusion,occlusion,occlusion,1.0f);

}