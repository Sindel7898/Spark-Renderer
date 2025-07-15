#version 450

layout(push_constant) uniform PushConstants {
    mat4 ProjectionMatrix;
} pc;


layout (binding = 0) uniform sampler2D ColorTexture;
layout (binding = 1) uniform sampler2D NormalTexture;
layout (binding = 2) uniform sampler2D ViewSpacePositionTexture;
layout (binding = 3) uniform sampler2D DepthTexture;

layout (location = 0) in vec2 inTexCoord;           
layout (location = 0) out vec4 outFragcolor;

const int MAX_ITERATION = 150;
float MAX_THICKNESS = 0.0002;

void ComputeReflection(vec4 ViewPosition,vec3 ViewNormal,
                                                        out float outMaxDistance,
                                                        out vec3  outSamplePosInTS,
                                                        out vec3  outReflDirInTS ){
       
       //Get The Reflection Direction Of the Position And Normal In ViewSpace.
       vec3 viewDir   = normalize(ViewPosition.xyz);
  vec4 ReflectionInVS = vec4(reflect(viewDir, ViewNormal.xyz),0);

       //Get End Point Of Reflection In ViewSpace
  vec4 ReflectionEndPositionInVS = ViewPosition + ReflectionInVS * 300;

       //Convert The End Position From ViewSpace To Clip Space
  vec4 ReflectionEndPosInCS = pc.ProjectionMatrix * vec4(ReflectionEndPositionInVS.xyz, 1);
       ReflectionEndPosInCS /= ReflectionEndPosInCS.w;

       //Convert The Current Position From ViewSpace To Clip Space
  vec4 PositionInCS = pc.ProjectionMatrix * ViewPosition;
       PositionInCS /= PositionInCS.w;

       //Reflection Direction In Clip Space Computed
  vec3 ReflectionDir = normalize((ReflectionEndPosInCS - PositionInCS).xyz);

       //Convert Position From Clip Space To Texture Space
       PositionInCS.xy  *= vec2(0.5f, -0.5f);
       PositionInCS.xy  += vec2(0.5f, 0.5f); 
       
       ReflectionDir.xy *= vec2(0.5f, -0.5f);
       
       outSamplePosInTS = PositionInCS.xyz;
       outReflDirInTS   = ReflectionDir;

    // Compute the maximum distance to trace before the ray goes outside of the visible area.
    if (outReflDirInTS.x >= 0) {
        outMaxDistance = (1 - outSamplePosInTS.x) / outReflDirInTS.x;
    } else {
        outMaxDistance = -outSamplePosInTS.x / outReflDirInTS.x;
    }
    
    float tempMaxDistanceY;

    if (outReflDirInTS.y < 0) {
        tempMaxDistanceY = -outSamplePosInTS.y / outReflDirInTS.y;
    } else {
        tempMaxDistanceY = (1 - outSamplePosInTS.y) / outReflDirInTS.y;
    }
    outMaxDistance = min(outMaxDistance, tempMaxDistanceY);
    
    float tempMaxDistanceZ;

    if (outReflDirInTS.z < 0) {
        tempMaxDistanceZ = -outSamplePosInTS.z / outReflDirInTS.z;
    } else {
        tempMaxDistanceZ = (1 - outSamplePosInTS.z) / outReflDirInTS.z;
    }
    outMaxDistance = 20;
}


float FindIntersection_Linear(vec3 SamplePosInTS,vec3 RefDirInTS,float MaxTraceDistance,out vec3 Intersection){

     //Claucluate The End Position Of The Reflection In TextureSpace
     vec3 ReflectionEndPosInTS = SamplePosInTS + RefDirInTS * MaxTraceDistance; //Values Based On Outputs From Previous Functions

     //Direction To Travel In TexTure Space
     vec3 dp                   = ReflectionEndPosInTS.xyz - SamplePosInTS.xyz; 
     vec2 ScreenDimensions = textureSize(ColorTexture,0);

     //Come back to this Section 
     vec2 sampleScreenPos      = vec2(SamplePosInTS.xy        * ScreenDimensions.xy);
     vec2 endPosScreenPos      = vec2(ReflectionEndPosInTS.xy * ScreenDimensions.xy);

     vec2 dp2 = endPosScreenPos - sampleScreenPos;

     const float max_dist = max(abs(dp2.x), abs(dp2.y));

     float stepScale = 2.5; 
     dp /= (max_dist / stepScale);

     vec4 rayPosInTS  = vec4(SamplePosInTS.xyz + dp, 0);
     vec4 RayDirInTS  = vec4(dp.xyz, 0);
	 vec4 rayStartPos = rayPosInTS;

     // Version 0.
    int hitIndex = -1;

    for(int i = 0; i < max_dist && i < MAX_ITERATION; i++)
    {
	    float depth = texture(DepthTexture, rayPosInTS.xy).x; // ray hit  current depth
	    float thickness = rayPosInTS.z - depth; // rays current depth - hit depth
        

	    if(thickness >= 0 && thickness < MAX_THICKNESS) // if we hit somthinging increase the index
	    {
	        hitIndex = i;
	    }

        if(hitIndex != -1) break;

         rayPosInTS += RayDirInTS; // move sampling point every itteration
    }


        bool intersected = hitIndex >= 0;
             Intersection = rayStartPos.xyz + RayDirInTS.xyz * hitIndex;

        float intensity = intersected ? 1 : 0;

        return intensity;
}

vec4 ComputeReflectedColor(float intensity, vec3 intersection)
{
    vec4 ssr_color = texture(ColorTexture, intersection.xy);
    return ssr_color;
}

void main() {
 
  vec3  Color              = texture(ColorTexture ,inTexCoord).rgb;
  vec3  Normal             = normalize(texture(NormalTexture,inTexCoord).rgb);
  vec4  Depth              = texture(DepthTexture ,inTexCoord).rgba;
  vec4  ViewSpacePosition  = texture(ViewSpacePositionTexture ,inTexCoord).rgba;
  float mask               = texture(NormalTexture,inTexCoord).a;

   vec4 ReflectionColor = vec4(0,0,0,1);

  if(mask != 0){

         float MaxDistance_Result;
         vec3  SamplePosInTS_Result;
         vec3  ReflDirInTS_Result;
         vec3  Intersection_Result;

        ComputeReflection(ViewSpacePosition,Normal,MaxDistance_Result,SamplePosInTS_Result,ReflDirInTS_Result);
        float Intensity = FindIntersection_Linear(SamplePosInTS_Result,ReflDirInTS_Result,MaxDistance_Result,Intersection_Result);

         ReflectionColor  = ComputeReflectedColor(Intensity,Intersection_Result);
  
  }

     outFragcolor  = vec4(Color,1.0f) + ReflectionColor;
}