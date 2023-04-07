#pragma once
#include "../vk_mesh.h"
#include "AssetManager.h"

struct BoundingBox {
	float xLow = 0;
	float xHigh = 0;
	float zLow = 0;
	float zHigh = 0;
};

struct GameObject {

	Model* _model = nullptr;
	//std::vector<Material*> _meshMaterials; 
	std::vector<int> _meshMaterialIndices;
private:
	std::function<void(void)> _questionCallback = nullptr;
	callback_function _pickupCallback = nullptr;

	std::string _name = "undefined";
	std::string _parentName = "undefined";
	std::string _scriptName = "undefined";
	std::string _pickUpScriptName = "undefined";
	std::string _pickUpText = "";
	std::string _interactText = "";
	std::string _questionText = "";
	std::string _questionOutcomeScript = "";
	Transform _transform;
	OpenState _initalOpenState = OpenState::CLOSED;
	OpenState _openState = OpenState::CLOSED;
	float _maxOpenAmount = 0;
	float _minOpenAmount = 0;
	float _openSpeed = 0;
	bool _collected = false; // if this were an item
	BoundingBox _boundingBox;
	bool _collisionEnabled = false;

public:
	GameObject();
	glm::mat4 GetModelMatrix();
	std::string GetName();
	void SetQuestion(std::string text, std::function<void(void)> callback);
	void SetPickUpCallback(callback_function callback);
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
	glm::vec3 GetPosition();
	glm::mat4 GetRotationMatrix();
	void SetScale(float scale);
	void SetScaleX(float scale);
	void SetInteractText(std::string text);
	void SetPickUpText(std::string text);
	void SetName(std::string name);
	void SetParentName(std::string name);
	void SetScriptName(std::string name);
	bool IsInteractable();
	void Interact();
	void Update(float deltaTime);
	void SetModel(const std::string& name);
	void SetBoundingBoxFromMesh(int meshIndex);
	VkTransformMatrixKHR GetVkTransformMatrixKHR();
	void SetMeshMaterial(const char* name, int meshIndex = -1);
	Material* GetMaterial(int meshIndex);
	void PickUp();
	void ResetToInitialState();
	BoundingBox GetBoundingBox();
	void EnableCollision();
	bool HasCollisionsEnabled();
};