#version 460
#extension GL_EXT_ray_tracing : require

struct RayPayload {
	bool hit;
};

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;

void main() {
	rayPayload.hit = true;
}
