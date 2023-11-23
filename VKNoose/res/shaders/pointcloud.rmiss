#version 460
#extension GL_EXT_ray_tracing : enable

struct RayPayload {
	bool hit;
};

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;

void main() {
	rayPayload.hit = false;
}