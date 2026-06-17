#version 460 core
#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_scalar_block_layout : enable

#include "constants.glsl"

layout (location = 0) out vec4 outFragColor;
layout (location = 0) in vec2 texCoords;

// Static Descriptor set
layout(set = 0, binding = DESC_IDX_SAMPLERS)                uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURES)                uniform texture2D textures[];
layout(set = 0, binding = DESC_IDX_SSBOS)                   readonly buffer GlobalSSBO { uint data[]; } ssbos[];
layout(set = 0, binding = DESC_IDX_STORAGE_IMAGES_RGBA32F,  rgba32f) uniform image2D storage_images_rgba32f[];
layout(set = 0, binding = DESC_IDX_STORAGE_IMAGES_RGBA16F,  rgba16f) uniform image2D storage_images_rgba16f[];
layout(set = 0, binding = DESC_IDX_STORAGE_IMAGES_RGBA8,    rgba8)   uniform image2D storage_images_rgba8[];

vec3 Filmic(vec3 x) {
  vec3 X = max(vec3(0.0), x - 0.004);
  vec3 result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
  return pow(result, vec3(2.2));
}

float Filmic(float x) {
  float X = max(0.0, x - 0.004);
  float result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
  return pow(result, 2.2);
}

vec3 Tonemap_ACES(const vec3 x) {
    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

// Valid from 1000 to 40000 K (and additionally 0 for pure full white)
vec3 ColorTemperatureToRGB(const in float temperature){
  // Values from: http://blenderartists.org/forum/showthread.php?270332-OSL-Goodness&p=2268693&viewfull=1#post2268693   
  mat3 m = (temperature <= 6500.0) ? mat3(vec3(0.0, -2902.1955373783176, -8257.7997278925690),
	                                      vec3(0.0, 1669.5803561666639, 2575.2827530017594),
	                                      vec3(1.0, 1.3302673723350029, 1.8993753891711275)) : 
	 								 mat3(vec3(1745.0425298314172, 1216.6168361476490, -8257.7997278925690),
   	                                      vec3(-2666.3474220535695, -2173.1012343082230, 2575.2827530017594),
	                                      vec3(0.55995389139931482, 0.70381203140554553, 1.8993753891711275)); 
  return mix(clamp(vec3(m[0] / (vec3(clamp(temperature, 1000.0, 40000.0)) + m[1]) + m[2]), vec3(0.0), vec3(1.0)), vec3(1.0), smoothstep(1000.0, 0.0, temperature));
}

void main() {
	vec2 uv = vec2(texCoords.x, 1-texCoords.y);

	vec3 firstHitColor     = texture(sampler2D(textures[RT_IDX_FIRST_HIT_COLOR],   samplers[0]), uv).xyz;
	vec3 firstHitNormals   = texture(sampler2D(textures[RT_IDX_FIRST_HIT_NORMALS], samplers[0]), uv).xyz;
	vec3 firstHitBaseColor = texture(sampler2D(textures[RT_IDX_FIRST_HIT_BASE],    samplers[0]), uv).xyz;
	vec3 secondHitColor    = texture(sampler2D(textures[RT_IDX_SECOND_HIT_COLOR],  samplers[0]), uv).xyz;

	vec3 finalColor = mix(firstHitColor, secondHitColor * firstHitBaseColor, 0.8);;

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
	finalColor = mix(finalColor, finalColor * ColorTemperatureToRGB(temperature), temperatureStrength); 
	
	// Filmic tonemapping
	finalColor = mix(finalColor, Filmic(finalColor), 0.75);
		
	// Vignette
	/*float uvMagSqrd = dot(d,d);
	float amount = 0.125;
    float vignette = 1.0 - uvMagSqrd * amount;
    finalColor *= vignette;*/
    
    outFragColor.rgb = finalColor;
    outFragColor.a = 1.0;
}
