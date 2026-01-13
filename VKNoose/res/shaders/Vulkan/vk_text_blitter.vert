#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vTexCoord;

layout (location = 0) out vec3 normal;
layout (location = 1) out vec2 texCoord;
layout (location = 2) out flat int textureIndex;
layout (location = 3) out flat int colorIndex;
layout (location = 4) out flat int xClipMin;
layout (location = 5) out flat int xClipMax;
layout (location = 6) out flat int yClipMin;
layout (location = 7) out flat int yClipMax;

struct ObjectData2D{
	mat4 model;
	int index_basecolor;
	int index_color;
	int xClipMin;
	int xClipMax;
	int yClipMin;
	int yClipMax;
	int dummy0;
	int dummy2;
} objectData;

//all object matrices
layout(std140,set = 0, binding = 3) readonly buffer ObjectBuffer{   
	ObjectData2D objects[];
} objectBuffer;

void main() 
{	
	mat4 modelMatrix = objectBuffer.objects[gl_InstanceIndex].model;
	textureIndex = int(objectBuffer.objects[gl_InstanceIndex].index_basecolor);
	colorIndex = int(objectBuffer.objects[gl_InstanceIndex].index_color);
	xClipMin = int(objectBuffer.objects[gl_InstanceIndex].xClipMin);
	xClipMax = int(objectBuffer.objects[gl_InstanceIndex].xClipMax);
	yClipMin = int(objectBuffer.objects[gl_InstanceIndex].yClipMin);
	yClipMax = int(objectBuffer.objects[gl_InstanceIndex].yClipMax);
	gl_Position = modelMatrix * vec4(vPosition, 1.0);
	texCoord = vTexCoord;
}
