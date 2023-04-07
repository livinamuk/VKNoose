#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vTexCoord;

layout (location = 1) out vec2 texCoord;
layout (location = 2) out flat int textureIndex;
layout (location = 3) out flat int colorIndex;

struct ObjectData{
	mat4 model;
	int index_basecolor;
	int index_normals;
	int index_rma;
	int index_emissive;
} objectData;

//all object matrices
layout(std140,set = 0, binding = 0) readonly buffer ObjectBuffer{   
	ObjectData objects[];
} objectBuffer;

void main() 
{	
	mat4 modelMatrix = objectBuffer.objects[gl_InstanceIndex].model;
	textureIndex = int(objectBuffer.objects[gl_InstanceIndex].index_basecolor);
	colorIndex = int(objectBuffer.objects[gl_InstanceIndex].index_normals);
	gl_Position = modelMatrix * vec4(vPosition, 1.0);
	texCoord = vTexCoord;
}
