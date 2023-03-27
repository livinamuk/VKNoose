#pragma once
#include "../vk_mesh.h"
#include "AssetManager.h"

struct GameObject {

	Model* _model = nullptr;
	//std::vector<Material*> _meshMaterials; 
	std::vector<int> _meshMaterialIndices;
private:
	std::string _name = "undefined";
	std::string _parentName = "undefined";
	std::string _scriptName = "undefined";
	Transform _transform;
	OpenState _openState = OpenState::CLOSED;
	float _maxOpenAmount = 0;
	float _minOpenAmount = 0;
	float _openSpeed = 0;

public:
	GameObject();
	glm::mat4 GetModelMatrix();
	std::string GetName();
	void SetOpenState(OpenState openState, float speed, float min, float max);
	void SetPosition(glm::vec3 position);
	void SetPositionX(float position);
	void SetPositionY(float position);
	void SetPositionZ(float position);
	void SetPosition(float x, float y, float z);
	void SetRotationX(float rotation);
	void SetRotationY(float rotation);
	void SetRotationZ(float rotation);
	float GetRotationX();
	float GetRotationY();
	float GetRotationZ();
	void SetScale(float scale);
	void SetScaleX(float scale);
	void SetName(std::string name);
	void SetParentName(std::string name);
	void SetScriptName(std::string name);
	bool IsInteractable();
	void Interact();
	void Update(float deltaTime);
	void SetModel(const std::string& name);
	VkTransformMatrixKHR GetVkTransformMatrixKHR();
	void SetMeshMaterial(const char* name, int meshIndex = -1);
	Material* GetMaterial(int meshIndex);
};