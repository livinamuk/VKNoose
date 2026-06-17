#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_KHR_vulkan_glsl : enable

#include "constants.glsl"

layout (location = 0) in vec2 texCoord;
layout (location = 1) in flat int textureIndex;
layout (location = 2) in flat int colorIndex;

layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];

void main() {
    outFragColor = texture(sampler2D(textures[textureIndex], samplers[0]), texCoord).rgba;
    
    vec3 color = (colorIndex == 0) ? vec3(1,1,1) : vec3(0.2, 1, 0.2);

    float val = (outFragColor.r + outFragColor.g + outFragColor.b) / 3;
    if (val < 0.5) {
        //FragColor.a = 0.75;
        outFragColor.rgb += vec3(0.05);
    }
    outFragColor.rgb *= color;

}