#version 450
layout(push_constant) uniform PushConstants {
    int AccumilationCount;
} pc;

layout (binding = 0) uniform sampler2D SSGIImage;
layout (binding = 1) uniform sampler2D LastFrameImage;

layout(location = 0) in vec2 inTexCoord;           
layout (location = 0) out vec4 outFragcolor;



void main() {

    vec3 Color          = textureLod(SSGIImage,inTexCoord,0).rgb;
    vec3 LastFrameColor = textureLod(LastFrameImage,inTexCoord,0).rgb;

    float count = pc.AccumilationCount;


    outFragcolor = vec4 ((count * LastFrameColor + Color) / (count + 1),1);}

     ///outFragcolor = vec4 (Color,1);}

