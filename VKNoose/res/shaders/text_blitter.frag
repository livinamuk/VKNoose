#version 450

layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in flat int textureIndex;

layout (location = 0) out vec4 outFragColor;

layout(set = 1, binding = 0) uniform sampler samp;
layout(set = 1, binding = 1) uniform texture2D textures[91];

void main() {
    outFragColor = texture(sampler2D(textures[textureIndex], samp), texCoord).rgba;
}