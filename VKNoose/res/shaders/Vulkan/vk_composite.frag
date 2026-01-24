#version 460 core
#extension GL_KHR_vulkan_glsl : enable
#include "vk_raycommon.glsl"

layout (location = 0) out vec4 outFragColor;
layout (location = 0) in vec2 texCoords;

layout(set = 2, binding = 0) uniform sampler2D firstHitColorTexture;
layout(set = 2, binding = 1) uniform sampler2D firstHitNormalsTexture;
layout(set = 2, binding = 2) uniform sampler2D firstHitBaseColorTexture;
layout(set = 2, binding = 3) uniform sampler2D secondHitColorTexture;
layout(set = 2, binding = 4) uniform sampler2D denoiseATexture;
layout(set = 2, binding = 5) uniform sampler2D denoiseBTexture;
layout(set = 2, binding = 6) uniform sampler2D denoiseCTexture;
layout(set = 2, binding = 7) uniform sampler2D laptopDisplay;


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
    vec3 firstHitColor = texture(firstHitColorTexture,vec2(texCoords.x, 1-texCoords.y)).xyz;    
    vec3 firstHitNormals = texture(firstHitNormalsTexture,vec2(texCoords.x, 1-texCoords.y)).xyz;
    vec3 secondHitColorTexture = texture(secondHitColorTexture,vec2(texCoords.x, 1-texCoords.y)).xyz;
    vec3 firstHitBaseColor = texture(firstHitBaseColorTexture,vec2(texCoords.x, 1-texCoords.y)).xyz;
    vec3 denoiseA = texture(denoiseATexture,vec2(texCoords.x, 1-texCoords.y)).xyz;
    vec3 denoiseB = texture(denoiseBTexture,vec2(texCoords.x, 1-texCoords.y)).xyz;
    vec3 denoiseC = texture(denoiseCTexture,vec2(texCoords.x, 1-texCoords.y)).xyz;

	vec3 finalColor = mix(firstHitColor, secondHitColorTexture * firstHitBaseColor, 0.8);;

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
    outFragColor.a = 1.0;
}
