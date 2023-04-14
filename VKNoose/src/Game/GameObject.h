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
public:
	Model* _model = nullptr;
	std::vector<int> _meshMaterialIndices;
private:
	std::function<void(void)> _interactCallback = nullptr;
	callback_function _pickupCallback = nullptr;
	std::string _name = "undefined";
	std::string _parentName = "undefined";
	std::string _scriptName = "undefined";
	std::string _interactText = "";
	//std::string _interactTextOLD = "";
	//std::string _questionText = "";
	Transform _transform;
	OpenState _openState = OpenState::NONE;
	OpenAxis _openAxis = OpenAxis::NONE;
	float _maxOpenAmount = 0;
	float _minOpenAmount = 0;
	float _openSpeed = 0;
	bool _collected = false; // if this were an item
	BoundingBox _boundingBox;
	bool _collisionEnabled = false;
	InteractType _interactType = InteractType::NONE;

	
	struct AudioEffects {
		AudioEffectInfo onOpen;
		AudioEffectInfo onClose;
		AudioEffectInfo onInteract;
	} _audio;

public:
	GameObject();
	glm::mat4 GetModelMatrix();
	std::string GetName();
	void SetOpenAxis(OpenAxis openAxis);
	void SetAudioOnInteract(std::string filename, float volume);
	void SetAudioOnOpen(std::string filename, float volume);
	void SetAudioOnClose(std::string filename, float volume);
	void SetInteract(InteractType type, std::string text, std::function<void(void)> callback);
	//void SetPickUpQuestion(std::string text);
	//void SetPickUpCallback(callback_function callback);
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
	//void SetInteractText(std::string text);
	//void SetPickUpText(std::string text);
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
	void SetCollectedState(bool value);
	BoundingBox GetBoundingBox();
	void EnableCollision();
	bool HasCollisionsEnabled();
	bool IsCollected();
	const InteractType& GetInteractType();
	OpenState& GetOpenState();
};