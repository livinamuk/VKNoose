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

/*void GameObject::SetInteractText(std::string text) {
	_interactTextOLD = text;
}*/

void GameObject::SetParentName(std::string name) {
	_parentName = name;
}

void GameObject::SetScriptName(std::string name) {
	_scriptName = name;
}
bool GameObject::IsInteractable() {
	if (_openState == OpenState::CLOSED ||
		_openState == OpenState::OPEN ||
		_openState == OpenState::CLOSING ||
		_openState == OpenState::OPENING ||
		_interactType == InteractType::PICKUP ||
		_interactType == InteractType::TEXT || 
		_interactType == InteractType::QUESTION)
		return true;
	return false;
}

void GameObject::Interact() {
	// Open
	if (_openState == OpenState::CLOSED) {
		_openState = OpenState::OPENING; 
		Audio::PlayAudio(_audio.onOpen.c_str(), 0.5f);
	}
	// Close
	else if (_openState == OpenState::OPEN) {
		_openState = OpenState::CLOSING;
		Audio::PlayAudio(_audio.onClose.c_str(), 0.5f);
	}
	// Interact text
	else if (_interactType == InteractType::TEXT) {
		// blit any text
		TextBlitter::Type(_interactText);
		// call any callback
		if (_interactCallback)
			_interactCallback();
		// Audio
		if (_audio.onInteract.length() > 0) {
			Audio::PlayAudio(_audio.onInteract.c_str(), 0.5f);
		}
		// Default
		else if (_interactText.length() > 0) {
			Audio::PlayAudio("RE_type.wav", 1.0f);
		}
	}
	// Question text
	else if (_interactType == InteractType::QUESTION) {
		TextBlitter::AskQuestion(_interactText, this->_interactCallback, nullptr);
		Audio::PlayAudio("RE_type.wav", 1.0f);
	}
	// Pick up
	else if (_interactType == InteractType::PICKUP) {
		TextBlitter::AskQuestion(_interactText, this->_interactCallback, this);
		Audio::PlayAudio("RE_type.wav", 1.0f);
	}
}

void GameObject::Update(float deltaTime) {
	// Open/Close if applicable
	if (_openState != OpenState::NONE) {
		// Rotation
		if (GetName() == "Door") {
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
		else if (GetName() == "Cabinet Door") {
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
			if (_transform.rotation.y < 0) {
				_transform.rotation.y = 0;
				_openState = OpenState::CLOSED;
			}
		}
		// Position
		else {
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


void GameObject::SetOnInteractAudio(std::string filename) {
	_audio.onInteract = filename;
}

void GameObject::SetOpenState(OpenState openState, float speed, float min, float max, std::string audioOnOpen, std::string audioOnClose) {
	//_initalOpenState = openState;
	_openState = openState;
	_openSpeed = speed;
	_minOpenAmount = min;
	_maxOpenAmount = max;
	_audio.onOpen = audioOnOpen;
	_audio.onClose = audioOnClose;
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
#include "GameData.h"

void GameObject::PickUp() {
	GameData::AddInventoryItem(GetName());
	Audio::PlayAudio("ItemPickUp.wav", 0.5f);
	_collected = true;
	std::cout << "Picked up \"" << GetName() << "\"\n";
	std::cout << "_collected \"" << _collected << "\"\n";
	//std::cout << "Picked up \"" << _transform.position.x << " " <<_transform.position.y << " " << _transform.position.z << " " << "\"\n";
}

void GameObject::SetCollectedState(bool value) {
	_collected = value;
}

void GameObject::SetInteract(InteractType type, std::string text, std::function<void(void)> callback) {
	_interactType = type;
	_interactText = text;
	_interactCallback = callback;
}

void GameObject::SetBoundingBoxFromMesh(int meshIndex) {

	Mesh* mesh = AssetManager::GetMesh(_model->_meshIndices[meshIndex]);	
	std::vector<Vertex>& vertices = AssetManager::GetVertices_TEMPORARY();
	std::vector<uint32_t>& indices = AssetManager::GetIndices_TEMPORARY();

	int firstIndex = mesh->_indexOffset;
	int lastIndex = firstIndex + (int)mesh->_indexCount;

	for (int i = firstIndex; i < lastIndex; i++) {
		_boundingBox.xLow = std::min(_boundingBox.xLow, vertices[indices[i] + mesh->_vertexOffset].position.x);
		_boundingBox.xHigh = std::max(_boundingBox.xHigh, vertices[indices[i] + mesh->_vertexOffset].position.x);
		_boundingBox.zLow = std::min(_boundingBox.zLow, vertices[indices[i] + mesh->_vertexOffset].position.z);
		_boundingBox.zHigh = std::max(_boundingBox.zHigh, vertices[indices[i] + mesh->_vertexOffset].position.z);
	}	
	/*
	std::cout << "\n" << GetName() << "\n";
	std::cout << " meshIndex: " << meshIndex << "\n";
	std::cout << " firstIndex: " << firstIndex << "\n";
	std::cout << " lastIndex: " << lastIndex << "\n";
	std::cout << " _boundingBox.xLow: " << _boundingBox.xLow << "\n";
	std::cout << " _boundingBox.xHigh: " << _boundingBox.xHigh << "\n";
	std::cout << " _boundingBox.zLow: " << _boundingBox.zLow << "\n";
	std::cout << " _boundingBox.zHigh: " << _boundingBox.zHigh << "\n";*/
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

bool GameObject::IsCollected() {
	return _collected;
}

const InteractType& GameObject::GetInteractType() {
	return _interactType;
}