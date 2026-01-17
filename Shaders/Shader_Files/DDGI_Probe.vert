#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;       
layout(location = 3) in vec3 inTangent;

layout(push_constant) uniform PushConstants {
    mat4 view;
    mat4 proj;
    mat4 model;

    int   AtlasWidthSize;  
    int   ProbeSideLength; 
    int   GutterSize;      
    int   NumRays;
    int   ShowDebugStatus;
}pc;

const int PROBE_STATE_ACTIVE   = 0; //  this is for when a probe is activly collecting new scene data
const int PROBE_STATE_SLEEP    = 1; //  this is for when a probe is always hitting the skybox
const int PROBE_STATE_DISABLED = 2; //  this is for when a probe is inside an object and is not contributing meaningfully.
//A probe that is disabled can be moved so it is active

struct ProbeInformation {
    vec4 probeLocations;
    int  probeState;
    vec3 Padding;
};

layout(set = 0,binding = 1) readonly  buffer StorageBufferObject {
      ProbeInformation  ProbeData[2000];
}SBO;

layout(location = 0) out flat int ProbeIndex;
layout(location = 1) out vec2 TexCoord;
layout(location = 2) out  flat int ProbeStatus;

void main() {
     ProbeInformation probe = SBO.ProbeData[gl_InstanceIndex];

     vec4 vertexInWorldSpace = pc.model * vec4(inPosition, 1.0);

     vertexInWorldSpace.xyz += probe.probeLocations.xyz;

     ProbeIndex  = gl_InstanceIndex;
     TexCoord    = inTexCoord;
     ProbeStatus = probe.probeState;

     gl_Position = pc.proj * pc.view * vertexInWorldSpace;
}