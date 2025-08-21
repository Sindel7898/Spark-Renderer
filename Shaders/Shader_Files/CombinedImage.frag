#version 450

layout (binding = 0) uniform sampler2D LightingReflectionTexture;
layout (binding = 1) uniform sampler2D GITexture;                
layout (binding = 2) uniform sampler2D SSAOTexture;                
layout (binding = 3) uniform sampler2D MaterialsTexture;                

layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outFragColor;


void main() {
    vec3  Lighting_Shadows = textureLod(LightingReflectionTexture, inTexCoord,0).rgb;
    vec3  GI               = textureLod(GITexture, inTexCoord,0).rgb;
    float SSAO             = textureLod(SSAOTexture, inTexCoord,0).r;
    float TextureFromAO    = textureLod(MaterialsTexture, inTexCoord,0).b;

    float FinalAO   = (SSAO * TextureFromAO) * 0.7;

    vec3 FinalColor = Lighting_Shadows + (GI * FinalAO) ;

    outFragColor    = vec4(FinalColor, 1.0); 
}
