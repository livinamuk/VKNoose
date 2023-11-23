#version 460

#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;

layout (location = 0) out vec3 Color;




layout( push_constant ) uniform constants {
	mat4 transformation;
	bool useNormalForColor;
} pushConstants;

// Buffers
layout(set = 2, binding = 0) readonly buffer PointCloudPositions { vec4 i[]; } pointCloudPositions;
layout(set = 2, binding = 1) readonly buffer PointCloudDirectLighting { vec4 i[]; } pointCloudDirectLighting;
layout(set = 2, binding = 2) readonly buffer PointCloudNormals { vec4 i[]; } pointCloudNormals;
layout(set = 2, binding = 3) readonly buffer PointCloudDirtyFlags { float i[]; } pointCloudDirtyFlags;
layout(set = 2, binding = 4) readonly buffer ProbeGrid { vec4 i[]; } probeGrid;

void main() 
{	
	gl_Position = pushConstants.transformation * vec4(vPosition, 1.0);
	gl_PointSize = 2.0;

	Color = vec3(1, 1, 0);

	if (pushConstants.useNormalForColor) {		
		Color = vNormal;
		Color =  pointCloudDirectLighting.i[gl_VertexIndex].rgb;
		//swColor =  pointCloudNormals.i[gl_VertexIndex].rgb;
		//Color =  vec3(pointCloudDirtyFlags.i[gl_VertexIndex]);
			
	
		Color =  pointCloudPositions.i[gl_VertexIndex].rgb;

		// debug hack to view prove as differetn to point cloud lighting
	} else {
		Color =  probeGrid.i[gl_VertexIndex].rgb / probeGrid.i[gl_VertexIndex].a * 100;
	//	Color =  probeGrid.i[gl_VertexIndex].rgb;
	//	Color = vPosition;

		if (Color.x + Color.y + Color.z < 0.1)
			Color = vec3(0);
	
	//	Color = vPosition;
	}

	//Color = probeGrid.i[gl_VertexIndex].rgb;
	
	//float vertexId = float(gl_VertexIndex);
	
}
