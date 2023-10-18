 // fragment shader
#version 460 core
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) out vec4 outFragColor;
layout (location = 0) in vec2 texCoords;

layout(set = 0, binding = 0) uniform sampler2D firstHitColor;
layout(set = 0, binding = 1) uniform sampler2D firstHitNormals;
layout(set = 0, binding = 2) uniform sampler2D firstHitBaseColor;
layout(set = 0, binding = 3) uniform sampler2D secondHitColor;
layout(set = 0, binding = 4) uniform sampler2D denoiseA;
layout(set = 0, binding = 5) uniform sampler2D denoiseB;
layout(set = 0, binding = 6) uniform sampler2D denoiseC;
layout(set = 0, binding = 7) uniform sampler2D laptopDisplay;

void main() {
    vec3 inputColor = texture(denoiseC, texCoords).rgb;
    outFragColor = vec4(0, inputColor.b, inputColor.b, 1);
}
