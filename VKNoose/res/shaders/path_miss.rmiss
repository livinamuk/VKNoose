#version 460
#extension GL_EXT_ray_tracing : enable

#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;

void main() {
	rayPayload.done = 0;
    rayPayload.color = vec3(0);
	rayPayload.distance = 0;
	rayPayload.normal = vec3(0);
	rayPayload.nextRayOrigin = vec3(0);
	rayPayload.nextFactor = vec3(0);
	rayPayload.nextRayDirection = vec3(0);
	rayPayload.seed = rayPayload.seed;
	//rayPayload.writeToImageStore = 1;
}