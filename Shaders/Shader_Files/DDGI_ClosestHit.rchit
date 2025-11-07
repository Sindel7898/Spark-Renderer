#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
const float PI = 3.14159265359;

layout(set = 0, binding = 1)  uniform sampler2D         Albedo_AssetImages[];
layout(set = 0, binding = 2)  uniform sampler2D         Normal_AssetImages[];
layout(set = 0, binding = 3)  uniform sampler2D         MetalicRoughness_AssetImages[];
layout(set = 0, binding = 13,rgba16f) uniform readonly image2D  IrradianceStorageImage;
layout(set = 0, binding = 14,rgba16f) uniform readonly image2D  VisibilityStorageImage;

struct Vertex {
    vec4 position_Padding;
    vec4 texCoord_Padding;
    vec4 normal_Padding;
    vec4 tangent_Padding;
};

struct VertexAndIndexOffsets {

    uint VertexOffset;
    uint IndexOffset;
};

layout(set = 0, binding = 4) buffer IndexBufferSSBO {
    uint Indices[];
} indexBuffer;

layout(set = 0, binding = 5) buffer VertexBufferSSBO {
    Vertex vertices[];
} vertexBuffer;

layout(set = 0, binding = 6) buffer VertexIndexOffsetBufferSSBO {
    VertexAndIndexOffsets Offsets[];

} OffsetBuffer;

layout(set = 0, binding = 7) uniform Transformation {
    mat4 WorldMatrix[10];
} Transformations;


struct LightData{
    vec4    positionAndLightType;
    vec4    colorAndAmbientStrength;
    vec4    CameraPositionAndLightIntensity;
};

layout(set = 0, binding = 11) uniform LightUniformBuffer {

   LightData lights[4];
};


layout(push_constant) uniform PushConstant{
    vec4 GridBaseLocation_ScreenSizeWidth;
    vec4 ProbeSpacing_ScreenSizeHeight;
    vec4 ProbeCount;

    ///////////////////////

    int   AtlasWidthSize;  
    int   ProbeSideLength; 
    int   GutterSize;      
    int   NumRays;
    vec4 UseInfiniteBounce_infinite_bounces_multiplier_Padding;
}pc;

/////Move these stuff to header to be reused. bit cramped here
float sign_not_zero(in float k) {
    return (k >= 0.0) ? 1.0 : -1.0;
}

vec2 sign_not_zero2(in vec2 v) {
    return vec2(sign_not_zero(v.x), sign_not_zero(v.y));
}

vec2 oct_encode(in vec3 v) {
    float l1norm = abs(v.x) + abs(v.y) + abs(v.z);
    vec2 result = v.xy * (1.0 / l1norm);
    if (v.z < 0.0) {
        result = (1.0 - abs(result.yx)) * sign_not_zero2(result.xy);
    }
    return result;
}

ivec2 GetProbeTexel( vec3 direction, ivec3 probeIndex)
{

    vec2 octahedral_coordinates = oct_encode(normalize(direction));
    // 2. Map it to [0, 1]
     octahedral_coordinates = (octahedral_coordinates * 0.5) + 0.5f;

    int probeSideWithGutter = pc.ProbeSideLength + pc.GutterSize;
    int probesPerRow        = pc.AtlasWidthSize / probeSideWithGutter;
    int linearIndex         = probeIndex.x + probeIndex.y * int(pc.ProbeCount.x) + probeIndex.z * int(pc.ProbeCount.x * pc.ProbeCount.y);

    int cellOffsetX = (linearIndex % probesPerRow) * probeSideWithGutter;
    int cellOffsetY = (linearIndex / probesPerRow) * probeSideWithGutter;


    vec2 internalPixelOffset = octahedral_coordinates * float(pc.ProbeSideLength);

    int   border_offset = pc.GutterSize / 2; 
    ivec2 TopLeft = ivec2(cellOffsetX, cellOffsetY) + ivec2(border_offset);

    ivec2 FinalTexel = TopLeft + ivec2(internalPixelOffset) ;

    return FinalTexel;
}
////////////////////////////////////

