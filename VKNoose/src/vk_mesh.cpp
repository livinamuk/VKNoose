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

	//Position will be stored at Location 0
	VkVertexInputAttributeDescription positionAttribute = {};
	positionAttribute.binding = 0;
	positionAttribute.location = 0;
	positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	positionAttribute.offset = offsetof(Vertex, position);

	//Normal will be stored at Location 1
	VkVertexInputAttributeDescription normalAttribute = {};
	normalAttribute.binding = 0;
	normalAttribute.location = 1;
	normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	normalAttribute.offset = offsetof(Vertex, normal);

	//UV will be stored at Location 2
	VkVertexInputAttributeDescription uvAttribute = {};
	uvAttribute.binding = 0;
	uvAttribute.location = 2;
	uvAttribute.format = VK_FORMAT_R32G32_SFLOAT;
	uvAttribute.offset = offsetof(Vertex, uv);

	description.attributes.push_back(positionAttribute);
	description.attributes.push_back(normalAttribute);
	description.attributes.push_back(uvAttribute);
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

bool Mesh::load_from_raw_data(std::vector<Vertex> vertices, std::vector<uint32_t>indices) {
	
	std::cout <<_filename << ": " << " vertices: " << vertices.size() << " indices: " << indices.size() << "\n";


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

bool Mesh::load_from_obj(const char* filename)
{
	
	
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


bool Model::load_from_obj(const char* filename) 
{
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn;
		std::string err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename)) {
			std::cout << "Crashed loading model: " << filename << "\n";
			return false;
		}

		std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

		//glm::vec3 minPos = glm::vec3(9999, 9999, 9999);
		//glm::vec3 maxPos = glm::vec3(0, 0, 0);

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		// Loop over shapes
		for (size_t s = 0; s < shapes.size(); s++) {
			// Loop over faces(polygon)
			size_t index_offset = 0;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
				size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

				// Loop over vertices in the face.
				for (size_t v = 0; v < fv; v++)
				{
					Vertex vertex;

					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
					vertex.position.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
					vertex.position.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
					vertex.position.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
					// Check if `normal_index` is zero or positive. negative = no normal data
					if (idx.normal_index >= 0) {
						vertex.normal.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
						vertex.normal.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
						vertex.normal.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
					}
					// Check if `texcoord_index` is zero or positive. negative = no texcoord data
					if (idx.texcoord_index >= 0) {
						vertex.uv.x = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
						vertex.uv.y = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
					}
					vertices.push_back(vertex);
					indices.push_back(indices.size());
				}
				index_offset += fv;

				// per-face material
				shapes[s].mesh.material_ids[f];
			}
			//	Mesh* mesh = new Mesh(vertices, indices, shapes[s].name);
			//	this->m_meshes.push_back(mesh);
		}

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

			Mesh& mesh = _meshes.emplace_back(Mesh());
			mesh._vertices = vertices;
			mesh._indices = indices;
			mesh._filename = filename;
		}
	}
	return true;


}

bool Model::load_from_raw_data(std::vector<Vertex> vertices, std::vector<uint32_t>indices) 
{
	Mesh& mesh = _meshes.emplace_back(Mesh());
	mesh.load_from_raw_data(vertices, indices);
}