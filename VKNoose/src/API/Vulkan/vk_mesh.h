#pragma once

#include "vk_types.h"
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <iostream>
//#include "Common.h"
#define GLM_FORCE_SILENT_WARNINGS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>
#include "glm/gtx/hash.hpp"

//#include "API/Vulkan/Types/vk_acceleration_structure.h" // suss having this here

struct MeshOLD {
	uint32_t m_vertexOffset = 0;
	uint32_t m_indexOffset = 0;
	uint32_t m_vertexCount = 0;
	uint32_t m_indexCount = 0;

	//uint64_t m_vertexBuffer = 0;
	//uint64_t m_indexBuffer = 0;
	//uint64_t m_transformBuffer = 0;

	AllocatedBufferOLD m_vertexBufferOLD;
	AllocatedBufferOLD m_indexBufferOLD;
	AllocatedBufferOLD m_transformBufferOLD;
	//VulkanAccelerationStructure m_accelerationStructure;
	uint64_t m_vulkanAccelerationStructure = 0;
	std::string m_name = "undefined";
	bool m_uploadedToGPU = false;

	void draw(VkCommandBuffer commandBuffer, uint32_t firstInstance);
	uint64_t GetVulkanAccelerationStructureDeviceAddress();

	int32_t GetBaseVertex() const  { return m_vertexOffset; }
	int32_t GetBaseIndex() const   { return m_indexOffset; }
	int32_t GetVertexCount() const { return m_vertexCount; }
	int32_t GetIndexCount() const  { return m_indexCount; }
};

struct ModelOLD {
public:
	// methods
	ModelOLD();
	ModelOLD(const char* filename);
	void draw(VkCommandBuffer commandBuffer, uint32_t firstInstance);
	std::string m_filename = "undefined";
	std::vector<int> m_meshIndices;
	std::vector<std::string> m_meshNames;
	
private:

};