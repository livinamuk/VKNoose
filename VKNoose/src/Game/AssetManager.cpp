#include "AssetManager.h"
#include "../Util.h"
#include "../vk_initializers.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <cmath>

namespace AssetManager {
	std::unordered_map<std::string, Model> _models;
	//std::unordered_map<std::string, Material> _materials;
	std::vector<Mesh> _meshes;
	std::vector<Material> _materials;
	std::vector<Texture> _textures;
	std::vector<Vertex> _vertices;		// ALL of em
	std::vector<uint32_t> _indices;		// ALL of em
	int _vertexOffset = 0;				// insert index for next mesh
	int _indexOffset = 0;				// insert index for next mesh
}

void AssetManager::Init() {
	//_vertices.reserve(10000);
	//_indices.reserve(10000);
}

void* AssetManager::GetVertexPointer(int offset) {
	return &_vertices[offset];
}

Vertex AssetManager::GetVertex(int offset) {
	return _vertices[offset];
}

void* AssetManager::GetIndexPointer(int offset) {
	return &_indices[offset];
}

uint32_t AssetManager::GetIndex(int offset) {
	return _indices[offset];
}

std::vector<Vertex>& AssetManager::GetVertices_TEMPORARY() {
	return _vertices;
}

std::vector<uint32_t>& AssetManager::GetIndices_TEMPORARY() {
	return _indices;
}

std::vector<Mesh>& AssetManager::GetMeshList() {
	return _meshes;
}

int AssetManager::CreateMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {

	Mesh& mesh = _meshes.emplace_back(Mesh());
	mesh._vertexCount = vertices.size();
	mesh._indexCount = indices.size();
	mesh._vertexOffset = _vertexOffset;
	mesh._indexOffset = _indexOffset;

	for (int i = 0; i < vertices.size(); i++)
		_vertices.push_back(vertices[i]);
	for (int i = 0; i < indices.size(); i++)
		_indices.push_back(indices[i]);

	/*for (int i = 0; i < vertices.size(); i++)
		_vertices[_vertexOffset + i] = vertices[i];
	for (int i = 0; i < indices.size(); i++)
		_indices[_indexOffset + i] = indices[i];*/

	_vertexOffset += vertices.size();
	_indexOffset += indices.size();
	return _meshes.size() - 1;
}


/*
int AssetManager::CreateModel(std::vector<int> meshIndices) {

}*/

Mesh* AssetManager::GetMesh(int index) {
	if (index >= 0 && index < _meshes.size())
		return &_meshes[index];
	else
		return nullptr;
}

