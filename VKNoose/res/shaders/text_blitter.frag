#version 450
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in flat int textureIndex;
layout (location = 3) in flat int colorIndex;
layout (location = 4) in flat int xClipMin;
layout (location = 5) in flat int xClipMax;
layout (location = 6) in flat int yClipMin;
layout (location = 7) in flat int yClipMax;

layout (location = 0) out vec4 outFragColor;

layout(set = 1, binding = 0) uniform sampler samp;
layout(set = 1, binding = 1) uniform texture2D textures[91];

void main() {
    outFragColor = texture(sampler2D(textures[textureIndex], samp), texCoord).rgba;
    vec3 color = (colorIndex == 0) ? vec3(1,1,1) : vec3(0.2, 1, 0.2);

    float val = (outFragColor.r + outFragColor.g + outFragColor.b) / 3;
    if (val < 0.5) {
        //FragColor.a = 0.75;
        outFragColor.rgb += vec3(0.05);
    }
    outFragColor.rgb *= color;

    
    if (gl_FragCoord.x < xClipMin)
        outFragColor.a = 0;    
    if (gl_FragCoord.x > xClipMax)
        outFragColor.a = 0;
    if (gl_FragCoord.y > 430 - yClipMin)
        outFragColor.a = 0;
    if (gl_FragCoord.y < 430 - yClipMax)
        outFragColor.a = 0;
}