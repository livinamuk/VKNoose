// fragment shader
#version 460 core
#extension GL_KHR_vulkan_glsl : enable
#include "raycommon.glsl"

layout (location = 0) out vec4 outFragColor;

layout (location = 0) in vec2 texCoords;


layout(set = 2, binding = 0) uniform sampler2D rt_texture;
layout(set = 2, binding = 3) uniform sampler2D rt_noisey_indirect_texture;/*
layout(set = 2, binding = 1) uniform sampler2D basecolor_texture;
layout(set = 2, binding = 2) uniform sampler2D normal_texture; // currently you have the RT generated normal texture here
layout(set = 2, binding = 3) uniform sampler2D rt_depth_texture;
layout(set = 2, binding = 4) uniform sampler2D depth_texture;
layout(set = 2, binding = 5) uniform sampler2D rt_inventory_texture;
*/
layout(set = 3, binding = 0) uniform sampler2D denoiseA;
layout(set = 4, binding = 0) uniform sampler2D denoiseB;



void main() 
{
    outFragColor.a = 1;
    outFragColor.rgb = texture(denoiseB,vec2(texCoords.x, 1-texCoords.y)).xyz;
    //outFragColor.g = 0;
    //outFragColor.b = 0;
    
    
    vec3 firstBounce = texture(rt_texture,vec2(texCoords.x, 1-texCoords.y)).xyz;
    outFragColor.rgb += firstBounce;
    
    outFragColor.rgb += firstBounce;
    
    vec3 noiseyIndirect = texture(rt_noisey_indirect_texture,vec2(texCoords.x, 1-texCoords.y)).xyz;
    //outFragColor.rgb = firstBounce + noiseyIndirect;

    

    vec3 color = pow(outFragColor.rgb, vec3(1.0/2.2)); 
    vec3 tonemapped = Tonemap_ACES(color.xyz);
    color = mix(color, tonemapped, 0.1);
    color = tonemapped;

    // colorContrasted 
    float contrast = 1.125;
    float brightness = -0.060;
    
   // contrast = 1.25;
   // brightness = -0.160;
    color = (color) * contrast;
    color = color + vec3(brightness,brightness,brightness);

    float temperature = 225; // mix(1000.0, 15000.0, (sin(iTime * (PI2 / 10.0)) * 0.5) + 0.5);
    float temperatureStrength = 1.75;
    color = mix(color, color * colorTemperatureToRGB(temperature), temperatureStrength); 
    outFragColor.rgb = color.rgb;

    outFragColor.rgb = tonemapped.rgb;
    outFragColor.g = 0;
    outFragColor.b = 0;

    

}
