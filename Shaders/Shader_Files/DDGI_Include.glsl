
const int PROBE_STATE_ACTIVE   = 0; //  this is for when a probe is activly collecting new scene data
const int PROBE_STATE_SLEEP    = 1; //  this is for when a probe is always hitting the skybox
const int PROBE_STATE_DISABLED = 2; //  this is for when a probe is inside an object and is not contributing meaningfully.
//A probe that is disabled can be moved so it is active


//https://stackoverflow.com/questions/1730961/convert-a-2d-array-index-into-a-1d-index
//convert the current pixel into an index for the probes
int get_probe_index_from_pixels(ivec2 pixel_coords, int atlas_width, int probe_full_size) {
    
    int   probes_per_row      = atlas_width  / probe_full_size; // How many probes can  be fit in one row of the atlas
    ivec2 probe_grid_coords   = pixel_coords / probe_full_size; // What probe in the atlas am i in ?

    return probe_grid_coords.y * probes_per_row + probe_grid_coords.x; // scale it 
}

// Returns a unit vector. Argument o is an octahedral vector packed via oct_encode,     
// on the [-1, +1] square                                                               
float sign_not_zero(in float k) {                                                       
    return (k >= 0.0) ? 1.0 : -1.0;                                                     
}                                                                                       
                                                                                        
vec2 sign_not_zero2(in vec2 v) {                                                        
    return vec2(sign_not_zero(v.x), sign_not_zero(v.y));                                
}                                                                                       
                                                                                        
vec3 oct_decode(vec2 o) {                                                               
    vec3 v = vec3(o.x, o.y, 1.0 - abs(o.x) - abs(o.y));                                 
    if (v.z < 0.0) {                                                                    
        v.xy = (1.0 - abs(v.yx)) * sign_not_zero2(v.xy);                                
    }                                                                                   
    return normalize(v);                                                                
}                                                                                       

vec2 oct_encode(in vec3 v) {
    float l1norm = abs(v.x) + abs(v.y) + abs(v.z);
    vec2 result = v.xy * (1.0 / l1norm);
    if (v.z < 0.0) {
        result = (1.0 - abs(result.yx)) * sign_not_zero2(result.xy);
    }
    return result;
}

// Code adapted from:
// "Mastering Graphics Programming with Vulkan" by Packt Publishing
// GitHub repository: https://github.com/PacktPublishing/Mastering-Graphics-Programming-with-Vulkan
// Source file: source/chapter14/shaders/ddgi.glsl and source/chapter14/shaders/ddgi.h
// License: MIT License (see repository for details)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Computes normalized texel coordinates within a single probe’s quad in the atlas.
// This just tells us where inside the probe we are, in the range [-1, +1]^2. for guttar//
vec2 get_normalized_probe_texel_coord(ivec2 fragCoord,int ProbeSideLength, int GutterSize) {                                                                                           
                                                                                                                                           
    int probe_with_border_side  = ProbeSideLength + GutterSize; // How Long is a total probe?                          
                                                              
       ///Where are we exactly?
    float texel_coordinatesX =   (fragCoord.x - 1) % probe_with_border_side;
    float texel_coordinatesY =   (fragCoord.y - 1) % probe_with_border_side;
    
    vec2 Texel = vec2(texel_coordinatesX,texel_coordinatesY);
    
    Texel += vec2(0.5f); // Adjustment so we are sampling in the middle of the pixel 
    Texel = (Texel / float(ProbeSideLength)) * 2.0 - 1.0; // map out of texture space to normalized direction-space
                                                                                      
    return Texel;                                                                                  
}   

ivec2 GetProbeTexel(vec2 uv, ivec3 probeIndex, int ProbeSideLength, int GutterSize, int AtlasWidthSize, ivec3 ProbeCount) {
    int probe_full_size = ProbeSideLength + GutterSize;
    int probes_per_row  = AtlasWidthSize / probe_full_size;

    int linear_index = probeIndex.x + probeIndex.y * ProbeCount.x + probeIndex.z * (ProbeCount.x * ProbeCount.y);

    ivec2 probe_origin = ivec2((linear_index % probes_per_row) * probe_full_size,
                               (linear_index / probes_per_row) * probe_full_size);

    ivec2 border_offset = ivec2(GutterSize / 2);
    vec2 pixel_pos      = uv * float(ProbeSideLength);
    ivec2 pixel_offset  = clamp(ivec2(pixel_pos), ivec2(0), ivec2(ProbeSideLength - 1));

    return probe_origin + (border_offset + pixel_offset);
}

