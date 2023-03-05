#pragma once
#include <iostream>
#define GLM_FORCE_SILENT_WARNINGS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>
#include "glm/gtx/hash.hpp"

/*
#define NEAR_PLANE 0.01f
#define FAR_PLANE 100.0f
#define NOOSE_PI 3.14159265359f
#define NOOSE_HALF_PI 1.57079632679f

#define YELLOW glm::vec3(1, 1, 0)
#define WHITE glm::vec3(1, 1, 1)

struct Triangle {
	glm::vec3 p1;
	glm::vec3 p2;
	glm::vec3 p3;
	void* parent = nullptr;
};

struct IntersectionResult {
	bool found = false;
	float distance = 0;
	float dot = 0;
	glm::vec2 baryPosition = glm::vec2(0);
};

enum class RaycastObjectType { NONE, FLOOR, WALLS, ENTITY};

enum class OpenState { CLOSED, CLOSING, OPEN, OPENING, STOPPED };

struct CameraRayData {
	bool found = false;
	float distanceToCamera = 99999;
	float interactDistance = 0;
	void* parent = nullptr;
	Triangle triangle;
	RaycastObjectType raycastObjectType = RaycastObjectType::NONE;
	glm::mat4 triangeleModelMatrix = glm::mat4(1);
	glm::vec3 intersectionPoint = glm::vec3(0);
	glm::vec2 baryPosition = glm::vec2(0);
	glm::vec3 closestPointOnBoundingBox = glm::vec3(0,0,0);
	unsigned int rayCount = 0;
};*/

struct Transform
{
	glm::vec3 position = glm::vec3(0);
	glm::vec3 rotation = glm::vec3(0);
	glm::vec3 scale = glm::vec3(1);

	glm::mat4 to_mat4()
	{
		glm::mat4 m = glm::translate(glm::mat4(1), position);
		glm::quat qt = glm::quat(rotation);
		m *= glm::mat4_cast(qt);
		m = glm::scale(m, scale);
		return m;
	}
};

/*struct Vertex {
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords = glm::vec2(0, 0);
	glm::vec3 Tangent;
	glm::vec3 Bitangent;
	unsigned int BlendingIndex[4];
	glm::vec4 BlendingWeight;
	unsigned int MaterialID = 0;

	bool operator==(const Vertex& other) const {
		return Position == other.Position && Normal == other.Normal && TexCoords == other.TexCoords;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.Position) ^ (hash<glm::vec3>()(vertex.Normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.TexCoords) << 1);
		}
	};
}*/
