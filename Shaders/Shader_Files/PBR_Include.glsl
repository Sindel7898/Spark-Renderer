const float PI = 3.14159265359;


vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float safeRoughness = max(roughness, 0.05);
    float a      = safeRoughness*safeRoughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

 mat3 AngleAxis3x3(float angle, vec3 axis) {
    float s = sin(angle);
    float c = cos(angle);
    float t = 1.0 - c;

    return mat3(
        vec3(t * axis.x * axis.x + c,           t * axis.x * axis.y + s * axis.z, t * axis.x * axis.z - s * axis.y),
        vec3(t * axis.x * axis.y - s * axis.z, t * axis.y * axis.y + c,           t * axis.y * axis.z + s * axis.x),
        vec3(t * axis.x * axis.z + s * axis.y, t * axis.y * axis.z - s * axis.x, t * axis.z * axis.z + c)
    );
}

vec3 DirectionalSoftShadow(vec3 LightDir, float angleDegrees, float r1, float r2) {
    // Convert degrees to radians
    float cosAngle = cos(radians(angleDegrees));
    
    float z = r1 * (1.0 - cosAngle) + cosAngle;
    float phi = r2 * 2.0 * 3.14159265359;
    
    float sinTheta = sqrt(1.0 - z * z);
    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);

    // Safely build an orthonormal basis to avoid NaN singularities
    vec3 up = abs(LightDir.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, LightDir));
    vec3 bitangent = cross(LightDir, tangent);

    // Transform local sampled cone vector to world space
    return tangent * x + bitangent * y + LightDir * z;
}


vec3 SoftShadow(vec3 LightPos, vec3 WorldPos, float radius, float r1, float r2) {
    vec3 toLight = LightPos - WorldPos;
    float distToLight = length(toLight);
    vec3 L = toLight / distToLight;

    float sinAngle = clamp(radius / distToLight, 0.0, 1.0);
    float cosAngle = sqrt(1.0 - sinAngle * sinAngle);

    float z = r1 * (1.0 - cosAngle) + cosAngle;
    float phi = r2 * 2.0 * 3.14159265359;

    float sinTheta = sqrt(1.0 - z * z);
    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);

    vec3 up = abs(L.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, L));
    vec3 bitangent = cross(L, tangent);

    return tangent * x + bitangent * y + L * z;
}