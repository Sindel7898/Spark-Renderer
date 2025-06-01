#version 450

layout (binding = 0) uniform sampler2D samplerPosition;
layout (binding = 1) uniform sampler2D samplerNormal;
layout (binding = 2) uniform sampler2D samplerAlbedo;
layout (binding = 3) uniform sampler2D samplerSSAO;
layout (binding = 4) uniform sampler2D samplerMaterials;

layout(location = 0) in vec2 inTexCoord;           

struct LightData{
    vec4    positionAndLightType;
    vec4    colorAndAmbientStrength;
    vec4    CameraPositionAndLightIntensity;
    mat4    LightProjectionViewMatrix;

};
layout (binding = 5) uniform LightUniformBuffer {
   
   LightData lights[3];
};

layout (location = 0) out vec4 outFragcolor;



void main() {

    vec3 FragPosition = texture(samplerPosition,inTexCoord).rgb;
    vec3 Normal       = normalize(texture(samplerNormal,inTexCoord).rgb);
    vec3 Albedo       = texture(samplerAlbedo,inTexCoord).rgb;
    float SSAO        = texture(samplerSSAO,inTexCoord).r;
    float Metalic     = texture(samplerMaterials,inTexCoord).r;
    float Roughtness  = texture(samplerMaterials,inTexCoord).g;
    float AO          = texture(samplerMaterials,inTexCoord).b;

    const float Shininess       = 30.0;
    const float SpecularStrength = 0.2;

    vec3  LightDir = vec3(1,1,1);

    const float Constant   = 1.0;
    const float Linear     = 0.09;
    const float Quadratic  = 0.032;

    vec3 totalLighting = vec3(0.0);
    float ambientOcclusion = AO * SSAO;

  for (int i = 0; i < 3; i++) {
     
     float Attenuation = 1.0;

     LightData light = lights[i];


      if(light.positionAndLightType.w == 0){

         LightDir = normalize(-light.positionAndLightType.xyz);

       }
      else if (light.positionAndLightType.w == 1){
               
               vec3 LightPos = light.positionAndLightType.xyz;
               LightDir          = normalize(LightPos - FragPosition);
               float Distance    = length(LightPos -  FragPosition);
               Attenuation       = 1.0 / (Constant + Linear * Distance + Quadratic * (Distance * Distance));
      }  


    float DiffuseAmount = max(dot(Normal, LightDir), 0.0);
    vec3  Diffuse        = Albedo * light.colorAndAmbientStrength.rgb * DiffuseAmount;

    vec3  ViewDir    = normalize(light.CameraPositionAndLightIntensity.xyz -  FragPosition.xyz);
    vec3  halfwayDir = normalize(LightDir + ViewDir);
    float spec       = pow(max(dot(Normal, halfwayDir), 0.0), Shininess);
    vec3  specular    = light.colorAndAmbientStrength.rgb * spec * SpecularStrength;


    vec3 ambientColor = light.colorAndAmbientStrength.rgb * light.colorAndAmbientStrength.a;
    vec3 Ambient = Albedo * ambientColor * ambientOcclusion;


    vec3 lightContribution = (Ambient  + Diffuse + specular) * Attenuation  * light.CameraPositionAndLightIntensity.w;

    totalLighting += lightContribution;
  }

    outFragcolor = vec4(Roughtness,Roughtness,Roughtness, 1.0);
}