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
			_meshMaterials.resize(_model->_meshes.size()); 
		}
		else {
			std::cout << "Failed to set model '" << name << "', it does not exist.\n";
		}
	}
	
	void SetMeshMaterial(int meshIndex, const char* name) {
		// write an if is within range funcion if you ever crash on here. OK!
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