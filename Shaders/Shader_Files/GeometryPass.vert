#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;

layout(set = 0, binding = 0) uniform VertexUniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 prev_view;
    mat4 prev_proj;
} vuob;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 prevModel;
} pc;


layout(location = 0) out vec4 outWorldPos;
layout(location = 1) out vec4 outViewPos;
layout(location = 2) out vec2 outTexCoord;

layout(location = 3) out mat3 outWorldTBN;
layout(location = 6) out mat3 outViewTBN;

layout(location = 9)  out vec4 outCurrClipPos;
layout(location = 10) out vec4 outPrevClipPos;

void main()
{

    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    vec4 viewPos  = vuob.view * worldPos;
    vec4 clipPos  = vuob.proj * viewPos;

    gl_Position   = clipPos;

    outWorldPos = worldPos;
    outViewPos  = viewPos;
    outTexCoord = inTexCoord;

    outCurrClipPos = clipPos;


    vec4 prevWorldPos = pc.model * vec4(inPosition, 1.0);
    vec4 prevViewPos  = vuob.prev_view * prevWorldPos;
    vec4 prevClipPos  = vuob.prev_proj * prevViewPos;

    outPrevClipPos = prevClipPos;


    mat3 normalMatrix = transpose(inverse(mat3(pc.model)));

    vec3 T = normalize(normalMatrix * inTangent);
    vec3 N = normalize(normalMatrix * inNormal);
    vec3 B = cross(N, T);

    outWorldTBN = mat3(T, B, N);


    mat3 viewNormalMatrix = mat3(vuob.view);

    vec3 vT = normalize(viewNormalMatrix * T);
    vec3 vN = normalize(viewNormalMatrix * N);
    vec3 vB = cross(vN, vT);

    outViewTBN = mat3(vT, vB, vN);
}
