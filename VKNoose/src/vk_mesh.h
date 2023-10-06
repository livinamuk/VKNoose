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

struct Mesh {
	uint32_t _vertexOffset;
	uint32_t _indexOffset;
	uint32_t _vertexCount;
	uint32_t _indexCount;

	AllocatedBuffer _vertexBuffer;
	AllocatedBuffer _indexBuffer;
	AllocatedBuffer _transformBuffer;
	//Transform _transform;
	AccelerationStructure _accelerationStructure;
	std::string _name = "undefined";
	bool _uploadedToGPU = false;

	void draw(VkCommandBuffer commandBuffer, uint32_t firstInstance);
};

struct Model {
public:
	// methods
	Model();
	Model(const char* filename);
	void draw(VkCommandBuffer commandBuffer, uint32_t firstInstance);
	std::string _filename = "undefined";
	std::vector<int> _meshIndices;
	std::vector<std::string> _meshNames;
	
private:
	// methods
	//bool load_from_obj(const char* filename);
	//bool load_from_raw_data(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
};