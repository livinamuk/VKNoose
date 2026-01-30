#version 460

layout (location = 0) in vec3 vPosition;

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

layout( push_constant ) uniform constants {
	mat4 transformation;
} pushConstants;

void main() 
{	
	gl_Position = pushConstants.transformation * vec4(vPosition, 1.0);
}
