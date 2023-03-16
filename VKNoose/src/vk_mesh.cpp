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
				//		for (const auto& index : shape.mesh.indices) {
				Vertex vertex = {};

				const auto& index = shape.mesh.indices[i];
				//vertex.MaterialID = shape.mesh.material_ids[i / 3];

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
					vertex.uv = {attrib.texcoords[2 * index.texcoord_index + 0],	1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
				}

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}

			for (int i = 0; i < indices.size(); i += 3) {
				//Util::SetNormalsAndTangentsFromVertices(&vertices[indices[i]], &vertices[indices[i + 1]], &vertices[indices[i + 2]]);
			}

				_vertices = vertices;
			_indices = indices;

			_filename = filename;

		}

	}


	return true;

	/*
	//attrib will contain the vertex arrays of the file
	tinyobj::attrib_t attrib;
	//shapes contains the info for each separate object in the file
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn;
	std::string err;

	tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename,
		nullptr);
	if (!warn.empty()) {
		//std::cout << "WARN: " << warn << std::endl;
	}
	if (!err.empty()) {
		std::cerr << err << std::endl;
		return false;
	}


	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			//hardcode loading to triangles
			int fv = 3;
			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				//vertex position
				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
				//vertex normal
				tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
				//vertex uv
				tinyobj::real_t ux = attrib.texcoords[2 * idx.texcoord_index + 0];
				tinyobj::real_t uy = attrib.texcoords[2 * idx.texcoord_index + 1];
				//copy it into our vertex
				Vertex new_vert;
				new_vert.position.x = vx;
				new_vert.position.y = vy;
				new_vert.position.z = vz;
				new_vert.normal.x = nx;
				new_vert.normal.y = ny;
				new_vert.normal.z = nz;
				new_vert.uv.x = ux;
				new_vert.uv.y = 1 - uy;
				_vertices.push_back(new_vert);
			}
			index_offset += fv;
		}
	}
	_filename = filename;
	return true;*/
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
