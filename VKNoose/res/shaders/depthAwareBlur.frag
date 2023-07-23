// fragment shader
#version 460 core
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) out vec4 outFragColor;

layout (location = 0) in vec2 texCoords;

layout(set = 2, binding = 0) uniform sampler2D rt_texture;
layout(set = 2, binding = 1) uniform sampler2D basecolor_texture;
layout(set = 2, binding = 2) uniform sampler2D normal_texture;
layout(set = 2, binding = 3) uniform sampler2D rma_texture;
layout(set = 2, binding = 4) uniform sampler2D depth_texture;
layout(set = 2, binding = 5) uniform sampler2D rt_inventory_texture;
//layout(set = 0, binding = 1) uniform CameraData_ { CameraData data; } cam;


//layout(set = 1, binding = 0) uniform sampler samp;
//layout(set = 1, binding = 6, rgba8) uniform image2D noisyShadowTexture;
  
  #define INV_SQRT_OF_2PI 0.39894228040143267793994605993439  // 1.0/SQRT_OF_2PI
#define INV_PI 0.31830988618379067153776752674503


  vec4 smartDeNoise(sampler2D tex, vec2 uv, float sigma, float kSigma, float threshold)
{
    float radius = round(kSigma*sigma);
    float radQ = radius * radius;

    float invSigmaQx2 = .5 / (sigma * sigma);      // 1.0 / (sigma^2 * 2.0)
    float invSigmaQx2PI = INV_PI * invSigmaQx2;    // 1/(2 * PI * sigma^2)

    float invThresholdSqx2 = .5 / (threshold * threshold);     // 1.0 / (sigma^2 * 2.0)
    float invThresholdSqrt2PI = INV_SQRT_OF_2PI / threshold;   // 1.0 / (sqrt(2*PI) * sigma^2)

    vec4 centrPx = texture(tex,uv); 

    float zBuff = 0.0;
    vec4 aBuff = vec4(0.0);
    vec2 size = vec2(textureSize(tex, 0));

    vec2 d;
    for (d.x=-radius; d.x <= radius; d.x++) {
        float pt = sqrt(radQ-d.x*d.x);       // pt = yRadius: have circular trend
        for (d.y=-pt; d.y <= pt; d.y++) {
            float blurFactor = exp( -dot(d , d) * invSigmaQx2 ) * invSigmaQx2PI;

            vec4 walkPx =  texture(tex,uv+d/size);
            vec4 dC = walkPx-centrPx;
            float deltaFactor = exp( -dot(dC, dC) * invThresholdSqx2) * invThresholdSqrt2PI * blurFactor;

            zBuff += deltaFactor;
            aBuff += deltaFactor*walkPx;
        }
    }
    return aBuff/zBuff;
}

float calculateDoomFactor(vec3 fragPos, vec3 camPos, float fallOff)
{
    float distanceFromCamera = distance(fragPos, camPos);
    float doomFactor = 1;	
    if (distanceFromCamera > fallOff) {
        distanceFromCamera -= fallOff;
        distanceFromCamera *= 0.13;
        doomFactor = (1 - distanceFromCamera);
       // doomFactor = clamp(doomFactor, 0.23, 1.0);
        doomFactor = clamp(doomFactor, 0.1, 1.0);
    }
    return doomFactor;
}


void main() 
{


    //vec3 raytraced = texture(rma_texture,vec2(texCoords.x, 1-texCoords.y)).xyz;
    vec3 raytraced = texture(basecolor_texture,vec2(texCoords.x, 1-texCoords.y)).xyz;
    outFragColor.a = 1;
    
    // gamma correct
  // raytraced = pow(raytraced, vec3(1.0/2.2)); 
 //  raytraced = Tonemap_ACES(raytraced);

    outFragColor.rgb = raytraced;

	float radius = 0.125;
    float sigma = 10;
    float kSigma = radius / radius;
    float threshold = 0.5;
    
    vec2 uv = vec2(texCoords.x, 1-texCoords.y);
 //   outFragColor.rgb = smartDeNoise(rt_texture, uv, sigma, kSigma, threshold).rgb;
}

