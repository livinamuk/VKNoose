#version 460

#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "push_constants.glsl"
#include "types.glsl"

layout(location = 0) out vec2 texCoord;
layout(location = 1) out flat int textureIndex;
layout(location = 2) out flat int colorIndex;

layout(push_constant, scalar) uniform PushConstants { 
	UIPushConstant data; 
} pushConstant;

layout(buffer_reference, scalar) readonly buffer ObjectData2DBuffer { ObjectData2D data[]; };

const vec2 positions[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0,  1.0),

    vec2(-1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2( 1.0, -1.0) 
);

const vec2 texCoords[6] = vec2[](
    vec2(0.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),

    vec2(0.0, 1.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0) 
);

void main() {
    ObjectData2DBuffer objects = ObjectData2DBuffer(pushConstant.data.instancesDeviceAddress);
    ObjectData2D object = objects.data[gl_InstanceIndex];

    textureIndex = object.index_basecolor;
    colorIndex = object.index_color;

    vec2 position = positions[gl_VertexIndex];

    gl_Position = object.model * vec4(position, 0.0, 1.0);
    texCoord = texCoords[gl_VertexIndex];
}