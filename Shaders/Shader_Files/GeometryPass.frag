#version 450

layout (binding = 2) uniform sampler2D   samplerColor;
layout (binding = 3) uniform sampler2D   samplerNormal;
layout (binding = 4) uniform sampler2D   samplerMetallicRoughness;
layout (binding = 5) uniform sampler2D   samplerAO;

layout(location = 0) in vec4 WorldSpacePosition;   
layout(location = 1) in vec4 ViewSpacePosition;        
layout(location = 2) in vec2 fragTexCoord;           
layout(location = 3) in mat3 WorldSpaceTBN; 
layout(location = 6) in mat3 ViewSpaceTBN; 
layout(location = 9) in vec4 bCubeMapReflection_bScreenSpaceReflection_Padding; 

layout (location = 0) out vec4 outWorldPosition;
layout (location = 1) out vec4 outViewSpacePosition;
layout (location = 2) out vec4 outNormal;
layout (location = 3) out vec4 outViewSpaceNormal;
layout (location = 4) out vec4 outAlbedo;
layout (location = 5) out vec4 outMetallicRoughnessMapAO;
layout (location = 6) out vec4 outReflectionMask;



void main() {
  
  outWorldPosition     = WorldSpacePosition;
  outViewSpacePosition = ViewSpacePosition;

  vec3 tnorm = normalize(WorldSpaceTBN * normalize(texture(samplerNormal, fragTexCoord).rgb * 2.0 - vec3(1.0)));
  outNormal = vec4(tnorm, 1);

  vec3 vtnorm = ViewSpaceTBN * normalize(texture(samplerNormal, fragTexCoord).rgb * 2.0 - vec3(1.0));
  outViewSpaceNormal = vec4(vtnorm, 1);

  vec2 MetallicRoughness  = texture(samplerMetallicRoughness,fragTexCoord).gb;
  float A0                = texture(samplerAO,fragTexCoord).r;

  outMetallicRoughnessMapAO = vec4(MetallicRoughness,A0,1);

  vec3 Albedo = texture(samplerColor,fragTexCoord).rgb;

  outAlbedo   = vec4(Albedo,1.0);

  outReflectionMask   = vec4(bCubeMapReflection_bScreenSpaceReflection_Padding.rgb,1.0);

}