//glsl version 4.5
#version 450
#extension GL_ARB_separate_shader_objects : enable

//shader input
layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 worldPos;
layout (location = 3) in vec3 camPos;
//output write
layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 1) uniform  SceneData{   
    vec4 fogColor; // w is for exponent
	vec4 fogDistances; //x for min, y for max, zw unused.
	vec4 ambientColor;
	vec4 sunlightDirection; //w for sun power
	vec4 sunlightColor;
} sceneData;


layout(set = 2, binding = 0) uniform sampler2D tex1;


//layout(set = 0, binding = 1) uniform sampler samp;
//layout(set = 0, binding = 2) uniform texture2D textures[78];

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
        doomFactor = clamp(doomFactor, 0.23, 1.0);
    }
    return doomFactor;
}

void main() 
{
	vec3 color = texture(tex1,texCoord).xyz;

	 // Darken fragments further from the camera    

/*
    float v = 0.025;
    outFragColor.r = mix(outFragColor.r * outFragColor.r,  outFragColor.r, 0.25);
    outFragColor.g *= v;
    outFragColor.b *= v;

    outFragColor.rgb = color * doomFactor;  */

    vec3 fogColor = vec3(0.5);
    float fogDensity = 0.75;
    float fogDistance = (gl_FragCoord.z / gl_FragCoord.w) * 0.12;
    
    float v = 0.0125;
    v = 0.25;
   
   color.r = mix(color.r * color.r,  color.r, 0.25);
    color.g *= v;
    color.b *= v;

    vec3 FOG = calculateFog(color, fogColor, fogDensity, fogDistance);

    outFragColor.rgb = FOG;
    
    float doomfactor = calculateDoomFactor(worldPos, camPos.xyz, 1.0);
   outFragColor.rgb = outFragColor.rgb * doomfactor * doomfactor * doomfactor; 

    outFragColor.a =  texture(tex1,texCoord).a;
    if (outFragColor.a < 0.5)
        discard;


}