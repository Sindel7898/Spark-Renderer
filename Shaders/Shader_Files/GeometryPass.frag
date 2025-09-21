#version 450

layout (binding = 2) uniform sampler2D   samplerColor;
layout (binding = 3) uniform sampler2D   samplerNormal;
layout (binding = 4) uniform sampler2D   samplerMetallicRoughness;
layout (binding = 5) uniform sampler2D   samplerAO;
layout (binding = 6) uniform sampler2D   samplerEmisive;

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
layout (location = 5) out vec4 outEmisive;
layout (location = 6) out vec4 outMetallicRoughnessMapAO;
layout (location = 7) out vec4 outReflectionMask;



void main() {
  
  outWorldPosition     = WorldSpacePosition;
  outViewSpacePosition = ViewSpacePosition;
  
  vec3 NormalTexture = textureLod(samplerNormal, fragTexCoord,0).rgb * 2.0 - vec3(1.0);


  vec3 tnorm = normalize(WorldSpaceTBN * NormalTexture);
  outNormal = vec4(tnorm, 1);

  vec3 vtnorm = normalize(ViewSpaceTBN * NormalTexture);
  outViewSpaceNormal = vec4(vtnorm, 1);

  vec2 MetallicRoughness  = textureLod(samplerMetallicRoughness,fragTexCoord,0).gb;
  float A0                = textureLod(samplerAO,fragTexCoord,0).r;

  outMetallicRoughnessMapAO = vec4(MetallicRoughness,A0,1);

  vec3 Albedo = textureLod(samplerColor,fragTexCoord,0).rgb;

  outAlbedo   = vec4(Albedo,1.0);

  outReflectionMask   = vec4(bCubeMapReflection_bScreenSpaceReflection_Padding.rgb,1.0);


  vec3 Emisive = textureLod(samplerEmisive,fragTexCoord,0).rgb;
  outEmisive   = vec4(Emisive * 3,1);

}