#version 450

layout(push_constant) uniform PushConstants {
    mat4 ProjectionMatrix;
} pc;


layout (binding = 0) uniform sampler2D ColorTexture;
layout (binding = 1) uniform sampler2D NormalTexture;
layout (binding = 2) uniform sampler2D ViewSpacePositionTexture;
layout (binding = 3) uniform sampler2D DepthTexture;
layout (binding = 4) uniform sampler2D ReflectionMaskTexture;
layout (binding = 5) uniform sampler2D MaterialTexture;


layout (location = 0) in vec2 inTexCoord;           
layout (location = 0) out vec4 outFragcolor;

const int MAX_ITERATION = 150;
const int NUM_BINARY_SEARCH_SAMPLES = 5;
float MAX_THICKNESS = 0.0002;


void ComputeReflection(vec4 ViewPosition,vec3 ViewNormal,
                                                        out float outMaxDistance,
                                                        out vec3  outSamplePosInTS,
                                                        out vec3  outReflDirInTS ){
       
       //Get The Reflection Direction Of the Position And Normal In ViewSpace.
       vec3 viewDir   = -normalize(ViewPosition.xyz);
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

      outMaxDistance = 1;
}

vec3 BinarySearch(vec3 PrevSamplePosition,vec3 SamplePosition)
{
    	vec3 MinRaySample = PrevSamplePosition;
	    vec3 MaxRaySample = SamplePosition;
        vec3 MidRaySample;

        for (int i = 0; i < NUM_BINARY_SEARCH_SAMPLES; i++)
	        {
		              MidRaySample  = mix(MinRaySample,MaxRaySample,0.5);
                float ZBufferVal    = texture(DepthTexture, MidRaySample.xy).x; 

                if (MidRaySample.z > ZBufferVal)
			        MaxRaySample = MidRaySample;
		        else
			        MinRaySample = MidRaySample;
	        }

       return MidRaySample;
}

float FindIntersection_Linear(vec3 SamplePosInTS,vec3 RefDirInTS,float MaxTraceDistance,out vec3 Intersection){

     //Claucluate The End Position Of The Reflection In TextureSpace
     vec3 ReflectionEndPosInTS = SamplePosInTS + RefDirInTS * MaxTraceDistance; //Values Based On Outputs From Previous Functions

     //Direction To Travel In TexTure Space

     //Convert to Screen Space
     vec2 ScreenDimensions  = textureSize(ColorTexture,0);
     vec2 sampleScreenPos   = vec2(SamplePosInTS.xy        * ScreenDimensions.xy);
     vec2 endPosScreenPos   = vec2(ReflectionEndPosInTS.xy * ScreenDimensions.xy);

     //Direction To Travel In Screen Space
     vec2 dp2 = endPosScreenPos - sampleScreenPos;

     const float max_dist = max(abs(dp2.x), abs(dp2.y)); //get the maximum possible distance that will be traveled on the X or Y axis

     float stepScale = 4.4; 
     vec3 Step       = (ReflectionEndPosInTS.xyz - SamplePosInTS.xyz) / (max_dist / stepScale); // scale down steps !! look into more

     vec4 rayPosInTS  = vec4(SamplePosInTS.xyz + Step, 0);
     vec4 RayDirInTS  = vec4(Step.xyz, 0);
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

        if(hitIndex != -1) break; // if we have not hit anything Keep on Keeping on!!

         rayPosInTS += RayDirInTS; // move sampling point every itteration
    }

        bool intersected;
        
        if(hitIndex >= 0){
           intersected = true;
        };

        Intersection     = rayStartPos.xyz + RayDirInTS.xyz * hitIndex; //Get intersection point
   vec3 PrevIntersection = rayStartPos.xyz + RayDirInTS.xyz * (hitIndex - 1); //Get Previous intersection point

      if(intersected){
          Intersection  =  BinarySearch(PrevIntersection,Intersection);
      }



        float intensity = intersected ? 1 : 0;

        return intensity;
}

vec4 ComputeReflectedColor(float intensity, vec3 intersection)
{
    vec2         uv = clamp(intersection.xy, vec2(0.0), vec2(1.0));
    vec4 ssr_color  = texture(ColorTexture, uv,0);
    return ssr_color;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

void main() {
 
  vec3  Color              = texture(ColorTexture ,inTexCoord).rgb;
  vec3  Normal             = normalize(texture(NormalTexture,inTexCoord).rgb);
  vec4  ViewSpacePosition  = texture(ViewSpacePositionTexture ,inTexCoord).rgba;
  float mask               = texture(ReflectionMaskTexture,inTexCoord).g;
  vec3  MetalicRoughnessAO    = texture(MaterialTexture,inTexCoord).rgb;


  vec4 ReflectionColor = vec4(0,0,0,0);

  if(mask != 0){

         float MaxDistance_Result;
         vec3  SamplePosInTS_Result;
         vec3  ReflDirInTS_Result;
         vec3  Intersection_Result;

        ComputeReflection(ViewSpacePosition,Normal,MaxDistance_Result,SamplePosInTS_Result,ReflDirInTS_Result);
        float Intensity = FindIntersection_Linear(SamplePosInTS_Result,ReflDirInTS_Result,MaxDistance_Result,Intersection_Result);

         ReflectionColor  = ComputeReflectedColor(Intensity,Intersection_Result);
  }

     if(ReflectionColor.rgb == vec3(0,0,0) ){
        ReflectionColor.rgb =  Color;
     };


     vec3 viewDir   = -normalize(ViewSpacePosition.xyz);
     vec3 normal    = normalize(texture(NormalTexture, inTexCoord).rgb);
     float cosTheta = clamp(dot(viewDir, normal), 0.0, 1.0);
     vec3 F0        = mix(vec3(0.08),Color,MetalicRoughnessAO.r);
     
     vec3 fresnel = fresnelSchlick(cosTheta, F0);
     vec3 SSR     = ReflectionColor.rgb;

     ReflectionColor.rgb;
     vec3 finalColor = mix(Color, ReflectionColor.rgb, fresnel * 0.7); 

     outFragcolor = vec4(finalColor.rgb, 1.0);
}