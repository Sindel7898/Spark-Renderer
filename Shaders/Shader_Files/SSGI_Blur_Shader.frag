#version 450

layout (binding = 0) uniform sampler2D samplerta_SSGI;
layout (binding = 1) uniform sampler2D samplerViewSpacePosition;

layout (location = 0) in vec2 inTexCoord;           
layout (location = 0) out vec4 outFragcolor;

// pass blur direction from outside: (1,0) for horizontal, (0,1) for vertical
layout (push_constant) uniform PushConsts {
    int Direction;
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
    vec2 texelSize = 1.0 / vec2(textureSize(samplerta_SSGI, 0));

    vec3 color  = texture(samplerta_SSGI,inTexCoord).rgb;
    float Depth  = texture(samplerViewSpacePosition,inTexCoord).b;

    //outFragcolor = blur13(samplerta_SSGI, inTexCoord, texelSize, pc.direction);

    vec3 result = vec3(0.0);
    float totalWeight = 0.0;

    float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

    int uBlurSize = 1;

    bool Direction = true;

    if(pc.Direction == 0){ Direction = false;}
    float MainDepth   = texture(samplerViewSpacePosition, inTexCoord,0).b ;

    for(int i = -4; i <= 4; i++)
    {
        int idx = abs(i);
        vec2 offset = Direction    ? vec2(texelSize.x * i * uBlurSize, 0.0) 
                                   : vec2(0.0, texelSize.y * i * uBlurSize);

        float OffsetDepth = texture(samplerViewSpacePosition, inTexCoord + offset,0).b ;

        float w = weights[idx];

        float Difference = abs(MainDepth - OffsetDepth);
        if(Difference < 0.1){
            result += texture(samplerta_SSGI, inTexCoord + offset).rgb * w; 
            totalWeight += w;
        }
    }

    if(totalWeight > 0.0) {
       result /= totalWeight;
     }

    outFragcolor = vec4(result,1);

}
