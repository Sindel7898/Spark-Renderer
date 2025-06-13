#version 450

layout (binding = 1) uniform sampler2D samplerColor;
layout (binding = 2) uniform sampler2D samplerNormalMap;
layout (binding = 3) uniform sampler2D samplerMetallicRoughnessMapAO;
layout (binding = 4) uniform sampler2D samplerHeightMap;

layout(location = 0) in vec4 Position;   
layout(location = 1) in vec4 ViewSpacePosition;        
layout(location = 2) in vec2 fragTexCoord;           
layout(location = 3) in mat3 TBN; 
layout(location = 6) in mat3 ViewSpaceTBN; 

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outViewSpacePosition;
layout (location = 2) out vec4 outNormal;
layout (location = 3) out vec4 outViewSpaceNormal;
layout (location = 4) out vec4 outAlbedo;
layout (location = 5) out vec4 outMetallicRoughnessMapAO;


void main() {
  
  outPosition = Position;
  outViewSpacePosition = ViewSpacePosition;

  vec3 tnorm = TBN * normalize(texture(samplerNormalMap, fragTexCoord).rgb * 2.0 - vec3(1.0));

  outNormal = vec4(-tnorm,1.0f);

    vec3 vtnorm = ViewSpaceTBN * normalize(texture(samplerNormalMap, fragTexCoord).rgb * 2.0 - vec3(1.0));
  outViewSpaceNormal = vec4(vtnorm, 1.0);

  outMetallicRoughnessMapAO = texture(samplerMetallicRoughnessMapAO,fragTexCoord* 14.5);
  outAlbedo = texture(samplerColor,fragTexCoord * 14.5);
}