#version 450

layout (binding = 1) uniform sampler2D   samplerColor;
layout (binding = 2) uniform sampler2D   samplerNormalMap;
layout (binding = 3) uniform sampler2D   samplerMetallicRoughnessMapAO;

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

  vec3 tnorm = normalize(WorldSpaceTBN * normalize(texture(samplerNormalMap, fragTexCoord).rgb * 2.0 - vec3(1.0)));
  outNormal = vec4(tnorm, 1);

  vec3 vtnorm = ViewSpaceTBN * normalize(texture(samplerNormalMap, fragTexCoord).rgb * 2.0 - vec3(1.0));
  outViewSpaceNormal = vec4(vtnorm, 1);

  outMetallicRoughnessMapAO = vec4(texture(samplerMetallicRoughnessMapAO,fragTexCoord).rgb,1);

  vec4 Albedo = texture(samplerColor,fragTexCoord);
  outAlbedo   = vec4(Albedo.rgb,1.0);

  outReflectionMask   = vec4(bCubeMapReflection_bScreenSpaceReflection_Padding.rgb,1.0);

}