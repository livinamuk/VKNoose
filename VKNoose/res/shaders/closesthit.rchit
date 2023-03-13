#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) rayPayloadInEXT vec3 hitValue;

layout(binding = 2, set = 0) uniform CameraProperties 
{
	mat4 viewInverse;
	mat4 projInverse;
	vec4 viewPos;
} cam;

hitAttributeEXT vec2 attribs;

float fog_exp2(const float dist, const float density) {
  const float LOG2 = -1.442695;
  float d = density * dist;
  return 1.0 - clamp(exp2(d * d * LOG2), 0.0, 1.0);
}

vec3 calculateFog(vec3 inputColor, vec3 fogColor, float fogDensity, float fogDistance) {
    float fogAmount = fog_exp2(fogDistance, fogDensity);
    return mix(inputColor, fogColor, fogAmount);
}

float calculateDoomFactor(vec3 fragPos, vec3 camPos, float fallOff)
{
    float distanceFromCamera = distance(fragPos, camPos);
    float doomFactor = 1;	
    if (distanceFromCamera > fallOff) {
        distanceFromCamera -= fallOff;
        distanceFromCamera *= 0.125;
        doomFactor = (1 - distanceFromCamera);
       // doomFactor = clamp(doomFactor, 0.23, 1.0);
        doomFactor = clamp(doomFactor, 0.1, 1.0);
    }
    return doomFactor;
}

void main()
{
    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    hitValue = barycentricCoords;

    vec3 hitPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;


    vec3 camPos = cam.viewPos.rgb;

    float doom = calculateDoomFactor(hitPos, camPos, 1.0);

    vec3 fogColor = vec3(0.5);
    float fogDensity = 0.75;
    float fogDistance = distance(hitPos, camPos) * 0.12;
    

    vec3 FOG = calculateFog(barycentricCoords, fogColor, fogDensity, fogDistance);
    
    hitValue = FOG;// * doom;
  //  hitValue = vec3(1, 1,1) * doom;
}
