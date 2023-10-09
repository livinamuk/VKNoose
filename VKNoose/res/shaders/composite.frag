// fragment shader
#version 460 core
#extension GL_KHR_vulkan_glsl : enable
#include "raycommon.glsl"

layout (location = 0) out vec4 outFragColor;

layout (location = 0) in vec2 texCoords;


layout(set = 2, binding = 0) uniform sampler2D first_bounce_texture;
layout(set = 2, binding = 2) uniform sampler2D normal_texture;
layout(set = 2, binding = 3) uniform sampler2D second_bounce_texture;/*
layout(set = 2, binding = 1) uniform sampler2D basecolor_texture;
layout(set = 2, binding = 2) uniform sampler2D normal_texture; // currently you have the RT generated normal texture here
layout(set = 2, binding = 3) uniform sampler2D rt_depth_texture;
layout(set = 2, binding = 4) uniform sampler2D depth_texture;
layout(set = 2, binding = 5) uniform sampler2D rt_inventory_texture;
*/
layout(set = 3, binding = 0) uniform sampler2D denoiseA;
layout(set = 4, binding = 0) uniform sampler2D denoiseB;


vec3 filmic(vec3 x) {
  vec3 X = max(vec3(0.0), x - 0.004);
  vec3 result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
  return pow(result, vec3(2.2));
}

float filmic(float x) {
  float X = max(0.0, x - 0.004);
  float result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
  return pow(result, 2.2);
}


void main() 
{
    vec3 firstBounce = texture(first_bounce_texture,vec2(texCoords.x, 1-texCoords.y)).xyz;    
    vec3 secondBounce = texture(second_bounce_texture,vec2(texCoords.x, 1-texCoords.y)).xyz;
    vec3 denoised = texture(denoiseA,vec2(texCoords.x, 1-texCoords.y)).xyz;

	vec3 finalColor = firstBounce + secondBounce;
	finalColor = mix(firstBounce, secondBounce, 0.8);
	
	//finalColor = secondBounce;

    // Tonemap
	finalColor = pow(finalColor, vec3(1.0/2.2)); 
	finalColor = Tonemap_ACES(finalColor);
		
	// Brightness and contrast
	float contrast = 1.25;
	float brightness = -0.08;
	finalColor = finalColor * contrast;
	finalColor = finalColor + vec3(brightness);
	
	// Temperature
	float temperature = 225; // mix(1000.0, 15000.0, (sin(iTime * (PI2 / 10.0)) * 0.5) + 0.5);
	float temperatureStrength = 1.75;
	finalColor = mix(finalColor, finalColor * colorTemperatureToRGB(temperature), temperatureStrength); 
	
	// Filmic tonemapping
	finalColor = mix(finalColor, filmic(finalColor), 0.75);
		
	// Vignette
	/*float uvMagSqrd = dot(d,d);
	float amount = 0.125;
    float vignette = 1.0 - uvMagSqrd * amount;
    finalColor *= vignette;*/
    
    outFragColor.rgb = finalColor;
 //   outFragColor.rgb = denoised;
    outFragColor.a = 1.0;
}
