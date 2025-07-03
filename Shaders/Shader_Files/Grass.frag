#version 460

layout(location = 0) out vec4 outFragPosition;
layout(location = 1) out vec4 outFragNormal;
layout(location = 2) out vec4 outAlbedo;
layout(location = 3) out vec4 outMaterials;

layout(location = 0) in vec4 OutPosition;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in float OutCurrentHeight;

void main()
{
    vec3 baseColor = vec3(0.013, 0.545, 0.013);     // healthy green
    vec3 tipColor  = vec3(0.70, 0.584, 0.0);        // dry grass

    vec3 color;

    if (OutCurrentHeight > 0.2) {
        color = mix(tipColor, baseColor, 0.8);
    } else {
        color = mix(baseColor, tipColor, 0.8);
    }

    outAlbedo = vec4(color, 1.0);

    // Output world or view-space position as-is (usually vec4)
    outFragPosition = OutPosition;

    // Normalize the input normal to be safe
    vec3 normal = normalize(InNormal);

    outFragNormal = vec4(normal, 1.0);

    outMaterials = vec4(0.0, 0.0, 0.0, 1.0); 
}
