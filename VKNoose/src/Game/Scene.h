#pragma once
#include "GameObject.h"
#include "../Common.h"
#include "../vk_types.h"

struct MeshRenderInfo {
	uint64_t _deviceAddress;
	VkTransformMatrixKHR _vkTransform;
	glm::mat4 _modelMatrix;
	int _basecolor;
	int _normal;
	int _rma;
	int _vertexOffset;
	int _indexOffset;
	Mesh* _mesh;
	//int _parentIndex;
	//std::string _parentType;
	void* _parent;
};

namespace Scene {
	void Init();
	void Update(GameData& gamedata, float deltaTime);
	std::vector<GameObject>& GetGameObjects();	
	std::vector<MeshRenderInfo> GetMeshRenderInfos();
	void StoreMousePickResult(int instanceIndex, int primitiveIndex);
	void ResetCollectedItems();
	GameObject* GetGameObjectByName(std::string);
	std::vector<Vertex> GetCollisionLineVertices();

	inline uint32_t _instanceIndex = 0;
	inline uint32_t _primitiveIndex = 0;
	inline uint32_t _rayhitGameObjectIndex = 0;
	inline std::vector<Vertex> _hitTriangleVertices;

	inline std::string _hitModelName;
	inline GameObject* _hoveredGameObject = nullptr;
}
