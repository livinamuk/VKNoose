#include "vk_mesh.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <iostream>
#include <unordered_map>

VertexInputDescription Vertex::get_vertex_description()
{
	VertexInputDescription description;

	//we will have just 1 vertex buffer binding, with a per-vertex rate
	VkVertexInputBindingDescription mainBinding = {};
	mainBinding.binding = 0;
	mainBinding.stride = sizeof(Vertex);
	mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	description.bindings.push_back(mainBinding);

	VkVertexInputAttributeDescription positionAttribute = {};
	positionAttribute.binding = 0;
	positionAttribute.location = 0;
	positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	positionAttribute.offset = offsetof(Vertex, position);

	VkVertexInputAttributeDescription normalAttribute = {};
	normalAttribute.binding = 0;
	normalAttribute.location = 1;
	normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	normalAttribute.offset = offsetof(Vertex, normal);

	VkVertexInputAttributeDescription uvAttribute = {};
	uvAttribute.binding = 0;
	uvAttribute.location = 2;
	uvAttribute.format = VK_FORMAT_R32G32_SFLOAT;
	uvAttribute.offset = offsetof(Vertex, uv);

	VkVertexInputAttributeDescription tangentAttribute = {};
	tangentAttribute.binding = 0;
	tangentAttribute.location = 3;
	tangentAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	tangentAttribute.offset = offsetof(Vertex, tangent);

	VkVertexInputAttributeDescription bitangentAttribute = {};
	bitangentAttribute.binding = 0;
	bitangentAttribute.location = 4;
	bitangentAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	bitangentAttribute.offset = offsetof(Vertex, bitangent);

	description.attributes.push_back(positionAttribute);
	description.attributes.push_back(normalAttribute);
	description.attributes.push_back(uvAttribute);
	description.attributes.push_back(tangentAttribute);
	description.attributes.push_back(bitangentAttribute);
	return description;
}


VertexInputDescription Vertex::get_vertex_description_position_and_texcoords_only()
{
	VertexInputDescription description;

	VkVertexInputBindingDescription mainBinding = {};
	mainBinding.binding = 0;
	mainBinding.stride = sizeof(Vertex);
	mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	description.bindings.push_back(mainBinding);

	VkVertexInputAttributeDescription positionAttribute = {};
	positionAttribute.binding = 0;
	positionAttribute.location = 0;
	positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	positionAttribute.offset = offsetof(Vertex, position);

	VkVertexInputAttributeDescription uvAttribute = {};
	uvAttribute.binding = 0;
	uvAttribute.location = 1;
	uvAttribute.format = VK_FORMAT_R32G32_SFLOAT;
	uvAttribute.offset = offsetof(Vertex, uv);

	description.attributes.push_back(positionAttribute);
	description.attributes.push_back(uvAttribute);
	return description;
}

bool Mesh::load_from_raw_data(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
	
	//std::cout <<_filename << ": " << " vertices: " << vertices.size() << " indices: " << indices.size() << "\n";

	if (!vertices.size()) {
		return false;
	}
	else {
		_vertices = vertices;
		_indices = indices;
		_filename = "raw_data_supplied";
		return true;
	}
}


void Mesh::draw(VkCommandBuffer commandBuffer, uint32_t firstInstance)
{
	VkDeviceSize offset = 0;
	//std::cout << "_indices.size(): " << _indices.size() << "\n";
	//std::cout << "static_cast<uint32_t>_indices.size(): " << static_cast<uint32_t>(_indices.size()) << "\n";
	if (_indices.size()) {
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertexBuffer._buffer, &offset);
		vkCmdBindIndexBuffer(commandBuffer, _indexBuffer._buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_indices.size()), 1, 0, 0, firstInstance);
	}
	else {
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertexBuffer._buffer, &offset);
		vkCmdDraw(commandBuffer, _vertices.size(), 1, 0, firstInstance);
	}
}


Model::Model(const char* filename) {

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn;
	std::string err;

	_filename = filename;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename)) {
		std::cout << "Crashed loading model: " << filename << "\n";
		return;
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	for (const auto& shape : shapes)
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		for (int i = 0; i < shape.mesh.indices.size(); i++) {

			Vertex vertex = {};

			const auto& index = shape.mesh.indices[i];
			vertex.position = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			// Check if `normal_index` is zero or positive. negative = no normal data
			if (index.normal_index >= 0) {
				vertex.normal.x = attrib.normals[3 * size_t(index.normal_index) + 0];
				vertex.normal.y = attrib.normals[3 * size_t(index.normal_index) + 1];
				vertex.normal.z = attrib.normals[3 * size_t(index.normal_index) + 2];
			}

			if (attrib.texcoords.size() && index.texcoord_index != -1) { // should only be 1 or 2, some bug in debug where there were over 1000 on the spherelines model...
				vertex.uv = { attrib.texcoords[2 * index.texcoord_index + 0],	1.0f - attrib.texcoords[2 * index.texcoord_index + 1] };
			}

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}

		// Set tangents
		//std::cout << filename << "\n";
		//std::cout << "vertices.size(): " << vertices.size() << "\n";
		//std::cout << "uniqueVertices.size(): " << uniqueVertices.size() << "\n";
		if (vertices.size() > 0) {
			for (int i = 0; i < indices.size(); i += 3) {
				Vertex* vert0 = &vertices[indices[i]];
				Vertex* vert1 = &vertices[indices[i + 1]];
				Vertex* vert2 = &vertices[indices[i + 2]];
				// Shortcuts for UVs
				glm::vec3& v0 = vert0->position;
				glm::vec3& v1 = vert1->position;
				glm::vec3& v2 = vert2->position;
				glm::vec2& uv0 = vert0->uv;
				glm::vec2& uv1 = vert1->uv;
				glm::vec2& uv2 = vert2->uv;
				// Edges of the triangle : postion delta. UV delta
				glm::vec3 deltaPos1 = v1 - v0;
				glm::vec3 deltaPos2 = v2 - v0;
				glm::vec2 deltaUV1 = uv1 - uv0;
				glm::vec2 deltaUV2 = uv2 - uv0;
				float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
				glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
				glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
				vert0->tangent = tangent;
				vert1->tangent = tangent;
				vert2->tangent = tangent;
				vert0->bitangent = bitangent;
				vert1->bitangent = bitangent;
				vert2->bitangent = bitangent;
			}
		}

		Mesh& mesh = _meshes.emplace_back(Mesh());
		mesh._vertices = vertices;
		mesh._indices = indices;
		mesh._filename = filename;
	}
}

Model::Model() {
	// intentionally blank
}

Model::Model(const char* name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
{
	Mesh& mesh = _meshes.emplace_back(Mesh());
	mesh.load_from_raw_data(vertices, indices);
	_filename = name;
}

void Model::draw(VkCommandBuffer commandBuffer, uint32_t firstInstance) {
	for (Mesh& mesh : _meshes) {
		mesh.draw(commandBuffer, firstInstance);
	}
}