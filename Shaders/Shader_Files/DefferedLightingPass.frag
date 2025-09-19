#version 450
const float PI = 3.14159265359;

layout (binding = 0) uniform sampler2D samplerPosition;
layout (binding = 1) uniform sampler2D samplerNormal;
layout (binding = 2) uniform sampler2D samplerAlbedo;
layout (binding = 3) uniform sampler2D samplerMaterials;
layout (binding = 4) uniform samplerCube samplerReflectiveCubeMap;
layout (binding = 5) uniform sampler2D samplerReflectionMask;
layout (binding = 6) uniform sampler2D samplerShadowMap[4];

layout(location = 0) in vec2 inTexCoord;           

struct LightData{
    vec4    positionAndLightType;
    vec4    colorAndAmbientStrength;
    vec4    CameraPositionAndLightIntensity;
    mat4    LightProjectionViewMatrix;

};
layout (binding = 7) uniform LightUniformBuffer {
   
   LightData lights[4];
};

layout (location = 0) out vec4 outFragcolor;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float safeRoughness = max(roughness, 0.05);
    float a      = safeRoughness*safeRoughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

float PCF(sampler2D ShadowMap, int Channel, vec2 texCoord)
{
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(ShadowMap, 0);

    float weights[9] = float[](
        0.0625, 0.125, 0.0625,
        0.125,  0.25,  0.125,
        0.0625, 0.125, 0.0625
    );

    int index = 0;
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            vec2 sampleCoord = texCoord + vec2(x, y) * texelSize;
            vec4 sampleValues = textureLod(ShadowMap, sampleCoord, 0);

            float sampleShadow =
                (Channel == 0) ? sampleValues.r :
                (Channel == 1) ? sampleValues.g :
                (Channel == 2) ? sampleValues.b :
                (Channel == 3) ? sampleValues.a : 1.0;

            shadow += sampleShadow * weights[index];
            index++;
        }
    }

    return shadow;
}



void main() {

   //Defaults/////////////////////////////
    vec3  LightDir = vec3(1,1,1);

    const float Constant   = 1.0;
    const float Linear     = 0.09;
    const float Quadratic  = 0.032;

    vec3 radiance          = vec3(0.0);
    vec3 totalLighting     = vec3(0.0);
    ///////////////////////////////////////

    vec3  WorldPos       = textureLod(samplerPosition,inTexCoord,0).rgb;
    vec3  Normal         = normalize(textureLod(samplerNormal,inTexCoord,0).rgb);
    vec3  Albedo         = textureLod(samplerAlbedo,inTexCoord,0).rgb;
    float Metallic       = textureLod(samplerMaterials,inTexCoord,0).g;
    float Roughness      = textureLod(samplerMaterials,inTexCoord,0).r;
    vec2  ReflectionMask = textureLod(samplerReflectionMask,inTexCoord,0).rg;

    vec3  ViewDir    = normalize(lights[0].CameraPositionAndLightIntensity.xyz -  WorldPos);

    //Reflection Calc
    vec3 cR = reflect (-ViewDir, normalize(Normal));
    float mipCount = float(textureQueryLevels(samplerReflectiveCubeMap));
    float mipLevel = Roughness * (mipCount - 1.0);
    vec3 Reflection = textureLod(samplerReflectiveCubeMap, cR, mipLevel).rgb;



    float shadows[4] = {PCF(samplerShadowMap[0],0,inTexCoord),
                        PCF(samplerShadowMap[0],1,inTexCoord),
                        PCF(samplerShadowMap[0],2,inTexCoord),
                        PCF(samplerShadowMap[0],3,inTexCoord)};



  for (int i = 0; i < 4; i++) {

    LightData light = lights[i];


     vec3 Lo      = vec3(0.0);


      if(light.positionAndLightType.w < 0.5){

         LightDir = normalize(-light.positionAndLightType.xyz);
         radiance = light.colorAndAmbientStrength.rgb ;

       }
      else if (light.positionAndLightType.w > 0.5){
               
           vec3 LightPos   = light.positionAndLightType.xyz;
               LightDir    = normalize(LightPos - WorldPos);
         float Distance    = length(LightPos -  WorldPos);
         float Attenuation = 1.0 / (Constant + Linear * Distance + Quadratic * (Distance * Distance));
               radiance    = light.colorAndAmbientStrength.rgb * Attenuation;
       }  

    vec3 F0          = vec3(0.04); 
         F0          = mix(F0, Albedo, Metallic);
    vec3 halfwayDir  = normalize(LightDir + ViewDir);
    
    vec3 F    = fresnelSchlick(max(dot(Normal, ViewDir), 0.0), F0);//Calculates how much light is reflected vs. refracted on a surface based on the view angle.
    float NDF = DistributionGGX(Normal, halfwayDir, Roughness); //describes how microfacet normal are distributed on a rough surface       
    float G   = GeometrySmith(Normal, ViewDir, LightDir, Roughness);// models shadowing and masking

    vec3  numerator    = NDF * G * F;
    float denominator  = 4.0 * max(dot(Normal, ViewDir), 0.0) * max(dot(Normal, LightDir), 0.0)  + 0.0001;
    vec3  specular     = (numerator / denominator) ;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
  
         kD *= 1.0 - Metallic;	

     float NdotL = max(dot(Normal, LightDir), 0.0);        
             Lo += (kD * (Albedo / PI) + specular) * radiance * NdotL;

     vec3 envSpecular = vec3(0); 

     if(ReflectionMask.x > 0.5){
     
       float NdotV = max(dot(Normal, ViewDir), 0.0);
       vec3 F      = fresnelSchlick(NdotV, F0);
       
       envSpecular = Reflection * F * 0.1;
     }

     totalLighting +=  ((Lo + envSpecular) * light.CameraPositionAndLightIntensity.a) * shadows[i];
  }


   outFragcolor = vec4(totalLighting, 1.0);
}