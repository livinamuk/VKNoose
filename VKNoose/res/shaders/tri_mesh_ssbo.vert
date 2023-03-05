#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;
layout (location = 3) in vec2 vTexCoord;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 texCoord;
layout (location = 2) out vec3 worldPos;
layout (location = 3) out vec3 camPos;

layout(set = 0, binding = 0) uniform  CameraBuffer{   
    mat4 view;
    mat4 proj;
	mat4 viewproj; 
	vec4 viewPos;
} cameraData;

struct ObjectData{
	mat4 model;
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
	//modelMatrix = objectBuffer.objects[gl_InstanceIndex].model;
	//modelMatrix = PushConstants.render_matrix;
	mat4 transformMatrix = (cameraData.viewproj * modelMatrix);
	gl_Position = transformMatrix * vec4(vPosition, 1.0f);
	outColor = vColor;
	texCoord = vTexCoord;

	worldPos = (modelMatrix * vec4(vPosition, 1.0f)).xyz;
	camPos = cameraData.viewPos.xyz;

	vec3 normal = vNormal;
	camPos = normal;

    camPos.x = inverse(cameraData.view)[3][0];
    camPos.y = inverse(cameraData.view)[3][1];
    camPos.z = inverse(cameraData.view)[3][2];

	outColor = (vPosition * 0.5) + 0.5;
}
