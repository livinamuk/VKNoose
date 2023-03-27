#include "vk_mesh.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <iostream>
#include <unordered_map>
#include "Util.h"
#include "Game/AssetManager.h"


/*bool Mesh::load_from_raw_data(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
	
	//std::cout <<_filename << ": " << " vertices: " << vertices.size() << " indices: " << indices.size() << "\n";

	if (!vertices.size()) {
		return false;
	}
	else {
		_vertices = vertices;
		_indices = indices;
		//_filename = "raw_data_supplied";
		return true;
	}
}*/


void Mesh::draw(VkCommandBuffer commandBuffer, uint32_t firstInstance)
{
	if (_vertexCount <= 0)
		return;

	VkDeviceSize offset = 0;
	if (_indexCount > 0) {
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertexBuffer._buffer, &offset);
		vkCmdBindIndexBuffer(commandBuffer, _indexBuffer._buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_indexCount), 1, 0, 0, firstInstance);
	}
	else {
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertexBuffer._buffer, &offset);
		vkCmdDraw(commandBuffer, _vertexCount, 1, 0, firstInstance);
	}
}


Model::Model(const char* filepath) {

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn;
	std::string err;

	FileInfo fileinfo = Util::GetFileInfo(filepath);
	_filename = fileinfo.filename;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath)) {
		std::cout << "Crashed loading model: " << filepath << "\n";
		return;
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

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


		Util::SetTangentsFromVertices(vertices, indices);


		if (shape.name == "Camila_Brow") {
			indices = { 0,0,0 };
		}

		int meshIndex = AssetManager::CreateMesh(vertices, indices);
		_meshIndices.push_back(meshIndex);
		AssetManager::GetMesh(meshIndex)->_name = shape.name;
		//std::cout << shape.name << "\n";
	}
	//std::cout << "Loaded " << filename << " (" << _meshIndices.size() << " meshes)\n";
}

Model::Model() {
	// intentionally blank
}

/*
Model::Model(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, const char* name)
{
	Mesh& mesh = _meshes.emplace_back(Mesh());
	mesh.load_from_raw_data(vertices, indices);
	_filename = name;
}*/

void Model::draw(VkCommandBuffer commandBuffer, uint32_t firstInstance) {

	for (int i = 0; i < _meshIndices.size(); i++)
		AssetManager::GetMesh(_meshIndices[i])->draw(commandBuffer, firstInstance);
}