#pragma once
#include "GameObject.h"
#include "Scene.h"

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

void GameObject::SetScale(float scale) {
	_transform.scale = glm::vec3(scale);
}
void GameObject::SetScaleX(float scale) {
	_transform.scale.x = scale;
}

glm::mat4 GameObject::GetModelMatrix() {

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

void GameObject::SetParentName(std::string name) {
	_parentName = name;
}

void GameObject::SetScriptName(std::string name) {
	_scriptName = name;
}
bool GameObject::IsInteractable() {
	if (_scriptName == "OpenableDoor" ||
		_scriptName == "OpenableDrawer")
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
	if (_scriptName == "OpenableDrawer") {
		if (_openState == OpenState::CLOSED) {
			_openState = OpenState::OPENING;
			Audio::PlayAudio("DrawerOpen.wav", 0.5f);
		}
		if (_openState == OpenState::OPEN) {
			_openState = OpenState::CLOSING;
			Audio::PlayAudio("DrawerOpen.wav", 0.5f);
		}
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
	if (_scriptName == "OpenableDrawer") {
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

void GameObject::SetOpenState(OpenState openState, float speed, float min, float max) {
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
