#version 450
const float PI = 3.14159265359;

layout (binding = 0) uniform sampler2D LightingReflectionTexture;
layout (binding = 1) uniform sampler2D GITexture;                
layout (binding = 2) uniform sampler2D SSAOTexture;                
layout (binding = 3) uniform sampler2D MaterialsTexture;                
layout (binding = 4) uniform sampler2D AlbedoTexture;                
layout(binding = 5, rgba16f) readonly uniform image2D  DDGITexture;

layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform PushConstants {
    vec4 Brightness_Saturation_Concentration_GIBoost;
    vec4 MaxGamma_MinGamma_GISolution_Padding;
} pc;


float rgb2luma(vec3 rgb) {
    return dot(rgb, vec3(0.299, 0.587, 0.114));
}

vec3 ContrastSaturationBrightness(vec3 color, float brt, float sat, float con) 
{
   /*
    * Adapted for Processing by Rapha�l de Courville <Twitter: @sableRaph>
   */
	// Increase or decrease theese values to adjust r, g and b color channels seperately
	const float AvgLumR = 0.5;
	const float AvgLumG = 0.5; 
	const float AvgLumB = 0.5;
	
	const vec3 LumCoeff = vec3(0.2125, 0.7154, 0.0721);
	
	vec3 AvgLumin  = vec3(AvgLumR, AvgLumG, AvgLumB);
	vec3 brtColor  = color * brt;
	vec3 intensity = vec3(dot(brtColor, LumCoeff));
	vec3 satColor  = mix(intensity, brtColor, sat);
	vec3 conColor  = mix(AvgLumin, satColor, con);
	
	return conColor;
}

vec3 aces_approx(vec3 v)
{
    v *= 0.6f;
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((v*(a*v+b))/(v*(c*v+d)+e), 0.0f, 1.0f);
}

void main() {

     float Brightness    = pc.Brightness_Saturation_Concentration_GIBoost.x;
     float Saturation    = pc.Brightness_Saturation_Concentration_GIBoost.y;
     float Concentration = pc.Brightness_Saturation_Concentration_GIBoost.z;
     float GIboost       = pc.Brightness_Saturation_Concentration_GIBoost.w;
     float MaxGamma      = pc.MaxGamma_MinGamma_GISolution_Padding.x;
     float MinGamma      = pc.MaxGamma_MinGamma_GISolution_Padding.y;

     vec3 DirectLighting   = texture(LightingReflectionTexture, inTexCoord).rgb;
     vec3 SSGI               = texture(GITexture, inTexCoord).rgb;
     float SSAO            = texture(SSAOTexture, inTexCoord).r;
     float MaterialAO      = texture(MaterialsTexture, inTexCoord).b;
     vec3 Albedo           = texture(AlbedoTexture, inTexCoord).rgb;
     ivec2 texelSize = imageSize(DDGITexture);
     ivec2 texelCoord = ivec2(inTexCoord * vec2(texelSize));
     vec3 DDGI = imageLoad(DDGITexture, texelCoord).rgb;

     float AO     = SSAO * MaterialAO;

     vec3 DDGIresult   = DDGI  * Albedo / PI;
     vec3 SSGIGIresult = SSGI  * Albedo / PI;

     if(AO < 0.1){AO = 1;}

     vec3 GI = vec3(0.0);   

     if(pc.MaxGamma_MinGamma_GISolution_Padding.z == 0) {
        GI = DDGIresult;
     }

     if(pc.MaxGamma_MinGamma_GISolution_Padding.z == 1) {
        GI = SSGI;
     }

     if(pc.MaxGamma_MinGamma_GISolution_Padding.z == 2) {
        GI = DDGIresult + SSGI;
     }

    if(pc.MaxGamma_MinGamma_GISolution_Padding.z == 3) {
        GI = vec3(0);
     }
    vec3 FinalColor = (DirectLighting + (GI * GIboost) * AO);

    vec3 CorrectedColor   = ContrastSaturationBrightness(FinalColor, Brightness, Saturation, Concentration);

    outFragColor = vec4(CorrectedColor,1.0);
}