void main2()
{    
    vec2 uv = vec2(texCoords.x, 1-texCoords.y);
	//vec3 test = texture(tex1,vec2(texCoords.x, 1-texCoords.y)).xyz;
/*
    float radius = 0.125;
    float sigma = 20;
    float kSigma = radius / radius;
    float threshold = 10.83;

    vec4 test = vec4(0);//smartDeNoise(tex1, uv, sigma, kSigma, threshold);
    outFragColor.rgb = vec3(test.r, 0, 0);
    outFragColor.a = 1;
    
	vec3 noiseyShadow = texture(tex1,vec2(texCoords.x, 1-texCoords.y)).xyz;
    
	vec3 test2 = texture(basecolor_texture,vec2(texCoords.x, 1-texCoords.y)).xyz;
    outFragColor.rgb = vec3(test2 * test.rgb + (test2 * 0.0));
    outFragColor.rgb *= 0.7;
    
    outFragColor.rgb = test2.rgb * noiseyShadow;
    outFragColor.g = 0;
    outFragColor.b = 0;
    outFragColor.a = 1;*/
    
    vec3 raytraced = texture(rt_texture,vec2(texCoords.x, 1-texCoords.y)).xyz;
    vec3 normal = texture(normal_texture,vec2(texCoords.x, 1-texCoords.y)).xyz;
    
    outFragColor.rgb = texture(rma_texture,vec2(texCoords.x, 1-texCoords.y)).xyz;
    outFragColor.a = 1;

    outFragColor.rgb = normal;










        vec2 offset[25];
    offset[0] = vec2(-2,-2);
    offset[1] = vec2(-1,-2);
    offset[2] = vec2(0,-2);
    offset[3] = vec2(1,-2);
    offset[4] = vec2(2,-2);
    
    offset[5] = vec2(-2,-1);
    offset[6] = vec2(-1,-1);
    offset[7] = vec2(0,-1);
    offset[8] = vec2(1,-1);
    offset[9] = vec2(2,-1);
    
    offset[10] = vec2(-2,0);
    offset[11] = vec2(-1,0);
    offset[12] = vec2(0,0);
    offset[13] = vec2(1,0);
    offset[14] = vec2(2,0);
    
    offset[15] = vec2(-2,1);
    offset[16] = vec2(-1,1);
    offset[17] = vec2(0,1);
    offset[18] = vec2(1,1);
    offset[19] = vec2(2,1);
    
    offset[20] = vec2(-2,2);
    offset[21] = vec2(-1,2);
    offset[22] = vec2(0,2);
    offset[23] = vec2(1,2);
    offset[24] = vec2(2,2);
    
    
    float kernel[25];
    kernel[0] = 1.0f/256.0f;
    kernel[1] = 1.0f/64.0f;
    kernel[2] = 3.0f/128.0f;
    kernel[3] = 1.0f/64.0f;
    kernel[4] = 1.0f/256.0f;
    
    kernel[5] = 1.0f/64.0f;
    kernel[6] = 1.0f/16.0f;
    kernel[7] = 3.0f/32.0f;
    kernel[8] = 1.0f/16.0f;
    kernel[9] = 1.0f/64.0f;
    
    kernel[10] = 3.0f/128.0f;
    kernel[11] = 3.0f/32.0f;
    kernel[12] = 9.0f/64.0f;
    kernel[13] = 3.0f/32.0f;
    kernel[14] = 3.0f/128.0f;
    
    kernel[15] = 1.0f/64.0f;
    kernel[16] = 1.0f/16.0f;
    kernel[17] = 3.0f/32.0f;
    kernel[18] = 1.0f/16.0f;
    kernel[19] = 1.0f/64.0f;
    
    kernel[20] = 1.0f/256.0f;
    kernel[21] = 1.0f/64.0f;
    kernel[22] = 3.0f/128.0f;
    kernel[23] = 1.0f/64.0f;
    kernel[24] = 1.0f/256.0f;
    
    vec4 sum = vec4(0.0);
    float c_phi = 1.0;
    float n_phi = 0.5;
    //float p_phi = 0.3;
	vec4 cval = texture(rt_texture,vec2(texCoords.x, 1-texCoords.y));
	vec4 nval = texture(normal_texture,vec2(texCoords.x, 1-texCoords.y));
	//vec4 pval = texelFetch(iChannel2, ivec2(fragCoord), 0);
    
    float denoiseStrength = 3;

    float cum_w = 0.0;
    for(int i=0; i<25; i++)
    {
        vec2 uv = vec2(uv.x, 1-uv.y)  + (offset[i] / vec2(512, 288)) * denoiseStrength;
        
        vec4 ctmp = texture(rt_texture,vec2(uv.x, 1-uv.y));
        vec4 t = cval - ctmp;
        float dist2 = dot(t,t);
        float c_w = min(exp(-(dist2)/c_phi), 1.0);
        
        vec4 ntmp = texture(normal_texture,vec2(uv.x, 1-uv.y));
        t = nval - ntmp;
        dist2 = max(dot(t,t), 0.0);
        float n_w = min(exp(-(dist2)/n_phi), 1.0);
        
        //vec4 ptmp = texelFetch(iChannel2, ivec2(uv), 0);
        //t = pval - ptmp;
        //dist2 = dot(t,t);
        //float p_w = min(exp(-(dist2)/p_phi), 1.0);
        
        //float weight = c_w*n_w*p_w;
        float weight = c_w*n_w;
        sum += ctmp*weight*kernel[i];
        cum_w += weight*kernel[i];
    }
 
        outFragColor = sum/cum_w;
   
        //outFragColor = cval;
        //outFragColor.rgb = raytraced;
        outFragColor.a = 1;
} 