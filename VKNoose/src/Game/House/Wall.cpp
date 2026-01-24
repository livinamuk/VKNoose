#include "Wall.h"
#include "AssetManagement/AssetManager.h"

Wall::Wall(glm::vec3 begin, glm::vec3 end, std::string materialName) {

	_material = AssetManager::GetMaterial(materialName);

	_begin = begin;
	_end = end;

	std::vector<uint32_t> indices = { 0, 1, 2, 0, 2, 3 };

	Vertex vert0;
	Vertex vert1;
	Vertex vert2;
	Vertex vert3;

	vert0.position = begin + glm::vec3(0, CEILING_HEIGHT - begin.y, 0);
	vert1.position = begin;
	vert2.position = end;
	vert3.position = end + glm::vec3(0, CEILING_HEIGHT - begin.y, 0);

	glm::vec3 normal = Util::NormalFromTriangle(vert0.position, vert1.position, vert3.position);
	vert0.normal = normal;
	vert1.normal = normal;
	vert2.normal = normal;
	vert3.normal = normal;

	float wallWidth = glm::length((vert1.position - vert2.position)) / CEILING_HEIGHT;

	float upperYCoord = (CEILING_HEIGHT - begin.y) / CEILING_HEIGHT;
	float lowerYCoord = 0;

	vert0.uv = glm::vec2(0, upperYCoord);
	vert1.uv = glm::vec2(wallWidth, upperYCoord);
	vert2.uv = glm::vec2(wallWidth, lowerYCoord);
	vert3.uv = glm::vec2(0, lowerYCoord);

	vert0.uv = glm::vec2(0, lowerYCoord);
	vert1.uv = glm::vec2(0, upperYCoord);
	vert2.uv = glm::vec2(wallWidth, upperYCoord);
	vert3.uv = glm::vec2(wallWidth, lowerYCoord);

	std::vector<Vertex> vertices;
	vertices.push_back(vert0);
	vertices.push_back(vert1);
	vertices.push_back(vert2);
	vertices.push_back(vert3);

	Util::SetTangentsFromVertices(vertices, indices);

	_meshIndex = AssetManager::CreateMeshOLD(vertices, indices);

	/*std::cout << "\nnew wall\n";
	for (auto index : indices) {
		Vertex v = vertices[index];
		//std::cout << v.position.x << ", " << v.position.y << ", " << v.position.z << "\n";
	}
	for (auto v : vertices) {
		std::cout << v.position.x << ", " << v.position.y << ", " << v.position.z << "\n";
	}*/
}