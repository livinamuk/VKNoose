#version 460
#extension GL_EXT_ray_tracing : require
#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT MousepickPayload mousepickPayload;

void main() {
	mousepickPayload.instanceIndex = gl_InstanceCustomIndexEXT;
	mousepickPayload.primitiveIndex = gl_PrimitiveID;
}
