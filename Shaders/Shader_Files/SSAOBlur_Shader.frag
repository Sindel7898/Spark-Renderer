#version 450

layout (binding = 0) uniform sampler2D samplerSSAO;

layout(location = 0) in vec2 inTexCoord;           

layout (location = 0) out vec4 outFragcolor;


void main() {
   
    vec2 texelSize  = 1.0/vec2(textureSize(samplerSSAO,0));
    
    float result = 0.0;

    for(int x = -2; x < 2; ++x){
        for(int y = -2; y < 2; ++y){

                    vec2 offset = vec2(float(x), float(y)) * texelSize;
                     result += textureLod(samplerSSAO, inTexCoord + offset,0).r;

        }
    }

    outFragcolor =  vec4(result / (4.0 * 4.0),result / (4.0 * 4.0),result / (4.0 * 4.0),1.0f);
}