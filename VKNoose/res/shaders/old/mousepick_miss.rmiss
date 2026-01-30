#version 460
#extension GL_EXT_ray_tracing : enable
#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT MousepickPayload mousepickPayload;

void main() {
	mousepickPayload.instanceIndex = 0;
	mousepickPayload.primitiveIndex = 0;
}