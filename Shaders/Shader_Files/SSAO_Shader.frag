#version 450

layout (binding = 0) uniform sampler2D samplerposition;
layout (binding = 1) uniform sampler2D samplerNormal;

layout(location = 0) in vec2 inTexCoord;           


layout (location = 0) out vec4 outFragcolor;

void main() {
    
    vec4 position = texture(samplerposition,inTexCoord);
    vec4 normals = texture(samplerNormal,inTexCoord);

    outFragcolor = vec4(normals.rgb,1.0f);
}