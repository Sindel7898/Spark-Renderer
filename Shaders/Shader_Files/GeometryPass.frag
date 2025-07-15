#version 450

layout (binding = 1) uniform sampler2D   samplerColor;
layout (binding = 2) uniform sampler2D   samplerNormalMap;
layout (binding = 3) uniform sampler2D   samplerMetallicRoughnessMapAO;
layout (binding = 4) uniform samplerCube samplerReflectiveCubeMap;

layout(location = 0) in vec4 WorldSpacePosition;   
layout(location = 1) in vec4 ViewSpacePosition;        
layout(location = 2) in vec2 fragTexCoord;           
layout(location = 3) in mat3 WorldSpaceTBN; 
layout(location = 6) in mat3 ViewSpaceTBN; 
layout(location = 9) in vec4 IsReflective; 
layout(location = 10)in mat4 InverseModel; 

layout (location = 0) out vec4 outWorldPosition;
layout (location = 1) out vec4 outViewSpacePosition;
layout (location = 2) out vec4 outNormal;
layout (location = 3) out vec4 outViewSpaceNormal;
layout (location = 4) out vec4 outAlbedo;
layout (location = 5) out vec4 outMetallicRoughnessMapAO;



void main() {
  
  outWorldPosition     = WorldSpacePosition;
  outViewSpacePosition = ViewSpacePosition;

  vec3 tnorm = normalize(WorldSpaceTBN * normalize(texture(samplerNormalMap, fragTexCoord).rgb * 2.0 - vec3(1.0)));
  outNormal = vec4(tnorm, 1);

  vec3 vtnorm = ViewSpaceTBN * normalize(texture(samplerNormalMap, fragTexCoord).rgb * 2.0 - vec3(1.0));
  outViewSpaceNormal = vec4(vtnorm, IsReflective.x);

  outMetallicRoughnessMapAO = vec4(texture(samplerMetallicRoughnessMapAO,fragTexCoord).rgb,IsReflective.r);


  vec3 Reflection = normalize(reflect(ViewSpacePosition.xyz,normalize(vtnorm)));
  	   Reflection = vec3(InverseModel * vec4(Reflection, 0.0));
       Reflection.xy *= -1.0;

  vec4 Albedo           = texture(samplerColor,fragTexCoord);
  vec4 MappedReflection = texture(samplerReflectiveCubeMap,Reflection);
  vec4 Result           = mix(Albedo,MappedReflection,outMetallicRoughnessMapAO.r);
  vec4 FinalResult      = mix(Result,Albedo,outMetallicRoughnessMapAO.g);

  outAlbedo   = vec4(FinalResult.rgb,1.0);
}