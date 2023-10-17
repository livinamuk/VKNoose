#pragma once
#include "../vk_mesh.h"
#include "../vk_types.h"
#include "../vk_engine.h"
#include "../Renderer/Material.hpp"
#include <Filesystem>

struct AssetFile {
	char type[4];
	int version;
	std::string json;
	std::vector<char> binaryBlob;
};

enum class TextureFormat : uint32_t {
	Unknown = 0,
	RGBA8
};

enum class CompressionMode : uint32_t {
	None = 0,
	LZ4 = 0
};

struct TextureInfo {
	uint64_t textureSize;
	TextureFormat textureFormat;
	CompressionMode compressionMode;
	uint32_t pixelsize[3];
	std::string originalFile;
};

struct ModelInfo {
	uint64_t modelSize;
	CompressionMode compressionMode;
	uint32_t pixelsize[3];
	std::string originalFile;
};

enum class VertexFormat : uint32_t
{
	Unknown = 0,
	PNCV_F32, //everything at 32 bits
	P32N8C8V16 //position at 32 bits, normal at 8 bits, color at 8 bits, uvs at 16 bits float
};

struct MeshBounds {

	float origin[3];
	float radius;
	float extents[3];
};

struct MeshInfo {
	uint64_t vertexBuferSize;
	uint64_t indexBuferSize;
	MeshBounds bounds;
	VertexFormat vertexFormat;
	char indexSize;
	CompressionMode compressionMode;
	std::string originalFile;
};

struct ImageData {

	unsigned int texture = 0;
	int width = 0;
	int height = 0;
	int nrChannels = 0;
	unsigned char* data = nullptr;
	ImageData() {};
	ImageData(std::string path);
	void free();
	void save(std::string path);
	int getSize();
};

namespace AssetManager 
{
	//parses the texture metadata from an asset file
	TextureInfo read_texture_info(AssetFile* file);	
	void unpack_mesh(MeshInfo* info, const char* sourcebuffer, size_t sourceSize, char* vertexBufer, char* indexBuffer);
	void unpack_texture(TextureInfo* info, const char* sourcebuffer, size_t sourceSize, char* destination);
	AssetFile pack_mesh(MeshInfo* info, char* vertexData, char* indexData);
	AssetFile pack_texture(TextureInfo* info, void* pixelData);
	AssetFile pack_model(ModelInfo* info, void* pixelData);

	CompressionMode parse_compression(const char* f);
	TextureFormat parse_texture_format(const char* f);
	VertexFormat parse_vertex_format(const char* f);

	bool save_binaryfile(const char* path, const AssetFile& file);
	bool load_binaryfile(const char* path, AssetFile& outputFile);
	bool convert_image(const std::string inputPath, const std::string outputPath);

	void Init();
	void LoadFont();
	void LoadHardcodedMesh();
	bool LoadNextModel();
	bool LoadNextTexture();
	void BuildMaterials();

	//int CreateMesh(); 
	int CreateMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

	void* GetVertexPointer(int offset);
	void* GetIndexPointer(int offset);
	Vertex GetVertex(int offset);
	uint32_t GetIndex(int offset);

	std::vector<Vertex>& GetVertices_TEMPORARY();
	std::vector<uint32_t>& GetIndices_TEMPORARY();

	//int CreateModel(std::vector<int> meshIndices);
	void AddTexture(Texture& texture);
	int GetTextureIndex(const std::string& name);
	int GetMaterialIndex(const std::string& name);
	Mesh* GetMesh(int index);
	Texture* GetTexture(int index);
	Texture* GetTexture(const std::string& filename);
	Material* GetMaterial(int index);
	Material* GetMaterial(const std::string& filename);
	Model* GetModel(const std::string& name);
	//std::vector<Model*> GetAllModels();

	std::vector<Mesh>& GetMeshList();

	bool load_image_from_file(const char* file, Texture& outTexture, VkFormat imageFormat, bool generateMips = false);

	int GetNumberOfTextures();
	bool TextureExists(const std::string& name);

	void SaveImageData(std::string, int width, int height, int channels, void* data);
	void SaveImageDataF(std::string, int width, int height, int channels, void* data);
	void SaveImageDataU8(std::string path, int width, int height, int channels, void* data);
}