 // fragment shader
#version 460 core
#extension GL_KHR_vulkan_glsl : enable
#include "raycommon.glsl"

layout (location = 0) out vec4 outFragColor;

layout (location = 0) in vec2 texCoords;

layout(set = 2, binding = 0) uniform sampler2D rt_texture;
layout(set = 2, binding = 1) uniform sampler2D basecolor_texture;
layout(set = 2, binding = 2) uniform sampler2D normal_texture; // currently you have the RT generated normal texture here
layout(set = 2, binding = 3) uniform sampler2D rt_depth_texture;
layout(set = 2, binding = 4) uniform sampler2D depth_texture;
layout(set = 2, binding = 5) uniform sampler2D rt_inventory_texture;
//layout(set = 0, binding = 1) uniform CameraData_ { CameraData data; } cam;

layout(set = 3, binding = 0) uniform sampler2D inputImage;


//layout(set = 1, binding = 0) uniform sampler samp;
//layout(set = 1, binding = 6, rgba8) uniform image2D noisyShadowTexture;
  
  #define INV_SQRT_OF_2PI 0.39894228040143267793994605993439  // 1.0/SQRT_OF_2PI
#define INV_PI 0.31830988618379067153776752674503

float linearize_depth(float d,float zNear,float zFar)
{
    return zNear * zFar / (zFar + d * (zNear - zFar));
}


void main() 
{
    outFragColor.a = 1;

    vec4 nomralTexture = texture(normal_texture,vec2(texCoords.x, 1-texCoords.y));
    vec3 normal = nomralTexture.rgb;
    float depth = nomralTexture.a;

    outFragColor.rgb = normal * depth;
    
    float viewportWidth = 512 * 2; 
    float viewportHeight = 288 * 2; 
    float pixelWidth = 1.0 / viewportWidth;
    float pixelHeight = 1.0 / viewportHeight;

    vec2 TexCoords = vec2(texCoords.x, 1-texCoords.y);
    vec3 fragmentColor = texture(rt_texture, TexCoords).rgb;
    vec3 fragmentNormal = texture(normal_texture, TexCoords).rgb * 2 - 1;
    float fragmentDepth = texture(normal_texture, TexCoords).a;
    vec3 fragmentPositon = texture(rt_depth_texture, TexCoords).rgb;
    float fragmentMeshIndex = texture(rt_depth_texture, TexCoords).a;
    vec3 firstBounceColor = texture(rt_depth_texture, TexCoords).rgb;

    float near_plane = 0.01;
    float far_plane = 25;
    //fragmentDepth = linearize_depth(fragmentDepth, near_plane, far_plane);

    vec3 result = vec3(0);
    int count = 0;
    int range = 2;

    for (int x = -range; x <= range; x++) {    
        for (int y = -range; y <= range; y++) {

            vec2 sampleCoords = vec2(texCoords.x, 1-texCoords.y);
            sampleCoords.x += pixelWidth * x;
            sampleCoords.y += pixelHeight * y;
                
            if (sampleCoords.x > 1 ||
                sampleCoords.y > 1 ||
                sampleCoords.x < 0 ||
                sampleCoords.y < 0)
                continue;
                    
            vec3 sampleColor = texture(inputImage, sampleCoords).rgb;
            vec3 sampleNormal = texture(normal_texture, sampleCoords).rgb * 2 - 1;
            float sampleDepth = texture(normal_texture, sampleCoords).a;
            vec3 samplePositon = texture(rt_depth_texture, sampleCoords).rgb;
            float sampleMeshIndex = texture(rt_depth_texture, sampleCoords).a;

            /*
            if (int(sampleMeshIndex) != int(fragmentMeshIndex)){//&& sampleNormal != fragmentNormal) {
                //result += vec3(1);
                continue;
            }
            //sampleDepth = linearize_depth(sampleDepth, near_plane, far_plane);

            // Position similiarty
            float positionDisplacement = distance(samplePositon, fragmentPositon);
            if (positionDisplacement > 0.05) {
                //result += vec3(1);
                //count++;
               // continue;
            }

            if (samplePositon.z > -1.01) {
           // result += vec3(positionDisplacement);
               //fcontinue;
            }

            //float depthDisplacement = abs(sampleDepth - fragmentDepth);


            float depthCutoff = 0.115 * fragmentDepth;
            depthCutoff = max(depthCutoff, 0.0045);
            float wz = 1.0;   
            if (abs(fragmentDepth - sampleDepth) > depthCutoff) {                                                   
                //result += vec3(1);
                continue;                                                                                 
            }     */


            // Normal similiarity
            float normalDisplacement = abs( dot(sampleNormal, fragmentNormal));
            if (normalDisplacement < 0.35) {
                //result += vec3(1);
            //    continue;
            }

            result += sampleColor;

            count++;
  
        }
    }
    outFragColor.rgb = result.rgb / count;
    
//	outFragColor.rgb = pow(outFragColor.rgb, vec3(1.0/2.2)); 
//    outFragColor = vec4(fragmentNormal, 1);
}
