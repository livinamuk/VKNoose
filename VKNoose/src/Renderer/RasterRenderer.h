#pragma once
#include "../common.h"
#include "../Game/AssetManager.h"
#include "Material.hpp"

namespace RasterRenderer {

	inline GPUObjectData _instanceData[MAX_RENDER_OBJECTS] = {};
	inline std::vector<int> _UIToRender;
	int instanceCount = 0;

	inline void SubmitUI(int meshIndex, int textureIndex, int colorIndex, glm::mat4 modelMatrix) {
		int instanceIndex = instanceCount;
		_instanceData[instanceIndex].modelMatrix = modelMatrix;
		_instanceData[instanceIndex].index_basecolor = textureIndex;
		_instanceData[instanceIndex].index_normals = colorIndex;
		_UIToRender.push_back(meshIndex);
		instanceCount++;
	}

	inline void DrawMesh(VkCommandBuffer commandbuffer, int index) {
		Mesh* mesh = AssetManager::GetMesh(_UIToRender[index]);
		mesh->draw(commandbuffer, index);
	}

	inline void ClearQueue() {
		_UIToRender.clear();
		instanceCount = 0;
	}

	inline void DrawQuad(const std::string& textureName, int xPosition, int yPosition, bool centered = false, int xSize = -1, int ySize = -1) {
		
		float quadWidth = xSize;
		float quadHeight = ySize;
		if (xSize == -1 || ySize == -1) {
			quadWidth = AssetManager::GetTexture(textureName)->_width;
			quadHeight = AssetManager::GetTexture(textureName)->_height;
		}
		if (centered) {
			xPosition -= quadWidth / 2;
			yPosition -= quadHeight / 2;
		}
		float renderTargetWidth = 512;
		float renderTargetHeight = 288;
		float width = (1.0f / renderTargetWidth) * quadWidth;
		float height = (1.0f / renderTargetHeight) * quadHeight;
		float ndcX = ((xPosition + (quadWidth / 2.0f)) / renderTargetWidth) * 2 - 1;
		float ndcY = ((yPosition + (quadHeight / 2.0f)) / renderTargetHeight) * 2 - 1;
		Transform transform;
		transform.position.x = ndcX;
		transform.position.y = ndcY * -1;
		transform.scale = glm::vec3(width, height * -1, 1);
		int meshIndex = AssetManager::GetModel("blitter_quad")->_meshIndices[0];
		int textureIndex = AssetManager::GetTextureIndex(textureName);
		SubmitUI(meshIndex, textureIndex, 0, transform.to_mat4());
	}

	/*inline void SubmitForRender(int meshIndex, Material* material, glm::mat4 modelMatrix) {
		int index = _meshToRender.size();
		_rasterInstanceData[index].modelMatrix = modelMatrix;
		_rasterInstanceData[index].index_basecolor = material->_basecolor;
		_rasterInstanceData[index].index_rma = material->_rma;
		_rasterInstanceData[index].index_normals = material->_normal;
		_meshToRender.push_back(meshIndex);
	}*/
}