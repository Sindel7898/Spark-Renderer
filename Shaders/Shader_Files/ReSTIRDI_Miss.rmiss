#version 460
#extension GL_EXT_ray_tracing : require


struct UnifiedPayload {
    int  rayType; 
    vec3 data;    
    vec3 Emmisive;   
    vec3 Position;
    vec3 Normal;
};

layout(location = 0) rayPayloadInEXT UnifiedPayload payload;

void main() {

   if (payload.rayType == 0)
    {
        
    }
    else
    {
        payload.data = vec3(0.0);
        payload.Emmisive = vec3(0.0);
        payload.Position = vec3(0.0);
        payload.Normal = vec3(0.0);

    }

}