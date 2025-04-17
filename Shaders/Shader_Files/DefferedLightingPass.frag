#version 450

layout (binding = 0) uniform sampler2D samplerposition;
layout (binding = 1) uniform sampler2D samplerNormal;
layout (binding = 2) uniform sampler2D samplerAlbedo;

layout(location = 0) in vec2 inTexCoord;           

struct LightData{
    vec4    positionAndLightType;
    vec4    colorAndAmbientStrength;
    vec4    lightIntensityAndPadding;

};
layout (binding = 3) uniform LightUniformBuffer {
   
   LightData lights[3];
};

layout (location = 0) out vec4 outFragcolor;

void main() {

    vec3 FragPosition = texture(samplerposition,inTexCoord).rgb;
    vec3 Normal       = normalize(texture(samplerNormal,inTexCoord).rgb);
    vec3 Albedo       = texture(samplerAlbedo,inTexCoord).rgb;

    vec3  LightDir = vec3(1,1,1);
    float Attenuation = 1.0;

    float Constant   = 1.0;
    float Linear     = 0.09;
    float Quadratic  = 0.032;

    vec3 totalLighting = vec3(0.0);

  for (int i = 0; i < 100; i++) {
     
     LightData light = lights[i];

      if(light.positionAndLightType.w == 0){

         LightDir = normalize(-light.positionAndLightType.xyz);

    }
    else if (light.positionAndLightType.w == 1){
    
         LightDir          = normalize(light.positionAndLightType.xyz - FragPosition);
         float Distance    = length(light.positionAndLightType.xyz - FragPosition);
         Attenuation       = 1.0 / (Constant + Linear * Distance + Quadratic * (Distance * Distance));
   }

    float DifuseAmount = max(dot(Normal, LightDir), 0.0);

    vec3 Diffuse = Albedo * light.colorAndAmbientStrength.rgb * DifuseAmount * Attenuation;
    vec3 Ambient = Albedo * light.colorAndAmbientStrength.a;
    vec3 FinalColor = (Diffuse + Ambient) * light.lightIntensityAndPadding.x;

   totalLighting += FinalColor;
  }

    outFragcolor = vec4(totalLighting, 1.0);
}