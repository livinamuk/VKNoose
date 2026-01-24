#pragma once
#include <glm/glm.hpp>
#include "glm/gtx/hash.hpp"

struct Vertex {
	glm::vec3 position = glm::vec3(0);
	float pad = 0;

	glm::vec3 normal = glm::vec3(0);
	float pad2 = 0;

	glm::vec2 uv = glm::vec2(0);
	glm::vec2 pad3 = glm::vec2(0);

	glm::vec3 tangent = glm::vec3(0);
	float pad4 = 0;

	Vertex() {};
	Vertex(glm::vec3 pos) {
		position = pos;
	}

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

struct Vertex2D {
    glm::vec3 position;
    glm::vec2 uv;
};

struct VertexDebug {
    glm::vec3 position;
};
