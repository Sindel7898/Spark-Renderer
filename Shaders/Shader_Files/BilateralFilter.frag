#version 450

layout (binding = 0) uniform sampler2D SSGIImage;

layout(location = 0) in vec2 inTexCoord;           
layout (location = 0) out vec4 outFragcolor;

void main() {
   

    outFragcolor = vec4(texture(SSGIImage, inTexCoord,2).rgb, 1.0);
}
