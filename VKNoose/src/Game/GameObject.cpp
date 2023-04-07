#pragma once
#include "GameObject.h"
#include "Scene.h"
#include "Callbacks.hpp"

GameObject::GameObject() {
}

void GameObject::SetPosition(float x, float y, float z) {
	_transform.position = glm::vec3(x, y, z);
}

void GameObject::SetPosition(glm::vec3 position) {
	_transform.position = position;
}

void GameObject::SetPositionX(float position) {
	_transform.position.x = position;
}

void GameObject::SetPositionY(float position) {
	_transform.position.y = position;
}

void GameObject::SetPositionZ(float position) {
	_transform.position.z = position;
}

void GameObject::SetRotationX(float rotation) {
	_transform.rotation.x = rotation;
}

void GameObject::SetRotationY(float rotation) {
	_transform.rotation.y = rotation;
}

void GameObject::SetRotationZ(float rotation) {
	_transform.rotation.z = rotation;
}

float GameObject::GetRotationX() {
	return _transform.rotation.x;
}

float GameObject::GetRotationY() {
	return _transform.rotation.y;
}

float GameObject::GetRotationZ() {
	return _transform.rotation.z;
}

glm::vec3 GameObject::GetPosition() {
	return _transform.position;
}


glm::mat4 GameObject::GetRotationMatrix() {
	Transform transform;
	transform.rotation = _transform.rotation;
	return transform.to_mat4();
}

void GameObject::SetScale(float scale) {
	_transform.scale = glm::vec3(scale);
}
void GameObject::SetScaleX(float scale) {
	_transform.scale.x = scale;
}

glm::mat4 GameObject::GetModelMatrix() {
	// If this is an item that has been collected, move if way below the world.
	if (_collected) {
		Transform t;
		t.position.y = -100;
		return t.to_mat4();
	}

	GameObject* parent = Scene::GetGameObjectByName(_parentName);
	if (parent) {
		return parent->GetModelMatrix() * _transform.to_mat4();
	}
	return _transform.to_mat4();
}

std::string GameObject::GetName() {
	return _name;
}

void GameObject::SetName(std::string name) {
	_name = name;
}

void GameObject::SetInteractText(std::string text) {
	_interactText = text;
}

void GameObject::SetPickUpText(std::string text) {
	_pickUpText = text;
}

void GameObject::SetParentName(std::string name) {
	_parentName = name;
}

void GameObject::SetScriptName(std::string name) {
	_scriptName = name;
}
bool GameObject::IsInteractable() {
	if (_scriptName == "OpenableDoor" ||
		_scriptName == "OpenableDrawer" ||
		_scriptName == "OpenableCabinet" ||
		_scriptName == "OpenCabinetDoor" || 
		_pickUpText != "" ||
		_interactText != "" ||
		_questionText != "")
		return true;
	return false;
}

void GameObject::Interact() {
	if (_scriptName == "OpenableDoor") {
		if (_openState == OpenState::CLOSED) {
			_openState = OpenState::OPENING;
			Audio::PlayAudio("Door_Open.wav", 0.25f);
		}
		if (_openState == OpenState::OPEN) {
			_openState = OpenState::CLOSING;
			Audio::PlayAudio("Door_Open.wav", 0.25f);
		}
	}    
	else if (_scriptName == "OpenableDrawer") {
		if (_openState == OpenState::CLOSED) {
			_openState = OpenState::OPENING;
			Audio::PlayAudio("DrawerOpen.wav", 0.5f);
		}
		if (_openState == OpenState::OPEN) {
			_openState = OpenState::CLOSING;
			Audio::PlayAudio("DrawerOpen.wav", 0.5f);
		}
	}
	else if (_scriptName == "OpenableCabinet") {
		if (_openState == OpenState::CLOSED) {
			_openState = OpenState::OPENING;
			Audio::PlayAudio("CabinetOpen.wav", 0.75f);
		}
		if (_openState == OpenState::OPEN) {
			_openState = OpenState::CLOSING;
			Audio::PlayAudio("CabinetClose.wav", 0.5f);
		}
	}
	else if (_scriptName == "OpenCabinetDoor") {
		Scene::GetGameObjectByName("CabinetDoor")->Interact();
	}
	// Pick up
	//else if (_pickUpText != "") {
	//	TextBlitter::AskQuestion(_pickUpText, this->PickUp);
	//}
	// Interact text
	else if (_interactText != "") {
		TextBlitter::Type(_interactText);
		Audio::PlayAudio("RE_type.wav", 0.9f);
	}
	// Question text
	else if (_questionText != "") {
		TextBlitter::AskQuestion(_questionText, this->_questionCallback);
		Audio::PlayAudio("RE_type.wav", 0.9f);
	}
}