vec3 SampleIrradiance( vec3 Position, vec3 Normal)
{

    vec3  GridBaseLocation = pc.GridBaseLocation_ScreenSizeWidth.xyz;
    vec3  ProbeSpacing     = pc.ProbeSpacing_ScreenSizeHeight.xyz;
    ivec3 ProbeCount       = ivec3(pc.ProbeCount.xyz);

    vec3 GridIndexF = (Position - GridBaseLocation) / ProbeSpacing;
    vec3 Alpha = fract(GridIndexF);


    ivec3 GridIndex = ivec3(GridIndexF);

     ivec3 Offsets[8] = {
         ivec3(0,0,0), ivec3(1,0,0),
         ivec3(0,1,0), ivec3(1,1,0),
         ivec3(0,0,1), ivec3(1,0,1),
         ivec3(0,1,1), ivec3(1,1,1)
     };


     vec4 Irradiance = vec4(0);

     for(int i = 0; i < 8; i++){

         ivec3 ProbeIndex        = clamp((GridIndex + Offsets[i]),ivec3(0),ProbeCount - 1);
         vec3 ProbeWorldPosition = GridBaseLocation + (vec3(ProbeIndex) * ProbeSpacing);

         ivec2 irradiance_texel = GetProbeTexel(Normal,ProbeIndex);

         vec3 dir = Position  -  ProbeWorldPosition ;
         float r = length(dir);
         dir *= -1.0 / r;


         float weight = (dot(dir, Normal) + 1) * 0.5;
         weight = (weight * weight) + 0.2;

  
          vec3 trilinear = mix(vec3(1.0) - Alpha, Alpha, vec3(Offsets[i]));
         float trilinearWeight = trilinear.x * trilinear.y * trilinear.z;

         weight *= (trilinearWeight + 0.001f);

          //////////////////////////////////////////////////////////////
          ivec2 visibility_texel = GetProbeTexel(dir, ProbeIndex);
          vec2 DepthInfo =  imageLoad(VisibilityStorageImage, visibility_texel).rg;

          float Mean  = DepthInfo.x; 
          float Mean2 = DepthInfo.y; 
         
         
         ///////Same as how shadow maps are calculated
         float chebyshev_weight = 1.0;

         if(r >  Mean) {
         
             float variance = abs((Mean * Mean) - Mean2);
             const float distanceDiff = r - Mean;
             chebyshev_weight = variance / (variance + (distanceDiff * distanceDiff));
               
             chebyshev_weight = max((chebyshev_weight * chebyshev_weight * chebyshev_weight), 0.0f);
         }


         chebyshev_weight = max(0.05f, chebyshev_weight);
         weight *= chebyshev_weight;

         vec3 probeIrradiance = sqrt(imageLoad(IrradianceStorageImage, irradiance_texel).rgb) * weight;
         Irradiance += vec4(probeIrradiance, weight);
     }
     
       vec3 ComputedIrradiance = (Irradiance.rgb * (1.0 / Irradiance.a));

       ComputedIrradiance = ComputedIrradiance * ComputedIrradiance;

       return ComputedIrradiance;
}



struct Payload {
    vec3  Color;
    float Distance;
    int Hit;
};

layout(location = 0) rayPayloadInEXT Payload payload;

hitAttributeEXT struct HitAttribute {
    vec2 hitUV;
} attribs;

