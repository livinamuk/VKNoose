#version 460 core
#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_scalar_block_layout : enable

#include "vk_raycommon.glsl"
#include "constants.glsl"

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

// Static Descriptor set
layout(set = 3, binding = DESC_IDX_SAMLPLERS)               uniform sampler g_samplers[];
layout(set = 3, binding = DESC_IDX_TEXTURES)                uniform texture2D g_textures[];
layout(set = 3, binding = DESC_IDX_SSBOS)                   readonly buffer GlobalSSBO { uint data[]; } g_ssbos[];
layout(set = 3, binding = DESC_IDX_STORAGE_IMAGES_RGBA32F,  rgba32f) uniform image2D g_storage_images_rgba32f[];
layout(set = 3, binding = DESC_IDX_STORAGE_IMAGES_RGBA16F,  rgba16f) uniform image2D g_storage_images_rgba16f[];
layout(set = 3, binding = DESC_IDX_STORAGE_IMAGES_RGBA8,    rgba8)   uniform image2D g_storage_images_rgba8[];


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

	vec2 uv = vec2(texCoords.x, 1-texCoords.y);

	//firstHitColor     = texture(sampler2D(g_textures[RT_IDX_FIRST_HIT_COLOR],   g_samplers[0]), uv).xyz;

	//vec3 firstHitColor     = texture(sampler2D(g_textures[RT_IDX_FIRST_HIT_COLOR],   g_samplers[0]), uv).xyz;
	//vec3 firstHitNormals   = texture(sampler2D(g_textures[RT_IDX_FIRST_HIT_NORMALS], g_samplers[0]), uv).xyz;
	//vec3 firstHitBaseColor = texture(sampler2D(g_textures[RT_IDX_FIRST_HIT_BASE],    g_samplers[0]), uv).xyz;
	//vec3 secondHitColor    = texture(sampler2D(g_textures[RT_IDX_SECOND_HIT_COLOR],  g_samplers[0]), uv).xyz;


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
