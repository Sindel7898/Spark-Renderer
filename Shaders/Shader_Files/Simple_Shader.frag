#version 450
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragPosition;
layout(location = 2) in vec3 normal;         
layout(location = 3) in vec3 Tangent;         
layout(location = 4) in vec3 BiTangent;         
layout(location = 5) in mat3 TBN;         

layout(set = 0,binding = 1) uniform LightUniformBuffer {
    vec3 LightLocation;
    vec4 BaseColor;
    float AmbientStrength;
}lub;

layout(binding = 2) uniform sampler2D AlbedoTexSampler;
layout(binding = 3) uniform sampler2D NormalTexSampler;


layout(location = 0) out vec4 outColor;

void main() {
    vec4 objectColor = texture(AlbedoTexSampler, fragTexCoord);

    // Fetch and unpack normal from normal map (tangent space → [-1, 1])
    vec3 normalMap = texture(NormalTexSampler, fragTexCoord).rgb;
    normalMap = normalize(normalMap * 2.0 - 1.0);

    // Transform normal from tangent space to world space
    vec3 worldNormal = normalize(TBN * normalMap);

    // Light direction (world space)
    vec3 lightDir = normalize(lub.LightLocation - fragPosition);

    // Diffuse lighting (Lambertian)
    float diff = max(dot(worldNormal, lightDir), 0.0);
    vec3 diffuse = diff * lub.BaseColor.rgb;

    // Ambient lighting
    vec3 ambient = lub.AmbientStrength * lub.BaseColor.rgb;

    // Final color
    //outColor = vec4((ambient + diffuse) * objectColor.rgb, objectColor.a);
      outColor = objectColor;
      outColor = vec4(normal,1);

}