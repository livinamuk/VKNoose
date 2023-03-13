#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;

layout (location = 0) out vec3 normal;
layout (location = 1) out vec2 texCoord;
layout (location = 2) out flat int textureIndex;

layout(set = 0, binding = 0) uniform  CameraBuffer{   
    mat4 view;
    mat4 proj;
	mat4 viewproj; 
	vec4 viewPos;
} cameraData;

struct ObjectData{
	mat4 model;
	int index_basecolor;
	int index_normals;
	int index_rma;
	int index_emissive;
} objectData;

//all object matrices
layout(std140,set = 1, binding = 0) readonly buffer ObjectBuffer{   
	ObjectData objects[];
} objectBuffer;

//push constants block
layout( push_constant ) uniform constants
{
 vec4 data;
 mat4 render_matrix;
} PushConstants;


void main() 
{	
	mat4 modelMatrix = objectBuffer.objects[gl_InstanceIndex].model;
	textureIndex = int(objectBuffer.objects[gl_InstanceIndex].index_basecolor);

	
	//gl_Position = cameraData.viewproj * vec4(worldPos, 1.0);
	gl_Position = modelMatrix * vec4(vPosition, 1.0);

	texCoord = vTexCoord;

	normal = vNormal;
	normal = (vPosition * 0.5) + 0.5;
}
