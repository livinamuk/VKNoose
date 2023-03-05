//glsl version 4.5
#version 450

//shader input
layout (location = 0) in vec3 inColor;
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



float fog_exp2(
  const float dist,
  const float density
) {
  const float LOG2 = -1.442695;
  float d = density * dist;
  return 1.0 - clamp(exp2(d * d * LOG2), 0.0, 1.0);
}


void main() 
{
	vec3 color = texture(tex1,texCoord).xyz;
    
	 // Darken fragments further from the camera    
    float distanceFromCamera = distance(worldPos, camPos.xyz);
    float doomFactor = 1;
    float falloff = 2.0;//3.0;		
    if (distanceFromCamera > falloff) {
        distanceFromCamera -= falloff;
        distanceFromCamera *= 0.125;
        doomFactor = (1 - distanceFromCamera);
        doomFactor = clamp(doomFactor, 0.23, 1.0);
    }
   
   
  float FOG_DENSITY = 0.75;
 // FOG_DENSITY = 1;
  float fogDistance = gl_FragCoord.z / gl_FragCoord.w;
  fogDistance *= 0.125;
  float fogAmount = fog_exp2(fogDistance, FOG_DENSITY);
  vec3 fogColor = vec3(0.0);
  fogColor = vec3(0, 0, 0);
  vec3 fogResult = mix(color, fogColor, fogAmount);

   color *= doomFactor;
   // color = vec3(worldPos);

	outFragColor = vec4(color,1.0f);

    
	outFragColor = vec4(fogResult, texture(tex1,texCoord).a);

    if (outFragColor.a < 0.5)
        discard;
	//outFragColor = vec4(fogResult,1.0);
  //  outFragColor.r = 0;
}