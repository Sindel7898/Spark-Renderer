#version 460
#extension GL_EXT_ray_tracing : require


struct UnifiedPayload {
    int  rayType; 
    vec3 data;    
    vec3 Emmisive;   
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

    }

}