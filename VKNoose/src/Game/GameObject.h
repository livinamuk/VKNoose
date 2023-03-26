#pragma once
#include "../vk_mesh.h"
#include "AssetManager.h"

struct GameObject {

	Transform _worldTransform;
	Model* _model = nullptr;
	std::vector<Material*> _meshMaterials;
	
	GameObject() {
	}

	void SetModel(const std::string& name) 	
	{
		_model = AssetManager::GetModel(name);

		if (_model) {
			_meshMaterials.resize(_model->_meshIndices.size()); 
		}
		else {
			std::cout << "Failed to set model '" << name << "', it does not exist.\n";
		}
	}

	VkTransformMatrixKHR GetVkTransformMatrixKHR() {
		VkTransformMatrixKHR transformMatrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f };

		glm::mat4 modelMatrix = _worldTransform.to_mat4();
		modelMatrix = glm::transpose(modelMatrix);

		for (int x = 0; x < 3; x++) {
			for (int y = 0; y < 4; y++) {
				transformMatrix.matrix[x][y] = modelMatrix[x][y];
			}
		}
		return transformMatrix;
	}
	
	void SetMeshMaterial(int meshIndex, const char* name) {
		if (meshIndex < 0 || meshIndex > _meshMaterials.size() || !_meshMaterials.size()) {
			std::cout << "Index " << meshIndex << " is out of range, GameObject has _meshMaterials size " << _meshMaterials.size() << "\n";
			return;
		}

		Material* material = AssetManager::GetMaterial(name);
		if (material) {
			_meshMaterials[meshIndex] = material;

			if (meshIndex == 0) {
				for (int i = 0; i < _meshMaterials.size(); i++) {
					_meshMaterials[i] = material;
				}
			}
		}
	}
};