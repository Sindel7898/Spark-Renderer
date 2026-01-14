#version 450
const float PI = 3.14159265359;

layout (binding = 0) uniform sampler2D SceneTexture;

layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outFragColor;

void main() {

     vec3 Scene   = texture(SceneTexture, inTexCoord).rgb;
   
     vec3 srgbColor = pow(Scene, vec3(1.0 / 2.2));

     outFragColor = vec4(srgbColor,1.0);
}
