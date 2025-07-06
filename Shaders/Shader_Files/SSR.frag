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

vec2 ScreenDimensions = textureSize(ColorTexture,0);


float outMaxDistance;
vec3  outSamplePosInTS;
vec3  outReflDirInTS;

void ComputeReflection(vec4 position,vec3 normal){

  vec4 ReflectionInVS = vec4(reflect(position.xyz, normal.xyz),0);
  vec4 ReflectionEndPositionInVS = position + ReflectionInVS * 1000;
       ReflectionEndPositionInVS /= (ReflectionEndPositionInVS.z < 0 ? ReflectionEndPositionInVS.z : 1);

  vec4 ReflectionEndPosInCS = pc.ProjectionMatrix * vec4(ReflectionEndPositionInVS.xyz, 1);
       ReflectionEndPosInCS /= ReflectionEndPosInCS.w;


  vec4 PositionInCS = pc.ProjectionMatrix * position;
       PositionInCS /= PositionInCS.w;

  vec3 ReflectionDir = normalize((ReflectionEndPosInCS - PositionInCS).xyz);

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
    outMaxDistance = min(outMaxDistance, tempMaxDistanceZ);
}

bool FindIntersection_Linear(vec3 SamplePosInTS,vec3 RefDirInTS,float MaxTraceDistance){

     vec3 ReflectionEndPosInTS = SamplePosInTS + RefDirInTS * MaxTraceDistance;

     vec3 dp                   = ReflectionEndPosInTS.xyz - SamplePosInTS.xyz;
 
     vec2 sampleScreenPos      = vec2(SamplePosInTS.xy        * ScreenDimensions.xy);
     vec2 endPosScreenPos      = vec2(ReflectionEndPosInTS.xy * ScreenDimensions.xy);

     vec2 dp2 = endPosScreenPos - sampleScreenPos;

     const float max_dist = max(abs(dp2.x), abs(dp2.y));
     dp /= max_dist;

     vec4 rayPosInTS  = vec4(SamplePosInTS.xyz + dp, 0);
     vec4 RayDirInTS  = vec4(dp.xyz, 0);
	 vec4 rayStartPos = rayPosInTS;


  return false;
}

void main() {
 
  vec3  Color              = texture(ColorTexture ,inTexCoord).rgb;
  vec3  Normal             = normalize(texture(NormalTexture,inTexCoord).rgb);
  vec4  Depth              = texture(DepthTexture ,inTexCoord).rgba;
  vec4  ViewSpacePosition  = texture(ViewSpacePositionTexture ,inTexCoord).rgba;
  float mask               = texture(NormalTexture,inTexCoord).a;

  vec4 SkyColor        = vec4(0,0,1,1);
  vec4 ReflectionColor = vec4(0,0,0,1);

  if(mask != 0){

        ReflectionColor = SkyColor;

        ComputeReflection(ViewSpacePosition,Normal);
        
  outFragcolor = vec4(outSamplePosInTS,1);
  }else{
     outFragcolor = vec4(0.0f);
  }
}