void main()
{

   vec3 Radiance  = vec3(0.0);
   float Distance = 0;

  if(gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT){ // if we hit the inside of a triangle get the backface
      Distance = gl_RayTminEXT + gl_HitTEXT; //minimum traversel legth + gl_HitTEXT (contains the t-value of an intersection along the ray)
      Distance *= -0.2; // negated the value so that it is always -
 }else{
       uint packed  = gl_InstanceCustomIndexEXT;
   
       uint objectID   = packed >> 12;
       uint primitiveID = packed & 0xFFF;
   
       VertexAndIndexOffsets offsets = OffsetBuffer.Offsets[primitiveID];
       
       uint baseIndex = 3 * gl_PrimitiveID + offsets.IndexOffset;
   
       ivec3 index = ivec3(
           indexBuffer.Indices[baseIndex  + 0],
           indexBuffer.Indices[baseIndex  + 1],
           indexBuffer.Indices[baseIndex  + 2]
       );
   
       Vertex v0 = vertexBuffer.vertices[index.x];
       Vertex v1 = vertexBuffer.vertices[index.y];
       Vertex v2 = vertexBuffer.vertices[index.z];
   
       vec3 bary = vec3(1.0 - attribs.hitUV.x - attribs.hitUV.y, attribs.hitUV.x, attribs.hitUV.y);
   
         vec3 VertexPosition = 
           v0.position_Padding.xyz * bary.x +
           v1.position_Padding.xyz * bary.y +
           v2.position_Padding.xyz * bary.z;
   
   
       vec3 Normal = normalize(
           v0.normal_Padding.xyz * bary.x +
           v1.normal_Padding.xyz * bary.y +
           v2.normal_Padding.xyz * bary.z
       );
   
        vec3 Tangent = normalize(
           v0.tangent_Padding.xyz * bary.x +
           v1.tangent_Padding.xyz * bary.y +
           v2.tangent_Padding.xyz * bary.z
       );
   
        vec2 TexCoord = 
          v0.texCoord_Padding.xy * bary.x +
          v1.texCoord_Padding.xy * bary.y +
          v2.texCoord_Padding.xy * bary.z;
   
   
       mat3 normalMatrix  = transpose(inverse(mat3(Transformations.WorldMatrix[objectID])));
       vec3 WorldN        = normalize(normalMatrix * Normal);
       vec3 WorldT        = normalize(normalMatrix * Tangent);
       vec3 WorldB        = cross(WorldN,WorldT);
   
       mat3 WorldSpaceTBN = mat3(WorldT, WorldB, WorldN);
   
       vec3  Albedo     = texture(Albedo_AssetImages         [nonuniformEXT(primitiveID)], TexCoord).rgb;
       float Metallic  = texture(MetalicRoughness_AssetImages[nonuniformEXT(primitiveID)], TexCoord).r;
       float Roughness = texture(MetalicRoughness_AssetImages[nonuniformEXT(primitiveID)], TexCoord).r;
   
       vec3 NormalTexture = texture(Normal_AssetImages[nonuniformEXT(primitiveID)], TexCoord).rgb * 2.0 - vec3(1.0);
       vec3 tnorm = normalize(WorldSpaceTBN * NormalTexture);
       Normal = tnorm;
       /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   
       vec3  LightDir = vec3(1,1,1);
       const float Constant   = 1.0;
       const float Linear     = 0.09;
       const float Quadratic  = 0.032;
       
       vec3  radiance  = vec3(0.0);
       vec4  WorldPos  =  Transformations.WorldMatrix[objectID] * vec4(VertexPosition,1);
   
       for (int i = 0; i < 4; i++) {
     
           LightData light = lights[i];
           
           
           vec3 Lo      = vec3(0.0);
           
           
            if(light.positionAndLightType.w < 0.5){
           
               LightDir = normalize(-light.positionAndLightType.xyz);
               radiance = light.colorAndAmbientStrength.rgb ;
           
             }
            else if (light.positionAndLightType.w > 0.5){
                     
                 vec3 LightPos   = light.positionAndLightType.xyz;
                     LightDir    = normalize(LightPos - WorldPos.xyz);
               float Distance    = length(LightPos -  WorldPos.xyz);
               float Attenuation = 1.0 / (Constant + Linear * Distance + Quadratic * (Distance * Distance));
                     radiance    = light.colorAndAmbientStrength.rgb * Attenuation;
             }  
           
           float NdotL = max(dot(Normal, LightDir), 0.0);        
                   Lo +=  Albedo * radiance * NdotL;

        Radiance +=  ((Lo) * light.CameraPositionAndLightIntensity.a);
     }
     
       int UseInfiniteBounce =  int(pc.UseInfiniteBounce_infinite_bounces_multiplier_Padding.x);

        if(UseInfiniteBounce > 0.5){
          Radiance += Albedo * SampleIrradiance(WorldPos.xyz,Normal) *  pc.UseInfiniteBounce_infinite_bounces_multiplier_Padding.y;
        }

     Distance = gl_RayTminEXT + gl_HitTEXT;
   }

   Radiance = 
     payload.Color    = Radiance;
     payload.Distance = Distance;
     payload.Hit = 1;
 }