bool AssetManager::load_image_from_file(VulkanEngine& engine, const char* file, Texture& outTexture, VkFormat imageFormat, bool generateMips)
{

	stbi_uc* pixels = stbi_load(file, &outTexture._width, &outTexture._height, &outTexture._channelCount, STBI_rgb_alpha);

	if (!pixels) {
		std::cout << "Failed to load texture file " << file << std::endl;
		return false;
	}
	//std::cout << "Loading " << file << " " << outTexture._channelCount << " channel" << std::endl;

	VkDeviceSize imageSize = outTexture._width * outTexture._height * 4;
	AllocatedBuffer stagingBuffer = engine.create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	void* data;
	vmaMapMemory(engine._allocator, stagingBuffer._allocation, &data);
	memcpy(data, (void*)pixels, static_cast<size_t>(imageSize));
	vmaUnmapMemory(engine._allocator, stagingBuffer._allocation);
	stbi_image_free(pixels);

	VkExtent3D imageExtent;
	imageExtent.width = static_cast<uint32_t>(outTexture._width);
	imageExtent.height = static_cast<uint32_t>(outTexture._height);
	imageExtent.depth = 1;

	if (generateMips) {
		outTexture._mipLevels = floor(log2(std::max(outTexture._width, outTexture._height))) + 1;
	}
	else {
		outTexture._mipLevels = 1;
	}

	VkImageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.format = imageFormat;
	createInfo.extent = imageExtent;
	createInfo.mipLevels = outTexture._mipLevels;
	createInfo.arrayLayers = 1;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	AllocatedImage newImage;

	VmaAllocationCreateInfo dimg_allocinfo = {};
	dimg_allocinfo.usage = VMA_MEMORY_USAGE_AUTO;

	vmaCreateImage(engine._allocator, &createInfo, &dimg_allocinfo, &newImage._image, &newImage._allocation, nullptr);

	//transition image to transfer-receiver	
	engine.immediate_submit([&](VkCommandBuffer cmd)
		{
			VkImageSubresourceRange range;
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseMipLevel = 0;
			range.levelCount = 1;
			range.baseArrayLayer = 0;
			range.layerCount = 1;
			VkImageMemoryBarrier imageBarrier_toTransfer = {};
			imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrier_toTransfer.image = newImage._image;
			imageBarrier_toTransfer.subresourceRange = range;
			imageBarrier_toTransfer.srcAccessMask = 0;
			imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;
			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent = imageExtent;

			// First 1:1 copy for mip level 1
			vkCmdCopyBufferToImage(cmd, stagingBuffer._buffer, newImage._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

			// Prepare that image to be read from
			VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
			imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageBarrier_toReadable.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

			// Now walk the mip chain and copy down mips from n-1 to n
			for (int32_t i = 1; i < outTexture._mipLevels; i++)
			{
				VkImageBlit imageBlit{};

				// Source
				imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.srcSubresource.layerCount = 1;
				imageBlit.srcSubresource.mipLevel = i - 1;
				imageBlit.srcOffsets[1].x = int32_t(outTexture._width >> (i - 1));
				imageBlit.srcOffsets[1].y = int32_t(outTexture._height >> (i - 1));
				imageBlit.srcOffsets[1].z = 1;

				// Destination
				imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.dstSubresource.layerCount = 1;
				imageBlit.dstSubresource.mipLevel = i;
				imageBlit.dstOffsets[1].x = int32_t(outTexture._width >> i);
				imageBlit.dstOffsets[1].y = int32_t(outTexture._height >> i);
				imageBlit.dstOffsets[1].z = 1;

				VkImageSubresourceRange mipSubRange = {};
				mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				mipSubRange.baseMipLevel = i;
				mipSubRange.levelCount = 1;
				mipSubRange.layerCount = 1;

				// Prepare current mip level as image blit destination
				VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
				imageBarrier_toReadable.image = newImage._image;
				imageBarrier_toReadable.subresourceRange = mipSubRange;
				imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageBarrier_toReadable.srcAccessMask = 0;
				imageBarrier_toReadable.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

				// Blit from previous level
				vkCmdBlitImage(cmd, newImage._image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, newImage._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

				// Prepare current mip level as image blit source for next level
				VkImageMemoryBarrier barrier2 = imageBarrier_toTransfer;
				barrier2.image = newImage._image;
				barrier2.subresourceRange = mipSubRange;
				barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier2.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier2.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier2);
			}

			// After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.levelCount = outTexture._mipLevels;
			subresourceRange.layerCount = 1;

			// Prepare current mip level as image blit source for next level
			VkImageMemoryBarrier barrier = imageBarrier_toTransfer;
			barrier.image = newImage._image;
			barrier.subresourceRange = subresourceRange;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		});

	vmaDestroyBuffer(engine._allocator, stagingBuffer._buffer, stagingBuffer._allocation);

	outTexture.image = newImage;

	// Image view  
	VkImageViewCreateInfo imageinfo = vkinit::imageview_create_info(imageFormat, outTexture.image._image, VK_IMAGE_ASPECT_COLOR_BIT);
	imageinfo.subresourceRange.levelCount = outTexture._mipLevels;

	vkCreateImageView(engine._device, &imageinfo, nullptr, &outTexture.imageView);

	// isolate name
	std::string filepath = file;
	std::string filename = filepath.substr(filepath.rfind("/") + 1);
	outTexture._filename = filename.substr(0, filename.length() - 4);

	//std::cout << filename << " has " << outTexture._mipLevels << "mips\n";

	return true;
}

Texture* AssetManager::GetTexture(int index) {
	return &_textures[index];
}

Texture* AssetManager::GetTexture(const std::string& filename) {
	for (Texture& texture : _textures) {
		if (texture._filename == filename)
			return &texture;
	}
	return nullptr;
}

int AssetManager::GetTextureIndex(const std::string& filename) {
	for (int i = 0; i < _textures.size(); i++) {
		if (_textures[i]._filename == filename) {
			return i;
		}
	}
	std::cout << "Could not get texture with name \"" << filename << "\", it does not exist\n";
	return -1;
}

int AssetManager::GetMaterialIndex(const std::string& _name) {
	for (int i = 0; i < _materials.size(); i++) {
		if (_materials[i]._name == _name) {
			return i;
		}
	}
	std::cout << "Could not get material with name \"" << _name << "\", it does not exist\n";
	return -1;
}

int AssetManager::GetNumberOfTextures() {
	return _textures.size();
}

void AssetManager::AddTexture(Texture& texture) {
	_textures.push_back(texture);
}

/*void AssetManager::AddModel(Model& model) {
	_models.push_back(model);
}*/



bool AssetManager::TextureExists(const std::string& filename) {
	for (Texture& texture : _textures)
		if (texture._filename == filename) 
			return true;
	return false;
}

void AssetManager::LoadTextures(VulkanEngine& engine) {
	for (const auto& entry : std::filesystem::directory_iterator("res/textures/")) {
		FileInfo info = Util::GetFileInfo(entry);
		if (info.filetype == "png" || info.filetype == "tga" || info.filetype == "jpg") {
			if (!TextureExists(info.filename)) {
				Texture texture;
				VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;// VK_FORMAT_R8G8B8A8_SRGB;
				if (info.materialType == "ALB") {
					imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
				}
				load_image_from_file(engine, info.fullpath.c_str(), texture, imageFormat, true);
				AddTexture(texture);
			}
		}
	}
}

void AssetManager::BuildMaterials() {
	for (auto& texture : _textures) {
		if (texture._filename.substr(texture._filename.length() - 3) == "ALB") {
			Material& material = _materials.emplace_back(Material());
			material._name = texture._filename.substr(0, texture._filename.length() - 4);
			material._basecolor = GetTextureIndex(material._name + "_ALB");
			material._normal = GetTextureIndex(material._name + "_NRM");
			material._rma = GetTextureIndex(material._name + "_RMA");
		}
	}
}


void AssetManager::LoadAssets() {

	{
		// Quad for text blitting
		Vertex vertA, vertB, vertC, vertD;
		vertA.position = { -1.0f, -1.0f, 0.0f };
		vertB.position = { -1.0f, 1.0f, 0.0f };
		vertC.position = { 1.0f,  1.0f, 0.0f };
		vertD.position = { 1.0f,  -1.0f, 0.0f };
		vertA.uv = { 0.0f, 1.0f };
		vertB.uv = { 0.0f, 0.0f };
		vertC.uv = { 1.0f, 0.0f };
		vertD.uv = { 1.0f, 1.0f };
		std::vector<Vertex> vertices;
		vertices.push_back(vertA);
		vertices.push_back(vertB);
		vertices.push_back(vertC);
		vertices.push_back(vertD);
		std::vector<uint32_t> indices = { 0, 1, 2, 0, 2, 3 };

		Model model;
		model._meshIndices.push_back(CreateMesh(vertices, indices));
		_models["blitter_quad"] = model;
	}

	_models["chair"] = Model("res/models/FallenChair.obj");
	_models["wife"] = Model("res/models/Wife.obj");
	_models["door"] = Model("res/models/Door.obj");
	_models["skull"] = Model("res/models/BlackSkull.obj");
	_models["door_frame"] = Model("res/models/DoorFrame.obj");
	_models["trims_ceiling"] = Model("res/models/TrimCeiling.obj");
	_models["flowers"] = Model("res/models/Flowers.obj");
	_models["vase"] = Model("res/models/Vase.obj");
	//_models["chest_of_drawers"] = Model("res/models/ChestOfDrawers.obj");
	_models["light_switch"] = Model("res/models/LightSwitchOn.obj");

	_models["DrawerFrame"] = Model("res/models/DrawerFrame.obj");
	_models["DrawerTopLeft"] = Model("res/models/DrawerTopLeft.obj");
	_models["DrawerTopRight"] = Model("res/models/DrawerTopRight.obj");
	_models["DrawerSecond"] = Model("res/models/DrawerSecond.obj");
	_models["DrawerThird"] = Model("res/models/DrawerThird.obj");
	_models["DrawerFourth"] = Model("res/models/DrawerFourth.obj");
	_models["Diary"] = Model("res/models/Diary.obj");


	{
		// Floor 
		Vertex vert0, vert1, vert2, vert3;
		float size = 10;
		float texSize = 10;
		vert0.position = { -size, 0, -size };
		vert1.position = { -size, 0, size };
		vert2.position = { size, 0, size };
		vert3.position = { size, 0, -size };
		vert0.uv = { 0, texSize };
		vert1.uv = { 0, 0 };
		vert2.uv = { texSize, texSize };
		vert3.uv = { texSize, 0 };
		vert0.normal = { 0, 1, 0 };
		vert1.normal = { 0, 1, 0 };
		vert2.normal = { 0, 1, 0 };
		vert3.normal = { 0, 1, 0 };
		std::vector<Vertex> vertices;
		vertices.push_back(vert0);
		vertices.push_back(vert1);
		vertices.push_back(vert2);
		vertices.push_back(vert3);
		std::vector<uint32_t> indices = { 2, 1, 0, 3, 2, 0 };
		Util::SetTangentsFromVertices(vertices, indices);
		Model model;
		model._meshIndices.push_back(CreateMesh(vertices, indices));
		model._filename = "Floor";
		_models["floor"] = model;
	} 
	{
		float Zmin = -1.8f;
		float Zmax = 1.8f;
		float Xmin = -2.75f;
		float Xmax = 1.6f;
		// bedroom ceiling 
		Vertex vert0, vert1, vert2, vert3;
		float size = 10;
		float texScale = 3;
		vert0.position = { Xmin, CEILING_HEIGHT, Zmin };
		vert1.position = { Xmin, CEILING_HEIGHT, Zmax };
		vert2.position = { Xmax, CEILING_HEIGHT, Zmax };
		vert3.position = { Xmax, CEILING_HEIGHT, Zmin };
		vert0.uv = { 0, fmod(2.0, 1.0) };
		vert1.uv = { 0, 0 };
		vert2.uv = { fmod(2.0, 1.0), fmod(2.0, 1.0) };
		vert3.uv = { fmod(2.0, 1.0), 0 };
		vert0.uv = { Xmin / texScale, Zmax / texScale };
		vert1.uv = { Xmin / texScale, Zmin / texScale };
		vert2.uv = { Xmax / texScale, Zmin / texScale };
		vert3.uv = { Xmax / texScale, Zmax / texScale };
		vert0.normal = { 0, 1, 0 };
		vert1.normal = { 0, 1, 0 };
		vert2.normal = { 0, 1, 0 };
		vert3.normal = { 0, 1, 0 };
		std::vector<Vertex> vertices;
		vertices.push_back(vert0);
		vertices.push_back(vert1);
		vertices.push_back(vert2);
		vertices.push_back(vert3);
		std::vector<uint32_t> indices = { 2, 1, 0, 3, 2, 0 };
		Util::SetTangentsFromVertices(vertices, indices);
		Model model;
		model._meshIndices.push_back(CreateMesh(vertices, indices));
		_models["ceiling"] = model;
	}
}


Model* AssetManager::GetModel(const std::string & name) {
	auto it = _models.find(name);
	if (it == _models.end()) {
		return nullptr;
	}
	else {
		return &(*it).second;
	}
}

Material* AssetManager::GetMaterial(const std::string& name) {
	for (int i = 0; i < _materials.size(); i++) {
		if (_materials[i]._name == name) {
			return &_materials[i];
		}
	}
	std::cout << "Could not get material with name \"" << name << "\", it does not exist\n";
	return nullptr;
}

Material* AssetManager::GetMaterial(int index) {
	return &_materials[index];
}
