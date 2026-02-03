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

vec3 DirectionalSoftShadow(vec3 LightDirection, float radius, float rand1, float rand2) {
    // L is the normalized direction TO the light source
    float cosAngle = cos(radius);

    // Sample local cone around Z-axis
    float z = rand1 * (1.0 - cosAngle) + cosAngle;
    float phi = rand2 * 2.0 * PI;
    
    float x = sqrt(1.0f - z * z) * cos(phi);
    float y = sqrt(1.0f - z * z) * sin(phi);

    vec3 north = vec3(0.0, 0.0, 1.0);
    float d = dot(north, LightDirection);

    vec3 axis = normalize(cross(north, LightDirection));
    float angle = acos(clamp(d, -1.0, 1.0));
    mat3 R = AngleAxis3x3(angle, axis);

    return R * vec3(x, y, z);
}


vec3 SoftShadow(vec3 LightPosition, vec3 WorldPosition,float radius,float rand1,float rand2)     {

   // Calculate a vector perpendicular to L
   vec3 toLight = normalize(LightPosition - WorldPosition);

   vec3 perpL = cross(toLight, vec3(0.f, 1.0f, 0.f));

   // Handle case where L = up -> perpL should then be (1,0,0)
   if (perpL.x == 0.0f || perpL.y == 0.0f || perpL.z == 0.0f ) {
       perpL.x = 1.0;
     }

   vec3 toLightEdge = normalize((LightPosition + perpL * radius) - WorldPosition);
   float coneAngle = acos(dot(toLight, toLightEdge)) * 2.0f;

   float cosAngle = cos(coneAngle);

    // Generate points on the spherical cap around the north pole [1].
    // [1] See https://math.stackexchange.com/a/205589/81266
    float z   = rand1 * (1.0f - cosAngle) + cosAngle;
    float phi = rand2 * 2.0f * PI;

    float x = sqrt(1.0f - z * z) * cos(phi);
    float y = sqrt(1.0f - z * z) * sin(phi);
    vec3 north = vec3(0.f, 0.f, 1.f);

    // Find the rotation axis `u` and rotation angle `rot` [1]
    vec3 axis = normalize(cross(north, normalize(toLight)));
    float angle = acos(dot(normalize(toLight), north));

    // Convert rotation axis and angle to 3x3 rotation matrix [2]
    mat3x3 R = AngleAxis3x3(angle, axis);

    return R * vec3(x, y, z);
}