#version 460
#extension GL_EXT_scalar_block_layout : enable
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;

struct ObjDesc {
	mat4 worldMatrix;
	int vertexOffset;
	int indexOffset;
	int basecolorIndex;
	int normalIndex;
	int rmaIndex;
	int dummy;
	int dummy1;
	int dummy2;
};

layout(set = 0, binding = 1) uniform CameraData {
	mat4 proj;
	mat4 view;
	mat4 projInverse;
	mat4 viewInverse;
	vec4 viewPos;	
    int sizeOfVertex;
} camData;

layout(set = 0, binding = 2) buffer ObjDesc_ { ObjDesc i[]; } objDesc;

layout (location = 0) out vec3 normal;
layout (location = 1) out vec2 texCoords;
layout (location = 2) out flat int basecolorIndex;
layout (location = 3) out flat int normalIndex;
layout (location = 4) out flat int rmaIndex;

void main() {

	texCoords = vTexCoord;
	normal = vNormal;

	basecolorIndex = objDesc.i[gl_InstanceIndex].basecolorIndex;
	normalIndex = objDesc.i[gl_InstanceIndex].normalIndex;
	rmaIndex = objDesc.i[gl_InstanceIndex].rmaIndex;	

	mat4 proj = camData.proj;
	mat4 view = camData.view;
    mat4 model = objDesc.i[gl_InstanceIndex].worldMatrix;

    normal = transpose(inverse(mat3(model))) * vNormal;

	gl_Position = proj * view * model * vec4(vPosition, 1.0);
}
