#pragma once
#include "../vk_mesh.h"
#include "../vk_types.h"
#include "../vk_engine.h"
#include "../Renderer/Material.hpp"

namespace AssetManager 
{
	void Init();
	void LoadAssets();
	void LoadTextures(VulkanEngine& engine);
	void BuildMaterials();

	//int CreateMesh(); 
	int CreateMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

	void* GetVertexPointer(int offset);
	void* GetIndexPointer(int offset);

	std::vector<Vertex>& GetVertices_TEMPORARY();
	std::vector<uint32_t>& GetIndices_TEMPORARY();

	//int CreateModel(std::vector<int> meshIndices);
	Mesh* GetMesh(int index);
	void AddTexture(Texture& texture);
	Texture* GetTexture(int index);
	Texture* GetTexture(const std::string& filename);
	int GetTextureIndex(const std::string& filename);
	Material* GetMaterial(const std::string& filename);
	Model* GetModel(const std::string& name);
	//std::vector<Model*> GetAllModels();

	std::vector<Mesh>& GetMeshList();

	bool load_image_from_file(VulkanEngine& engine, const char* file, Texture& outTexture, VkFormat imageFormat, bool generateMips = false);

	int GetNumberOfTextures();
	bool TextureExists(const std::string& name);
}