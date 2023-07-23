#pragma once
#include "../common.h"
#include "../Game/AssetManager.h"
#include "Material.hpp"

namespace RasterRenderer {

	enum class Destination { MAIN_UI, LAPTOP_DISPLAY };

	struct UIInfo {
		int meshIndex = -1;
		Destination destination = Destination::MAIN_UI;
	};

	inline GPUObjectData2D _instanceData2D[MAX_RENDER_OBJECTS_2D] = {};
	inline std::vector<UIInfo> _UIToRender;
	inline int instanceCount = 0;

	inline void SubmitUI(int meshIndex, int textureIndex, int colorIndex, glm::mat4 modelMatrix, Destination destination, int xClipMin, int xClipMax, int yClipMin, int yClipMax) {
		int instanceIndex = instanceCount;
		_instanceData2D[instanceIndex].modelMatrix = modelMatrix;
		_instanceData2D[instanceIndex].index_basecolor = textureIndex;
		_instanceData2D[instanceIndex].index_color = colorIndex;
		_instanceData2D[instanceIndex].xClipMin = xClipMin;
		_instanceData2D[instanceIndex].xClipMax = xClipMax;
		_instanceData2D[instanceIndex].yClipMin = yClipMin;
		_instanceData2D[instanceIndex].yClipMax = yClipMax;

		UIInfo info;
		info.meshIndex = meshIndex;
		info.destination = destination;

		_UIToRender.push_back(info);
		instanceCount++;
	}

	inline void DrawMesh(VkCommandBuffer commandbuffer, int index) {
		Mesh* mesh = AssetManager::GetMesh(_UIToRender[index].meshIndex);
		mesh->draw(commandbuffer, index);
	}

	inline void ClearQueue() {
		_UIToRender.clear();
		instanceCount = 0;
	}

	inline void DrawQuad(const std::string& textureName, int xPosition, int yPosition, Destination destination, bool centered = false, int xSize = -1, int ySize = -1, int xClipMin = -1, int xClipMax = -1, int yClipMin = -1, int yClipMax = -1) {
		
		float quadWidth = xSize;
		float quadHeight = ySize;
		if (xSize == -1) {
			quadWidth = AssetManager::GetTexture(textureName)->_width;
		}
		if (ySize == -1) {
			quadHeight = AssetManager::GetTexture(textureName)->_height;
		}
		if (centered) {
			xPosition -= quadWidth / 2;
			yPosition -= quadHeight / 2;
		}
		float renderTargetWidth = 512;
		float renderTargetHeight = 288;

		if (destination == Destination::LAPTOP_DISPLAY) {
			renderTargetWidth = 640;
			renderTargetHeight = 430;
		}

		if (xClipMin == -1)
			xClipMin = 0;
		if (xClipMax == -1)
			xClipMax = renderTargetWidth;
		if (yClipMin == -1)
			yClipMin = 0;
		if (yClipMax == -1)
			yClipMax = renderTargetHeight;

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
		SubmitUI(meshIndex, textureIndex, 0, transform.to_mat4(), destination, xClipMin, xClipMax, yClipMin, yClipMax);
	}
}