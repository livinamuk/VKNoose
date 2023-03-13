#pragma once

#include "vk_types.h"
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct VertexInputDescription {
	std::vector<VkVertexInputBindingDescription> bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;

	VkPipelineVertexInputStateCreateFlags flags = 0;
};


struct Vertex {

	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 color;
	glm::vec2 uv;

	static VertexInputDescription get_vertex_description();
};

struct Mesh {
	std::vector<Vertex> _vertices;
	std::vector<uint16_t> _indices;
	std::string _filename;

	AllocatedBuffer _vertexBuffer;
	AllocatedBuffer _indexBuffer;

	bool load_from_obj(const char* filename);
	bool load_from_raw_data(std::vector<Vertex> vertices, std::vector<uint16_t>indices);
	void draw(VkCommandBuffer commandBuffer, uint32_t firstInstance);

};