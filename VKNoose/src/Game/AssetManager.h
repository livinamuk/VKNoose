#pragma once
#include "../vk_mesh.h"
#include "../vk_types.h"
#include "../vk_engine.h"
#include "../Renderer/Material.hpp"

namespace AssetManager 
{
	void LoadAssets();
	void LoadTextures(VulkanEngine& engine);
	void BuildMaterials();

	void AddTexture(Texture& texture);
	Texture* GetTexture(int index);
	Texture* GetTexture(const std::string& filename);
	int GetTextureIndex(const std::string& filename);
	Material* GetMaterial(const std::string& filename);
	Model* GetModel(const std::string& name);
	std::vector<Model*> GetAllModels();

	bool load_image_from_file(VulkanEngine& engine, const char* file, Texture& outTexture, VkFormat imageFormat, bool generateMips = false);

	int GetNumberOfTextures();
	bool TextureExists(const std::string& name);
}