void GameObject::Update(float deltaTime) {
	if (_scriptName == "OpenableDoor") {
		if (_openState == OpenState::OPENING) {
			_transform.rotation.y -= _openSpeed * deltaTime;
		}
		if (_openState == OpenState::CLOSING) {
			_transform.rotation.y += _openSpeed * deltaTime;
		}
		if (_transform.rotation.y < _maxOpenAmount) {
			_transform.rotation.y = _maxOpenAmount;
			_openState = OpenState::OPEN;
		}
		if (_transform.rotation.y > _minOpenAmount) {
			_transform.rotation.y = _minOpenAmount;
			_openState = OpenState::CLOSED;
		}
	}    
	else if (_scriptName == "OpenableDrawer") {
		if (_openState == OpenState::OPENING) {
			_transform.position.z += _openSpeed * deltaTime;
		}
		if (_openState == OpenState::CLOSING) {
			_transform.position.z -= _openSpeed * deltaTime;
		}
		if (_transform.position.z > _maxOpenAmount) {
			_transform.position.z = _maxOpenAmount;
			_openState = OpenState::OPEN;
		}
		if (_transform.position.z < 0) {
			_transform.position.z = 0;
			_openState = OpenState::CLOSED;
		}
	}
	else if (_scriptName == "OpenableCabinet") {
		if (_openState == OpenState::OPENING) {
			_transform.rotation.y += _openSpeed * deltaTime;
		}
		if (_openState == OpenState::CLOSING) {
			_transform.rotation.y -= _openSpeed * deltaTime;
		}
		if (_transform.rotation.y > _maxOpenAmount) {
			_transform.rotation.y = _maxOpenAmount;
			_openState = OpenState::OPEN;
		}
		if (_transform.rotation.y < _minOpenAmount) {
			_transform.rotation.y = _minOpenAmount;
			_openState = OpenState::CLOSED;
		}
	}

}

void GameObject::SetOpenState(OpenState openState, float speed, float min, float max) {
	_initalOpenState = openState;
	_openState = openState;
	_openSpeed = speed;
	_minOpenAmount = min;
	_maxOpenAmount = max;
}

void GameObject::SetModel(const std::string& name)
{
	_model = AssetManager::GetModel(name);

	if (_model) {
		_meshMaterialIndices.resize(_model->_meshIndices.size());
	}
	else {
		std::cout << "Failed to set model '" << name << "', it does not exist.\n";
	}
}

VkTransformMatrixKHR GameObject::GetVkTransformMatrixKHR() {
	VkTransformMatrixKHR transformMatrix = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f };
	glm::mat4 modelMatrix = glm::transpose(GetModelMatrix());
	for (int x = 0; x < 3; x++) {
		for (int y = 0; y < 4; y++) {
			transformMatrix.matrix[x][y] = modelMatrix[x][y];
		}
	}
	return transformMatrix;
}

void GameObject::SetMeshMaterial(const char* name, int meshIndex) {
	// Range checks
	if (!_meshMaterialIndices.size()) {
		std::cout << "Failed to set mesh material, GameObject has _meshMaterials with size " << _meshMaterialIndices.size() << "\n";
		return;
	}
	else if (meshIndex != -1 && meshIndex > _meshMaterialIndices.size()) {
		std::cout << "Failed to set mesh material with mesh Index " << meshIndex << ", GameObject has _meshMaterials with size " << _meshMaterialIndices.size() << "\n";
		return;
	}
	// Get the material index via the material name
	int materialIndex = AssetManager::GetMaterialIndex(name);
	// Set material for single mesh
	if (meshIndex != -1) {
		_meshMaterialIndices[meshIndex] = materialIndex;
	}
	// Set material for all mesh
	if (meshIndex == -1) {
		for (int i = 0; i < _meshMaterialIndices.size(); i++) {
			_meshMaterialIndices[i] = materialIndex;
		}
	}
}

Material* GameObject::GetMaterial(int meshIndex) {
	int materialIndex = _meshMaterialIndices[meshIndex];
	return AssetManager::GetMaterial(materialIndex);
}

void GameObject::PickUp() {
	//if (_pickupCallback != nullptr)
	//	_pickupCallback();
	Audio::PlayAudio("ItemPickUp.wav", 0.5f);
	_collected = true;
	std::cout << "Picked up \"" << GetName() << "\"\n";
	std::cout << "_collected \"" << _collected << "\"\n";
	//std::cout << "Picked up \"" << _transform.position.x << " " <<_transform.position.y << " " << _transform.position.z << " " << "\"\n";
}

void GameObject::SetPickUpCallback(callback_function callback) {
	_pickupCallback = callback;
}


void GameObject::SetQuestion(std::string text, std::function<void(void)> callback) {
	_questionText = text;
	_questionCallback = callback;
	//_questionCallback();
}

void GameObject::ResetToInitialState() {
	_collected = false;
	if (_initalOpenState == OpenState::CLOSED && _openState != OpenState::CLOSED)
		_openState = OpenState::CLOSING;
	if (_initalOpenState == OpenState::OPEN && _openState != OpenState::OPEN)
		_openState = OpenState::OPENING;
}


void GameObject::SetBoundingBoxFromMesh(int meshIndex) {

	Mesh* mesh = AssetManager::GetMesh(_model->_meshIndices[meshIndex]);	
	std::vector<Vertex> vertices = AssetManager::GetVertices_TEMPORARY();

	int firstIndex = mesh->_vertexOffset;
	int lastIndex = firstIndex + (int)mesh->_vertexCount;

	for (int i = firstIndex; i < lastIndex; i++) {
		_boundingBox.xLow = std::min(_boundingBox.xLow, vertices[i].position.x);
		_boundingBox.xHigh = std::max(_boundingBox.xHigh, vertices[i].position.x);
		_boundingBox.zLow = std::min(_boundingBox.zLow, vertices[i].position.z);
		_boundingBox.zHigh = std::max(_boundingBox.zHigh, vertices[i].position.z);
	}
}

BoundingBox GameObject::GetBoundingBox() {
	return _boundingBox;
}

void GameObject::EnableCollision() {
	_collisionEnabled = true;
}

bool GameObject::HasCollisionsEnabled() {
	return _collisionEnabled;
}