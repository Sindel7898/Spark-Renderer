#version 450
    
layout(location = 0) in vec4 BaseColor;
layout(location = 0) out vec4 outColor;

void main() {

    outColor = BaseColor;
}