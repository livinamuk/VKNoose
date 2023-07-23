#version 460
#extension GL_EXT_ray_tracing : enable

#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;

void main()
{
    rayPayload.color = vec3(0, 0, 0);
	rayPayload.done = 1;
	//rayPayload.instanceIndex = 8;
	//rayPayload.primitiveIndex = 9;
}