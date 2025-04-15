#version 450

layout (binding = 0) uniform sampler2D samplerposition;
layout (binding = 1) uniform sampler2D samplerNormal;
layout (binding = 2) uniform sampler2D samplerAlbedo;


layout(location = 0) in vec2 inTexCoord;           


layout (location = 0) out vec4 outFragcolor;

void main() {

    vec3 Position = texture(samplerposition,inTexCoord).rgb;
    vec3 Normal   = texture(samplerNormal,inTexCoord).rgb;
    vec3 Albedo   = texture(samplerAlbedo,inTexCoord).rgb;

    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(Normal, lightDir), 0.0);
    
    outFragcolor = vec4(Albedo * diff, 1.0);
}