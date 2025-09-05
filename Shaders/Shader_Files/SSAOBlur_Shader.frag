#version 450

layout (binding = 0) uniform sampler2D samplerSSAO;

layout(location = 0) in vec2 inTexCoord;           

layout (location = 0) out vec4 outFragcolor;

layout (push_constant) uniform PushConsts {
    vec2 direction; // blur direction
} pc;

vec4 blur13(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) {
   
      vec4 color = vec4(0.0);
      vec2 off1 = vec2(1.411764705882353) * direction;
      vec2 off2 = vec2(3.2941176470588234) * direction;
      vec2 off3 = vec2(5.176470588235294) * direction;
      color += textureLod(image, uv,0) * 0.1964825501511404;
      color += textureLod(image, uv + (off1 / resolution),0) * 0.2969069646728344;
      color += textureLod(image, uv - (off1 / resolution),0) * 0.2969069646728344;
      color += textureLod(image, uv + (off2 / resolution),0) * 0.09447039785044732;
      color += textureLod(image, uv - (off2 / resolution),0) * 0.09447039785044732;
      color += textureLod(image, uv + (off3 / resolution),0) * 0.010381362401148057;
      color += textureLod(image, uv - (off3 / resolution),0) * 0.010381362401148057;
     
     return color;
}

void main() {
    vec2 texelSize = vec2(textureSize(samplerSSAO, 0));

    outFragcolor = blur13(samplerSSAO, inTexCoord, texelSize, pc.direction);
}