#version 460
#extension GL_EXT_scalar_block_layout : enable
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vTangent;

layout (location = 0) out vec2 texCoords;

void main() {
	texCoords = vTexCoord;
	gl_Position = vec4(vPosition, 1.0);
}