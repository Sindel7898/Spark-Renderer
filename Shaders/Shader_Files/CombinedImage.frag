#version 450
const float PI = 3.14159265359;

layout (binding = 0) uniform sampler2D LightingReflectionTexture;
layout (binding = 1) uniform sampler2D GITexture;                
layout (binding = 2) uniform sampler2D SSAOTexture;                
layout (binding = 3) uniform sampler2D MaterialsTexture;                
layout (binding = 4) uniform sampler2D AlbedoTexture;                
layout (binding = 5) uniform sampler2D ReflectionTexture;                

layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform PushConstants {
    vec4 Brightness_Saturation_Concentration_Padding;
    vec4 MaxGamma_MinGamma_Padding;
} pc;


float rgb2luma(vec3 rgb) {
    return dot(rgb, vec3(0.299, 0.587, 0.114));
}

vec3 ContrastSaturationBrightness(vec3 color, float brt, float sat, float con) 
{
   /*
    * Adapted for Processing by Raphaël de Courville <Twitter: @sableRaph>
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

float bayerDither(vec2 uv) {
    int x = int(mod(uv.x, 4.0));
    int y = int(mod(uv.y, 4.0));
    int index = x + y * 4;
    float ditherTable[16] = float[16](
        0.0,  8.0,  2.0, 10.0,
        12.0, 4.0, 14.0, 6.0,
        3.0, 11.0, 1.0,  9.0,
        15.0, 7.0, 13.0, 5.0
    );
    return (ditherTable[index] / 16.0 - 0.5) / 255.0; 
}

void main() {

     float Brightness    = pc.Brightness_Saturation_Concentration_Padding.x;
     float Saturation    = pc.Brightness_Saturation_Concentration_Padding.y;
     float Concentration = pc.Brightness_Saturation_Concentration_Padding.z;
     float MaxGamma      = pc.MaxGamma_MinGamma_Padding.x;
     float MinGamma      = pc.MaxGamma_MinGamma_Padding.y;

     vec3 DirectLighting   = texture(LightingReflectionTexture, inTexCoord).rgb;
     vec3 GI               = texture(GITexture, inTexCoord).rgb;
     float SSAO            = texture(SSAOTexture, inTexCoord).r;
     float MaterialAO      = texture(MaterialsTexture, inTexCoord).b;
     vec3 Albedo           = texture(AlbedoTexture, inTexCoord).rgb;
     vec3 Reflection           = texture(ReflectionTexture, inTexCoord).rgb;


     float FinalAO         = SSAO * MaterialAO;
     vec3 IndirectLighting = GI * Albedo / PI;
     IndirectLighting *= FinalAO;

     if(FinalAO < 0.1){FinalAO = 1;}

     vec3 FinalColor = ((DirectLighting + (Reflection * 0.2) ) + IndirectLighting * 1.5);
     vec3 CorrectedColor   = ContrastSaturationBrightness(FinalColor, Brightness, Saturation, Concentration);

    float luma = rgb2luma(CorrectedColor);
    float darkFactor   = smoothstep(0.0, 0.2, luma); // 0 when dark, 1 when bright
    float dynamicGamma = mix(MinGamma, MaxGamma, darkFactor);  // gamma = 0.7 in darks, 1.0 in brights

    vec3 gammaCorrected = pow(clamp(CorrectedColor, 0.0, 1.0), vec3(dynamicGamma));
    vec3 tonemapped = aces_approx(gammaCorrected);
    tonemapped += vec3(bayerDither(gl_FragCoord.xy));

   outFragColor = vec4(clamp(tonemapped, 0.0, 1.0), 1.0);
}
