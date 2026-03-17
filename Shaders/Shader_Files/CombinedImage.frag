#version 450
const float PI = 3.14159265359;

layout (binding = 0) uniform sampler2D LightingReflectionTexture;
layout (binding = 1) uniform sampler2D GITexture;                
layout (binding = 2) uniform sampler2D SSAOTexture;                
layout (binding = 3) uniform sampler2D MaterialsTexture;                
layout (binding = 4) uniform sampler2D AlbedoTexture;                
layout (binding = 5, rgba16f) readonly  uniform image2D  DDGITexture;
layout (binding = 6, rgba16f) readonly  uniform image2D  PTGI_Texture;
layout (binding = 7, rgba16f) uniform image2D PreviousPTGI_Texture;
layout (binding = 8) uniform sampler2D MotionVectors;                

layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform PushConstants {
    vec4 Brightness_Saturation_Concentration_GIBoost;
    vec4 MaxGamma_MinGamma_GISolution_gAccumCount;
} pc;

float rgb2luma(vec3 rgb) {
    return dot(rgb, vec3(0.299, 0.587, 0.114));
}

vec3 ContrastSaturationBrightness(vec3 color, float brt, float sat, float con) 
{
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
     float MaxGamma      = pc.MaxGamma_MinGamma_GISolution_gAccumCount.x;
     float MinGamma      = pc.MaxGamma_MinGamma_GISolution_gAccumCount.y;

     vec3 DirectLighting   = texture(LightingReflectionTexture, inTexCoord).rgb;
     vec3 SSGI             = texture(GITexture, inTexCoord).rgb;
     float SSAO            = texture(SSAOTexture, inTexCoord).r;
     float MaterialAO      = texture(MaterialsTexture, inTexCoord).b;
     vec3  Albedo          = texture(AlbedoTexture, inTexCoord).rgb;
     
     ivec2 texelSize = imageSize(DDGITexture);
     ivec2 texelCoord = ivec2(inTexCoord * vec2(texelSize));
     
     vec3 DDGI = imageLoad(DDGITexture, texelCoord).rgb;
     
     float AO     = SSAO * MaterialAO;
     if(AO < 0.1){AO = 1;}

     vec3 DDGIresult   = DDGI  * Albedo / PI;
     vec3 SSGIGIresult = SSGI  * Albedo / PI;

     vec3 GI = vec3(0.0);    

     int GISolution = int(pc.MaxGamma_MinGamma_GISolution_gAccumCount.z);

     if(GISolution == 0) {
        GI = DDGIresult;
     }

     //if(GISolution == 1) {
     //   GI = SSGIGIresult;
     //}

    // if(GISolution == 2) {
    //    GI = DDGIresult + SSGIGIresult;
    // }

    if(GISolution == 1) {
       vec4 curColor  = imageLoad(PTGI_Texture, texelCoord);
       
       vec2 Velocity = texture(MotionVectors, inTexCoord).rg;
       ivec2 historySize = imageSize(PreviousPTGI_Texture);

       
       vec2 currentPixelPos = inTexCoord * vec2(historySize);

       vec2 motionInPixels = Velocity * vec2(historySize);

       vec2 screenPosPrevious = currentPixelPos - motionInPixels;


       ivec2 PrevTexelCoord = ivec2(screenPosPrevious);

       bool validHistory = all(greaterThanEqual(PrevTexelCoord, ivec2(0))) && 
                           all(lessThan(PrevTexelCoord, historySize));

       vec4 prevColor = vec4(0.0);
       float AccumeCount = pc.MaxGamma_MinGamma_GISolution_gAccumCount.w;

       if (validHistory) {
           prevColor = imageLoad(PreviousPTGI_Texture, PrevTexelCoord);
       } else {
           AccumeCount = 0.0;
           prevColor = curColor;
       }

       vec4 blendedColor = (AccumeCount * prevColor + curColor) / (AccumeCount + 1.0);

       imageStore(PreviousPTGI_Texture, texelCoord, blendedColor);
       
       GI = blendedColor.rgb;
     }

    vec3 FinalColor = (DirectLighting + (GI * GIboost) * AO);

    vec3 CorrectedColor   = ContrastSaturationBrightness(FinalColor, Brightness, Saturation, Concentration);

    outFragColor = vec4(FinalColor,1.0);
}