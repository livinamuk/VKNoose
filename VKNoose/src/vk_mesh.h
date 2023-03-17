#pragma once

#include "vk_types.h"
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <iostream>
#include "Common.h"
#define GLM_FORCE_SILENT_WARNINGS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>
#include "glm/gtx/hash.hpp"

struct VertexInputDescription {
	std::vector<VkVertexInputBindingDescription> bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;

	VkPipelineVertexInputStateCreateFlags flags = 0;
};


struct Vertex {

	glm::vec3 position;
	float pad;
	glm::vec3 normal;
	float pad2;
	glm::vec2 uv;
	glm::vec2 nothing;

	static VertexInputDescription get_vertex_description();
	static VertexInputDescription get_vertex_description_position_and_texcoords_only();

	bool operator==(const Vertex& other) const {
		return position == other.position && normal == other.normal && uv == other.uv;
	}
};


namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv) << 1);
		}
	};
}


struct Mesh {
	std::vector<Vertex> _vertices;
	std::vector<uint32_t> _indices;
	std::string _filename;
	int _textureIndex = 0;

	AllocatedBuffer _vertexBuffer;
	AllocatedBuffer _indexBuffer;
	Transform _transform;

	bool load_from_obj(const char* filename);
	bool load_from_raw_data(std::vector<Vertex> vertices, std::vector<uint32_t>indices);
	void draw(VkCommandBuffer commandBuffer, uint32_t firstInstance);
};

struct Model {
	std::vector<Mesh> _meshes;
	bool load_from_obj(const char* filename);
	bool load_from_raw_data(std::vector<Vertex> vertices, std::vector<uint32_t>indices);
};