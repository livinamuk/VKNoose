#pragma once
#include "API/Vulkan/vk_mesh.h"
#include "../../Common.h"
#include "../Util/Util.h"

#include <cstdint>

struct Wall {
	Wall() = default;
	Wall(const glm::vec3& begin, const glm::vec3& end, const std::string& materialName);

	uint64_t m_meshId = 0;
	glm::vec3 m_begin;
	glm::vec3 m_end;
	std::string m_materialName;

	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;
};
