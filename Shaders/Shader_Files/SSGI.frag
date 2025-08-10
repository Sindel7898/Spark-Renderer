#version 450


layout (binding = 0) uniform sampler2D NormalTexture;
layout (binding = 1) uniform sampler2D ViewSpacePosition;
layout (binding = 2) uniform sampler2D DepthTexture;
layout (binding = 3) uniform sampler2D ColorTexture;
layout (binding = 4) uniform sampler2D BlueNoise;

layout (binding = 5) uniform SSGIUniformBuffer{
    mat4 ProjectionMatrix;
}ubo;


layout (location = 0) in vec2 inTexCoord;           
layout (location = 0) out vec4 outFragcolor;

const int MAX_ITERATION = 150;
float MAX_THICKNESS = 0.0002;

vec3 GetPerpendicularVector(vec3 v)
{
    // Find a vector not colinear with input
    vec3 axis = abs(v.x) > abs(v.z) ? vec3(0, 0, 1) : vec3(1, 0, 0);
    
    // Cross product gives perpendicular vector
    vec3 perp = cross(v, axis);
    
    // Normalize to ensure unit length
    return normalize(perp);
}

vec3 GetHemisphereSample(vec2 randVal, vec3 HitNormal){
	
    vec3 bitangent = GetPerpendicularVector(HitNormal);
    vec3 tangent = cross(bitangent, HitNormal);
    float r = sqrt(randVal.x);
    float phi = 2.0f * 3.14159265f * randVal.y;
    
    return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + HitNormal.xyz * sqrt(max(0.0, 1.0f - randVal.x));
}

float LinearizeDepth(float depth) 
{
    float near = 0.1; 
    float far  = 200.0; 

    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

vec3 FindIntersectionPoint(vec3 SamplePosInVS,vec3 DirInVS,float MaxTraceDistance){

     vec3 EndPosInVS = SamplePosInVS + DirInVS * MaxTraceDistance;


     //Direction To Travel In View Space
     vec3 dp2 = EndPosInVS - SamplePosInVS;

     const float max_dist = max(abs(dp2.x), abs(dp2.y)); //get the maximum possible distance that will be traveled on the X or Y axis

     float stepScale = 4.4; 
     vec3 Step       = (EndPosInVS.xyz - SamplePosInVS.xyz) / (max_dist / stepScale); // scale down steps !! look into more

     vec4 rayPosInVS  = vec4(SamplePosInVS.xyz + Step, 0);
     vec4 RayDirInVS  = vec4(Step.xyz, 0);
	 vec4 rayStartPos = rayPosInVS;

     // Version 0.
    int hitIndex = -1;

    for(int i = 0; i < max_dist && i < MAX_ITERATION; i++)
    {

        vec4 RayPositionPS = RayDirInVS * ubo.ProjectionMatrix;
        vec2 uv = RayPositionPS.xy / RayPositionPS.z; // Perspective divide (view-space to UV)
             uv = uv * 0.5 + 0.5; // Map to [0,1] range

	    float depthtemp = texture(DepthTexture, uv.xy).r; // ray hit  current depth
        float depth = -LinearizeDepth(depthtemp);

	    float thickness = rayPosInVS.z - depth; // rays current depth - hit depth 
        
	    if(thickness >= 0 && thickness < MAX_THICKNESS) // if we hit somthinging increase the index
	    {
	        hitIndex = i;
	    }

        if(hitIndex != -1) break; // if we have not hit anything Keep on Keeping on!!

         rayPosInVS += RayDirInVS; // move sampling point every itteration
    }

        bool intersected;
        
        if(hitIndex >= 0){
           intersected = true;
        };


        return rayStartPos.xyz + RayDirInVS.xyz * hitIndex;;
}

void main() {
 
  vec3  Color       = texture(ColorTexture ,inTexCoord).rgb;
  vec3  VSposition  = texture(ViewSpacePosition ,inTexCoord).rgb;
  vec3  Normal      =  normalize(texture(NormalTexture, inTexCoord).xyz);

   vec2 tiledUV = inTexCoord * (textureSize(ColorTexture, 0) / textureSize(BlueNoise, 0));
   vec2 noise = texture(BlueNoise, tiledUV).rg;

  vec3 stochasticNormal  = GetHemisphereSample(noise,Normal);

  vec3 IntersectionPoint =  FindIntersectionPoint(VSposition,stochasticNormal,1000);

 vec4 RayPositionPS = vec4(IntersectionPoint,1) * ubo.ProjectionMatrix;
 vec2 uv = RayPositionPS.xy / RayPositionPS.z; // Perspective divide (view-space to UV)
      uv = uv * 0.5 + 0.5; // Map to [0,1] range

   
   vec3  GI   = texture(ColorTexture ,uv).rgb;

   outFragcolor = vec4(GI,1.0);
}