#version 450

layout(push_constant) uniform PushConstants {
    vec3 LightColor;
} pc;

layout(location = 0) out vec4 outColor;

void main() {

    outColor = vec4(pc.LightColor,1);
}