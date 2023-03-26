
struct RayPayload {
	uint instanceIndex;
	uint primitiveIndex;
	int done;
	vec3 color;
	float distance;
	vec3 normal;
	vec3 hitPos;
	vec3 attenuation;
};