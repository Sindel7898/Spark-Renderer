
struct SurfaceData {
    vec3  worldPos;
    vec3  normal;
    vec3  albedo;
    float metallic;
    float roughness;
};

const int GIIndex = 900;

 const float MaxResevoirVolume = 30.0; 

 ////////https://gist.github.com/JuanDiegoMontoya/f4226d0fa3c627bb78e82fda67057d6e
uint pcg_hash(uint seed)
{
  uint state = seed * 747796405u + 2891336453u;
  uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
  return (word >> 22u) ^ word;
}

// Used to advance the PCG state.
uint rand_pcg(inout uint rng_state)
{
  uint state = rng_state;
  rng_state = rng_state * 747796405u + 2891336453u;
  uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
  return (word >> 22u) ^ word;
}

// Advances the prng state and returns the corresponding random float.
float rand(inout uint state)
{
  uint x = rand_pcg(state);
  state = x;
  return float(x)*uintBitsToFloat(0x2f800000u);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////

vec3 offsetPositionAlongNormal(vec3 WorldPosition, vec3 normal) {
    return WorldPosition + normal * 0.01;
}

vec3 offsetPositionAlongView(vec3 WorldPosition, vec3 ViewDir) {
    return WorldPosition + ViewDir * 0.01;
}

struct Reservoir {
    uint  Light_Index; 
    float Light_Weight; 
    float Sum_Of_All_Weights; 
    float Num_OF_Lights_In_Resevoir;

    //Virtual Light Data
    vec3  Sample_Pos;  
    vec3  Sample_Flux;   
};

bool UpdateReservoir(inout Reservoir reservoir, uint LightIndex, float Weight, float c, inout uint seed) {
    reservoir.Sum_Of_All_Weights += Weight;
    reservoir.Num_OF_Lights_In_Resevoir += c;

    if (rand(seed) < (Weight / reservoir.Sum_Of_All_Weights)) {
        reservoir.Light_Index = LightIndex;
        return true;
    }
    return false;
}

vec3 GetPerpendicularVector(vec3 v) {
    return normalize(cross(v, abs(v.x) > abs(v.z) ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0)));
}

vec3 EvaluateBSDF(SurfaceData surface, vec3 ViewDir, vec3 LightDir) {

    vec3 F0 = mix(vec3(0.04), surface.albedo, surface.metallic);
    vec3 F = fresnelSchlick(max(dot(surface.normal, ViewDir), 0.0), F0);
    vec3 kD = (vec3(1.0) - F) * (1.0 - surface.metallic);
    vec3 diffuse = kD * (surface.albedo / 3.14159265359);

    vec3 halfwayDir = normalize(LightDir + ViewDir);
    float NDF = DistributionGGX(surface.normal, halfwayDir, surface.roughness);      
    float G_spec = GeometrySmith(surface.normal, ViewDir, LightDir, surface.roughness);
    
    vec3 numerator = NDF * G_spec * F;
    float denominator = 4.0 * max(dot(surface.normal, ViewDir), 0.0) 
                            * max(dot(surface.normal, LightDir), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    return diffuse + specular;
}

vec3 GetHemisphereSample(vec2 randVal, vec3 HitNormal) {
    vec3 bitangent = GetPerpendicularVector(HitNormal);
    vec3 tangent = cross(bitangent, HitNormal);
    float r = sqrt(randVal.x);
    float phi = 2.0 * 3.14159265 * randVal.y;
    
    return tangent * (r * cos(phi)) + bitangent * (r * sin(phi)) + HitNormal.xyz * sqrt(max(0.0, 1.0 - randVal.x));
}

vec3 GetGIRadiance(vec3 worldPos, vec3 normal) {

    vec3 Irradiance = SampleIrradiance(
        IrradianceStorageImage, VisibilityStorageImage,
        worldPos,          
        normal,             
        pc.GridBaseLocation_ScreenSizeWidth.xyz, 
        pc.ProbeSpacing_ScreenSizeHeight.xyz,
        ivec3(pc.ProbeCount.xyz), 
        pc.ProbeSideLength, pc.GutterSize, pc.AtlasWidthSize,
        pc.ProbeSideLength, pc.GutterSize, pc.AtlasWidthSize, 
        pc.CameraPosition.xyz
    );

    return Irradiance;
}

vec3 GetLightRadiance(LightData light, vec3 cameraPos, SurfaceData surface, vec3 ViewDir, vec3 F, vec3 diffuse) {
    vec3 radiance   = vec3(0.0);
    vec3 LightDir   = vec3(0.0);

    if (light.positionAndLightType.w > 0.5) {
        vec3 LightPos = light.positionAndLightType.xyz;
        LightDir = normalize(LightPos - surface.worldPos);
        float Distance = length(LightPos - surface.worldPos);
        float Attenuation = 1.0 / (1.0 + 0.09 * Distance + 0.032 * (Distance * Distance));
        radiance = light.colorAndAmbientStrength.rgb * Attenuation;
    } else {
        LightDir = normalize(-light.positionAndLightType.xyz);
        radiance = light.colorAndAmbientStrength.rgb;
    }

    vec3 halfwayDir = normalize(LightDir + ViewDir);
    float NDF = DistributionGGX(surface.normal, halfwayDir, surface.roughness);      
    float G = GeometrySmith(surface.normal, ViewDir, LightDir, surface.roughness);
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(surface.normal, ViewDir), 0.0) * max(dot(surface.normal, LightDir), 0.0) + 0.0001;
    vec3 specular = (numerator / denominator);

    float NdotL = max(dot(surface.normal, LightDir), 0.0);
    return (diffuse + specular) * radiance * NdotL * light.CameraPositionAndLightIntensity.a;
}

vec3 GetRadiance(uint lightIndex, vec3 cameraPos, SurfaceData surface, vec3 ViewDir, vec3 F, vec3 diffuse) {
    if (lightIndex == GIIndex) {
        return GetGIRadiance(surface.worldPos, surface.normal);
    }
    return GetLightRadiance(lights[lightIndex], cameraPos, surface, ViewDir, F, diffuse);
}

vec3 EvaluateCandidateRadiance(uint lightIndex, vec3 samplePos, vec3 sampleFlux, vec3 cameraPos, SurfaceData surface, vec3 ViewDir, vec3 F, vec3 diffuse) {
   
   if (lightIndex == GIIndex) {
        vec3  lightVec   = normalize(samplePos - surface.worldPos);
        float Distance   = distance(samplePos, surface.worldPos); 
        float Distance2  = Distance * Distance;

        float Cos = max(dot(surface.normal, lightVec), 0.0); // Lambert Diffuse

        //Calculate the size the pixel can have in world space and clamp based on that
        float camDist = distance(cameraPos, surface.worldPos);
        float a0 = ((4 * PI ) * camDist * camDist) / max(dot(surface.normal, ViewDir), 0.001);

        float G_Term   = min(Cos / max(Distance2, 0.001), a0);

        return sampleFlux * G_Term;
    }

    return GetLightRadiance(lights[lightIndex], cameraPos, surface, ViewDir, F, diffuse);
}
