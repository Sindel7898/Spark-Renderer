#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;       
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBiTangent;

layout(push_constant) uniform PushConstants {
    float time;
} pc;

layout(set = 0,binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

struct InstanceData{
       mat4 Model;
};

layout(set = 0,binding = 1) readonly buffer  GrassData {
  InstanceData GrassInstanceData[20000];
};
layout(binding = 2) uniform sampler2D TerrainHeightMap;

vec3 Cubic_Bezier(vec3 Point0, vec3 Point1, vec3 Point2, vec3 Point3, float t) {

    vec3 a = mix(Point0, Point1, t);
    vec3 b = mix(Point1, Point2, t);
    vec3 c = mix(Point2, Point3, t);

    vec3 d = mix(a, b, t);
    vec3 e = mix(b, c, t);

    return mix(d, e, t);
}

layout(location = 0) out vec4 OutPosition;
layout(location = 1) out vec3 OutNormal;
layout(location = 2) out float OutCurrentHeight;

void main() {
    
    mat4 model = GrassInstanceData[gl_InstanceIndex].Model;

   float bladeHeight = 0.7; // or pass in maximum Y
   float t = clamp(inPosition.y / bladeHeight, 0.0, 1.0);

   float swayPhase = pc.time * 2.2 + float(gl_InstanceIndex) * 0.1;
   float swayAmount = sin(swayPhase) * 0.2 * t;
   
   vec3 p0 = inPosition;
   vec3 p1 = p0 + vec3(swayAmount * 0, 0, 0);
   vec3 p2 = p0 + vec3(swayAmount * 0.6, 0, 0);
   vec3 p3 = p0 + vec3(swayAmount, 0, 0);

   
   vec3 pointOnCurve = Cubic_Bezier(p0, p1, p2, p3, t);

   vec4 worldPos = model * vec4(pointOnCurve, 1.0);

     
   vec3 worldNormal =  transpose(inverse(mat3(model))) * inNormal;

  OutNormal = worldNormal;

  OutPosition = worldPos;
  OutCurrentHeight = inPosition.y;

   gl_Position = ubo.proj * ubo.view * worldPos;
}