vec2 GetProbeUV(vec2 octant_uv, ivec3 probeIndex, int ProbeSideLength, int GutterSize, int AtlasWidth, ivec3 ProbeCount) {
    int probe_full_size = ProbeSideLength + GutterSize;
    int probes_per_row  = AtlasWidth / probe_full_size;
    
    int linear_index = probeIndex.x + (probeIndex.y * ProbeCount.x) + (probeIndex.z * ProbeCount.x * ProbeCount.y);

    vec2 probe_origin = vec2(
        (linear_index % probes_per_row) * probe_full_size,
        (linear_index / probes_per_row) * probe_full_size
    );

    vec2 border_offset = vec2(float(GutterSize) * 0.5);

    vec2 pixel_pos_inside_probe = octant_uv * float(ProbeSideLength);

    vec2 final_pixel_pos = probe_origin + border_offset + pixel_pos_inside_probe;
    
    return final_pixel_pos / float(AtlasWidth);
}


vec3 SampleIrradiance(sampler2D IrradianceTexture,
                      sampler2D VisibilityTexture, 
                      vec3 Position, vec3 Normal, 
                      vec3 GridBaseLocation, 
                      vec3 ProbeSpacing,
                      ivec3 ProbeCount,
                      int IrrSideLength,
                      int IrrGutterSize,
                      int IrrAtlasWidth,
                      int VisSideLength,
                      int VisGutterSize,
                      int VisAtlasWidth, 
                      vec3 Camera_Position)
{
   float min_spacing = min(ProbeSpacing.x, min(ProbeSpacing.y, ProbeSpacing.z)); 
   vec3 SamplePosition = Position + (Normal * min_spacing * 0.35);

    vec3 GridIndexF = (SamplePosition - GridBaseLocation) / ProbeSpacing;
    vec3 Alpha = fract(GridIndexF);
    ivec3 GridIndex = ivec3(floor(GridIndexF));

    ivec3 Offsets[8] = ivec3[](
        ivec3(0,0,0), ivec3(1,0,0), ivec3(0,1,0), ivec3(1,1,0),
        ivec3(0,0,1), ivec3(1,0,1), ivec3(0,1,1), ivec3(1,1,1)
    );

    vec4 Irradiance = vec4(0);
    vec2 Normaluv = (oct_encode(normalize(Normal)) * 0.5) + 0.5;

    for(int i = 0; i < 8; i++){
        ivec3 Offset = Offsets[i];
        ivec3 ProbeIndex = clamp((GridIndex + Offset), ivec3(0), ProbeCount - 1);
        vec3 ProbeWorldPosition = GridBaseLocation + (vec3(ProbeIndex) * ProbeSpacing);

        vec2 irradiance_uv = GetProbeUV(Normaluv, ProbeIndex, IrrSideLength, IrrGutterSize, IrrAtlasWidth, ProbeCount);

        vec3 probeIrradiance = textureLod(IrradianceTexture, irradiance_uv, 0.0).rgb;
        probeIrradiance = sqrt(probeIrradiance);

        vec3 probeToSample = SamplePosition - ProbeWorldPosition;
        float distToProbe = length(probeToSample);
        vec3 dir = -probeToSample / distToProbe;

        float weight = (dot(dir, Normal) + 1.0) * 0.5;
        weight = (weight * weight) + 0.2; 

        vec3 CornerWeight = mix(vec3(1.0) - Alpha, Alpha, vec3(Offset));
        float TrilinearWeight = CornerWeight.x * CornerWeight.y * CornerWeight.z + 0.001f;
        weight *= TrilinearWeight;

        vec2 Diruv = (oct_encode(normalize(-dir)) * 0.5) + 0.5;
        
        vec2 visibility_uv = GetProbeUV(Diruv, ProbeIndex, VisSideLength, VisGutterSize, VisAtlasWidth, ProbeCount);
        vec2 DepthInfo = textureLod(VisibilityTexture, visibility_uv, 0.0).rg;

        float Mean  = DepthInfo.x; 
        float Mean2 = DepthInfo.y; 
        float chebyshev_weight = 1.0;
        float r_biased = distToProbe - 0.05;  

        if(r_biased > Mean) {
            float variance = max(Mean2 - (Mean * Mean), 0.0);
            const float distanceDiff = distToProbe - Mean;
            chebyshev_weight = variance / (variance + (distanceDiff * distanceDiff));
            
            chebyshev_weight = max(pow(chebyshev_weight, 3.0), 0.0);
        }
        
        chebyshev_weight = max(0.0, chebyshev_weight);
        weight *= chebyshev_weight;

        const float crushThreshold = 0.2;
        if (weight < crushThreshold) {
            weight *= (weight * weight) * (1.0 / (crushThreshold * crushThreshold)); 
        }

        Irradiance += vec4(probeIrradiance * weight, weight);
    }
     
    vec3 ComputedIrradiance = (Irradiance.rgb * (1.0 / max(Irradiance.a, 0.0001)));
    
    ComputedIrradiance = ComputedIrradiance * ComputedIrradiance;
    
    return ComputedIrradiance * 2.0 * 3.14159;
}