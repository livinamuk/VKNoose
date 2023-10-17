#pragma once
#include <iostream>
#define GLM_FORCE_SILENT_WARNINGS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>
#include "glm/gtx/hash.hpp"
#include "vk_types.h"
#include "vk_mesh.h"

#define NEAR_PLANE 0.01f
#define FAR_PLANE 25.0f
#define NOOSE_PI 3.14159265359f
#define NOOSE_HALF_PI 1.57079632679f

#define CEILING_HEIGHT 2.5f

#define DRAWER_VOLUME 1.0f
#define DOOR_VOLUME 1.0f
#define CABINET_VOLUME 1.0f

enum class InventoryViewMode { SCROLL, EXAMINE };
enum class DebugMode { NONE, RAY, COLLISION, DEBUG_MODE_COUNT };
enum class OpenState { NONE, CLOSED, CLOSING, OPEN, OPENING };
enum class OpenAxis { NONE, TRANSLATE_X, TRANSLATE_Y, TRANSLATE_Z, ROTATION_POS_X, ROTATION_POS_Y, ROTATION_POS_Z, ROTATION_NEG_X, ROTATION_NEG_Y, ROTATION_NEG_Z};
enum class InteractType { NONE, TEXT, QUESTION, PICKUP, CALLBACK_ONLY};

typedef void (*callback_function)(void); // type for conciseness

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

struct Material
{
	Material() {}
	std::string _name = "undefined";
	int _basecolor = 0;
	int _normal = 0;
	int _rma = 0;
};

struct InventoryItemData {
	std::string name;
	std::vector<std::string> menu;
	Material* material = nullptr;
	Model* model = nullptr;
	Transform transform;
};

struct FileInfo {
	std::string fullpath;
	std::string directory;
	std::string filename;
	std::string filetype;
	std::string materialType;
};

struct VertexInputDescription {
	std::vector<VkVertexInputBindingDescription> bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;
	VkPipelineVertexInputStateCreateFlags flags = 0;

	void Reset() {
		bindings.clear();
		attributes.clear();
		flags = 0;
	}
};

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

struct MeshInstance {
	glm::mat4 worldMatrix;
	int vertexOffset;
	int indexOffset;
	int basecolorIndex;
	int normalIndex;
	int rmaIndex;
	int materialType; // 0 standard, 1 mirror, 2 glass 
	int dummy1;
	int dummy2;
};


struct Extent2Di {
	int width;
	int height;
}; 

struct Extent3Di {
	int width;
	int height;
	int depth;
};

struct Extent2Df {
	float width;
	float height;
};

struct Extent3Df {
	float width;
	float height;
	float depth;
};



struct GPUObjectData {
	glm::mat4 modelMatrix;
	int index_basecolor;
	int index_normals;
	int index_rma;
	int index_emissive;
};

struct GPUObjectData2D {
	glm::mat4 modelMatrix;
	int index_basecolor;
	int index_color;
	int xClipMin;
	int xClipMax;
	int yClipMin;
	int yClipMax;	
	int dummy0;
	int dummy2;
};

/*
* 
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

*/

