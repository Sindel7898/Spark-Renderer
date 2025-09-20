#version 450

layout (binding = 0) uniform sampler2D samplerta_SSGI;
layout (binding = 1) uniform sampler2D samplerViewSpacePosition;

layout (location = 0) in vec2 inTexCoord;           
layout (location = 0) out vec4 outFragcolor;

// pass blur direction from outside: (1,0) for horizontal, (0,1) for vertical
layout (push_constant) uniform PushConsts {
    int Direction;
} pc;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(samplerta_SSGI, 0));

    vec3 color  = texture(samplerta_SSGI,inTexCoord).rgb;
    float Depth  = texture(samplerViewSpacePosition,inTexCoord).b;

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
        if(Difference < 1.2){
            result += texture(samplerta_SSGI, inTexCoord + offset).rgb * w; 
            totalWeight += w;
        }
    }

    if(totalWeight > 0.0) {
       result /= totalWeight;
     }

    outFragcolor = vec4(result,1);

}
