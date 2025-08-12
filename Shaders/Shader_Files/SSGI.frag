#version 450

layout (binding = 0) uniform sampler2D NormalTexture;
layout (binding = 1) uniform sampler2D ViewSpacePosition;
layout (binding = 2) uniform sampler2D DepthTexture;
layout (binding = 3) uniform sampler2D ColorTexture;
layout (binding = 4) uniform sampler2D BlueNoise[14];

layout (binding = 5) uniform SSGIUniformBuffer {
    mat4 ProjectionMatrix;
    vec4 BlueNoiseImageIndex_WithPadding;
} ubo;

layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outFragcolor;

const int MAX_ITERATION = 100;
const float MAX_THICKNESS = 0.1; // Increased from 0.0002 to better handle depth precision

vec3 GetPerpendicularVector(vec3 v) {
    // Find a vector not colinear with input
    vec3 axis = abs(v.x) > abs(v.z) ? vec3(0, 0, 1) : vec3(1, 0, 0);
    
    // Cross product gives perpendicular vector
    vec3 perp = cross(v, axis);
    
    // Normalize to ensure unit length
    return normalize(perp);
}

vec3 GetHemisphereSample(vec2 randVal, vec3 HitNormal) {
    vec3 bitangent = GetPerpendicularVector(HitNormal);
    vec3 tangent = cross(bitangent, HitNormal);
    float r = sqrt(randVal.x);
    float phi = 2.0 * 3.14159265 * randVal.y;
    
    return tangent * (r * cos(phi)) + bitangent * (r * sin(phi)) + HitNormal.xyz * sqrt(max(0.0, 1.0 - randVal.x));
}

float LinearizeDepth(float depth) {
    float near = 0.1; 
    float far = 200.0; 
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));    
}

vec3 FindIntersectionPoint(vec3 SamplePosInVS, vec3 DirInVS, float MaxTraceDistance) {
    vec3 EndPosInVS = SamplePosInVS + DirInVS * MaxTraceDistance;
    
    // Direction To Travel In View Space
    vec3 dp2 = EndPosInVS - SamplePosInVS;
    const float max_dist = max(abs(dp2.x), max(abs(dp2.y), abs(dp2.z)));
    
    float stepScale = 0.1; // Reduced from 4.4 for more precise stepping
    vec3 Step = (EndPosInVS.xyz - SamplePosInVS.xyz) / (max_dist / stepScale);
    
    vec3 rayPosInVS = SamplePosInVS;
    bool intersected = false;
    vec3 hitPoint = SamplePosInVS;
    
    for(int i = 0; i < MAX_ITERATION; i++) {
        rayPosInVS += Step;
        
        // Project current ray position to screen space
        vec4 rayPosPS = ubo.ProjectionMatrix * vec4(rayPosInVS, 1.0);
        rayPosPS.xyz /= rayPosPS.w;
        vec2 uv = rayPosPS.xy * 0.5 + 0.5;
        
        // Check if we're still within screen bounds
        if(any(greaterThan(uv, vec2(1.0))) || any(lessThan(uv, vec2(0.0)))) {
            break;
        }
        
        // Get depth at current UV
        vec3 scenePosVS = texture(ViewSpacePosition, uv).xyz;
        
        // Compare ray depth with scene depth
        float rayDepth = -rayPosInVS.z;
        float sceneDepth = -scenePosVS.z;
        float thickness = rayDepth - sceneDepth;
        
        if(thickness >= 0.0 && thickness < MAX_THICKNESS) {
            hitPoint = rayPosInVS;
            intersected = true;
            break;
        }
    }
    
    return intersected ? hitPoint : SamplePosInVS;
}

vec3 offsetPositionAlongNormal(vec3 ViewPosition, vec3 normal)
{
    return ViewPosition + 0.0001 * normal;
}

void main() {

    int NoiseImageIndex = int(ubo.BlueNoiseImageIndex_WithPadding.x);

    vec3 Color = texture(ColorTexture, inTexCoord).rgb;
    vec3 VSposition = texture(ViewSpacePosition, inTexCoord).rgb;
    vec3 Normal = normalize(texture(NormalTexture, inTexCoord).xyz);
    
    // Get blue noise sample
    ivec2 colorTexSize = textureSize(ColorTexture, 0);
    ivec2 noiseTexSize = colorTexSize / textureSize(BlueNoise[NoiseImageIndex], 0);
    vec2 tiledUV = inTexCoord * vec2(noiseTexSize);
    //vec2 noise = texture(BlueNoise, tiledUV).rg;
    
     vec2 noise = texture(BlueNoise[NoiseImageIndex], tiledUV).rg;

    // Get random direction in hemisphere
    vec3 stochasticNormal = GetHemisphereSample(noise, Normal);
    
    // Find intersection point in view space
    vec3 IntersectionPoint = FindIntersectionPoint(offsetPositionAlongNormal(VSposition,stochasticNormal), stochasticNormal, 10.0);
    
    // Project intersection point to screen space
    vec4 RayPositionPS = ubo.ProjectionMatrix * vec4(IntersectionPoint, 1.0);
    RayPositionPS.xyz /= RayPositionPS.w;
    vec2 uv = RayPositionPS.xy * 0.5 + 0.5;
    
     vec3 hitColor = texture(ColorTexture, uv).rgb;
     float NdotL = max(dot(Normal, normalize(IntersectionPoint - VSposition)), 0.0);
     vec3 giContribution = hitColor * NdotL * 1.5;
     
     vec3 finalColor = giContribution;
    
    outFragcolor = vec4(finalColor, 1.0);
}