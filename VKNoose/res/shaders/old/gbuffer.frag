#version 450
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) out vec4 outBaseColor;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outRMA;

layout(set = 1, binding = 0) uniform sampler samp;
layout(set = 1, binding = 1) uniform texture2D textures[91];

layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 texCoords;
layout (location = 2) in flat int basecolorIndex;
layout (location = 3) in flat int normalIndex;
layout (location = 4) in flat int rmaIndex;

void main() {
    
    outBaseColor = texture(sampler2D(textures[basecolorIndex], samp), texCoords).rgba;
    outNormal = texture(sampler2D(textures[normalIndex], samp), texCoords).rgba;
    outRMA = texture(sampler2D(textures[rmaIndex], samp), texCoords).rgba;
    
    outNormal.a = 1;
    outRMA.a = 1;

    outNormal.rgb = normal;

    if (outBaseColor.a < 0.75)
        discard;


  //  outFragColor.a = 1;
   // outFragColor = vec4(texCoords, 0, 1);
}