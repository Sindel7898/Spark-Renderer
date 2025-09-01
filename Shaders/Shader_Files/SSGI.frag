#version 450

layout (binding = 0) uniform sampler2D NormalTexture;
layout (binding = 1) uniform sampler2D ViewSpacePosition;
layout (binding = 2) uniform sampler2D DepthTexture;
layout (binding = 3) uniform sampler2D AlbedoTexture;
layout (binding = 4) uniform sampler2D DirectLigtingTexture;
layout (binding = 5) uniform sampler2D BlueNoise[63];

layout (binding = 6) uniform SSGIUniformBuffer {
    mat4 ProjectionMatrix;
    vec4 BlueNoiseImageIndex_DeltaTime_Padding;
} ubo;

layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outFragcolor;

const int MAX_ITERATION = 20;
const int NUM_RAYS = 4;
const float MAX_THICKNESS = 0.1; 

vec3 GetPerpendicularVector(vec3 v) {

    return normalize(cross(v, abs(v.x) > abs(v.z) ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0)
    ));
}
vec3 GetHemisphereSample(vec2 randVal, vec3 HitNormal) {
    vec3 bitangent = GetPerpendicularVector(HitNormal);
    vec3 tangent = cross(bitangent, HitNormal);
    float r = sqrt(randVal.x);
    float phi = 2.0 * 3.14159265 * randVal.y;
    
    return tangent * (r * cos(phi)) + bitangent * (r * sin(phi)) + HitNormal.xyz * sqrt(max(0.0, 1.0 - randVal.x));
}

vec3 FindIntersectionPoint(vec3 SamplePosInVS, vec3 DirInVS, float MaxTraceDistance) {
    vec3 EndPosInVS = SamplePosInVS + DirInVS * MaxTraceDistance;
    vec3 dp2 = EndPosInVS - SamplePosInVS;
    
    // Use max component for step calculation
    float max_dist = max(max(abs(dp2.x), abs(dp2.y)), abs(dp2.z));
    vec3 Step = dp2 / (max_dist * 2.3); // Optimized step calculation
    
    vec3 rayPosInVS = SamplePosInVS;
    
    for(int i = 0; i < MAX_ITERATION; i++) {
        rayPosInVS += Step;
        
        vec4 rayPosPS = ubo.ProjectionMatrix * vec4(rayPosInVS, 1.0);
        rayPosPS.xyz /= rayPosPS.w;
        vec2 uv = rayPosPS.xy * 0.5 + 0.5;
        
        // Early exit for out-of-bounds
        if(any(greaterThan(uv, vec2(1.0))) || any(lessThan(uv, vec2(0.0)))) {
            break;
        }
        
        // Sample depth with LOD to improve cache performance
        vec3 scenePosVS = textureLod(ViewSpacePosition, uv, 0.0).xyz;
        float rayDepth = -rayPosInVS.z;
        float sceneDepth = -scenePosVS.z;
        
        if(rayDepth >= sceneDepth && rayDepth - sceneDepth < MAX_THICKNESS) {
            return rayPosInVS; // Early return
        }
    }
    
    return SamplePosInVS; // No intersection
}

vec3 offsetPositionAlongNormal(vec3 ViewPosition, vec3 normal)
{
    return ViewPosition + 0.0001 * normal;
}




void main() {


    int NoiseImageIndex = int(ubo.BlueNoiseImageIndex_DeltaTime_Padding.x);

    vec3 Color = textureLod(DirectLigtingTexture, inTexCoord,0).rgb;
    vec3 Albedo = textureLod(AlbedoTexture, inTexCoord,0).rgb;
    vec3 VSposition = textureLod(ViewSpacePosition, inTexCoord,0).rgb;
    vec3 Normal = normalize(textureLod(NormalTexture, inTexCoord,0).xyz);
    
    
    // Get blue noise sample
    ivec2 WindowSize = textureSize(DirectLigtingTexture, 3);

    vec2 tiledUV = (inTexCoord * (WindowSize / 100));

    
    vec3 giContribution = vec3(0.0);


       for(int i = 0; i < NUM_RAYS; i++) {

             int noiseIdx = (NoiseImageIndex + i) % 63;

             vec2 noise = textureLod(BlueNoise[noiseIdx], tiledUV,0).rg;

            vec3 stochasticNormal = GetHemisphereSample(noise, Normal);

            if(dot(stochasticNormal, stochasticNormal) > 0.001){
            
             vec3 IntersectionPoint = FindIntersectionPoint(
                                       offsetPositionAlongNormal(VSposition, stochasticNormal), 
                                        stochasticNormal, 
                                        100.0);
            
            vec4 RayPositionPS = ubo.ProjectionMatrix * vec4(IntersectionPoint, 1.0);
            RayPositionPS.xyz /= RayPositionPS.w;
            vec2 uv = RayPositionPS.xy * 0.5 + 0.5;
            
            vec3 hitLighting = textureLod(DirectLigtingTexture, uv,0).rgb;
            
            vec3 radianceB = hitLighting;
            float cosTerm = max(dot(Normal, stochasticNormal), 0.0);
            
             giContribution += (radianceB * cosTerm);  
       }
    }

    outFragcolor = vec4(giContribution, 1.0);
}