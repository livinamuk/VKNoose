#pragma once
#include "GameObject.h"
#include "../Common.h"
#include "API/Vulkan/vk_types.h"

struct RenderItem {
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

struct Light {
	enum State { ON, OFF };
	glm::vec3 position;
	glm::vec3 color;
	float currentBrightness = 1.0;
	State state;
	void Update(float delatTime);
};

struct LightRenderInfo {
	glm::vec4 position;
	glm::vec4 color;
};

namespace Scene {
	void Init();
	void Update(float deltaTime);
	std::vector<GameObject>& GetGameObjects();
	//std::vector<RenderItem> GetRenderItems(bool debugScene);
	std::vector<MeshInstance> GetSceneMeshInstances(bool debugScene);
	std::vector<MeshInstance> GetInventoryMeshInstances(bool debugScene);

	void UpdateInventoryScene(float deltaTime);
	std::vector<VkAccelerationStructureInstanceKHR> GetMeshInstancesForSceneAccelerationStructure();
	std::vector<VkAccelerationStructureInstanceKHR> GetMeshInstancesForInventoryAccelerationStructure();
	std::vector<Mesh*> GetSceneMeshes(bool debugScene);

	void StoreMousePickResult(int instanceIndex, int primitiveIndex);
	GameObject* GetGameObjectByName(std::string);
	GameObject* GetGameObjectByIndex(int index);
	std::vector< LightRenderInfo> GetLightRenderInfo();
	std::vector< LightRenderInfo> GetLightRenderInfoInventory();
	int GetGameObjectCount();
	std::vector<Vertex> GetCollisionLineVertices();
	void SetLightState(int index, Light::State state);

	inline uint32_t _instanceIndex = 0;
	inline uint32_t _primitiveIndex = 0;
	inline uint32_t _rayhitGameObjectIndex = 0;
	inline std::vector<Vertex> _hitTriangleVertices;
	inline std::string _hitModelName;
	inline GameObject* _hoveredGameObject = nullptr;
}
