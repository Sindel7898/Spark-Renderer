#version 450

layout (binding = 0) uniform sampler2D LightingReflectionTexture;
layout (binding = 1) uniform sampler2D GITexture;                
layout (binding = 2) uniform sampler2D SSAOTexture;                
layout (binding = 3) uniform sampler2D MaterialsTexture;                

layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outFragColor;


void main() {
    vec3  Lighting_Shadows = texture(LightingReflectionTexture, inTexCoord).rgb;
    vec3  GI               = texture(GITexture, inTexCoord).rgb;
    float SSAO             = texture(SSAOTexture, inTexCoord).r;
    float TextureFromAO    = texture(MaterialsTexture, inTexCoord).b;

    float FinalAO   = (SSAO * TextureFromAO) * 0.7;

    vec3 FinalColor = Lighting_Shadows + (GI * FinalAO) ;

    outFragColor    = vec4(FinalColor, 1.0); 
}
