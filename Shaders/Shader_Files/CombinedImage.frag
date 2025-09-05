#version 450

layout (binding = 0) uniform sampler2D LightingReflectionTexture;
layout (binding = 1) uniform sampler2D GITexture;                
layout (binding = 2) uniform sampler2D SSAOTexture;                
layout (binding = 3) uniform sampler2D MaterialsTexture;                
layout (binding = 4) uniform sampler2D AlbedoTexture;                

layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outFragColor;

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

void main() {
     vec3 Lighting_Shadows = texture(LightingReflectionTexture, inTexCoord).rgb;
     vec3 GI               = texture(GITexture, inTexCoord).rgb;
     float SSAO            = texture(SSAOTexture, inTexCoord).r;
     float TextureFromAO   = texture(MaterialsTexture, inTexCoord).b;
     vec3 Albedo           = texture(AlbedoTexture, inTexCoord).rgb;
     

     float FinalAO         = SSAO * TextureFromAO;
     
	 if(FinalAO == 0){
	    FinalAO = 1;
	 }
     vec3 FinalColor       = (Lighting_Shadows * FinalAO) + (GI * Albedo );
     vec3 CorrectedColor   = ContrastSaturationBrightness(FinalColor, 1.0, 1.7, 1.0);
     
     outFragColor = vec4(CorrectedColor, 1.0);
}
