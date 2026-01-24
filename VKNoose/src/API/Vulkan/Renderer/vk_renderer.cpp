#include "vk_renderer.h"

#include "API/Vulkan/Managers/vk_device_manager.h"
#include "API/Vulkan/Managers/vk_resource_manager.h"

#include "AssetManagement/Assetmanager.h"

#include "Noose/Constants.h"
#include "Noose/Types.h"

namespace VulkanRenderer {
	void CreateDescriptorSets();
	void CreatePipelines();
	void CreateRenderTargets();
	void LoadShaders();

	bool Init() {
		LoadShaders();
		CreateRenderTargets();
		CreatePipelines();
		CreateDescriptorSets();
		return true;
	}

    void LoadShaders() {
		VulkanResourceManager::CreateShader("TextBlitter", { "vk_text_blitter.vert", "vk_text_blitter.frag" });
		VulkanResourceManager::CreateShader("SolidColor",  { "vk_solid_color.vert", "vk_solid_color.frag" });
		VulkanResourceManager::CreateShader("GBuffer",     { "vk_gbuffer.vert", "vk_gbuffer.frag" });
		VulkanResourceManager::CreateShader("Composite",   { "vk_composite.vert", "vk_composite.frag" });

		// Path Tracer Raytracing Shaders
		VulkanResourceManager::CreateShader("Path_RayGen", { "path_raygen.rgen" });
		VulkanResourceManager::CreateShader("Path_Miss", { "path_miss.rmiss" });
		VulkanResourceManager::CreateShader("Path_Shadow", { "path_shadow.rmiss" });
		VulkanResourceManager::CreateShader("Path_Hit", { "path_closesthit.rchit" });

		// Mouse Pick Raytracing Shaders
		VulkanResourceManager::CreateShader("Mouse_RayGen", { "mousepick_raygen.rgen" });
		VulkanResourceManager::CreateShader("Mouse_Miss", { "mousepick_miss.rmiss" });
		VulkanResourceManager::CreateShader("Mouse_Hit", { "mousepick_closesthit.rchit" });
    }

	void CreateRenderTargets() {
		// Present resoltion
		uint32_t width = PRESENT_WIDTH;
		uint32_t height = PRESENT_HEIGHT;

		// Raytrace resolution
		int scale = 2;
		uint32_t rtWidth = PRESENT_WIDTH * scale;
		uint32_t rtHeight = PRESENT_HEIGHT * scale;

		VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		VulkanResourceManager::CreateAllocatedImage("LoadingScreen", 1024, 576, VK_FORMAT_R8G8B8A8_UNORM, usage);

		// Raytracing Storage Images (Using R32G32B32A32_SFLOAT to match shader expectations)
		VulkanResourceManager::CreateAllocatedImage("RT_FirstHit_Color", rtWidth, rtHeight, VK_FORMAT_R32G32B32A32_SFLOAT, usage);
		VulkanResourceManager::CreateAllocatedImage("RT_FirstHit_Normals", rtWidth, rtHeight, VK_FORMAT_R32G32B32A32_SFLOAT, usage);
		VulkanResourceManager::CreateAllocatedImage("RT_FirstHit_BaseColor", rtWidth, rtHeight, VK_FORMAT_R32G32B32A32_SFLOAT, usage);
		VulkanResourceManager::CreateAllocatedImage("RT_SecondHit_Color", rtWidth, rtHeight, VK_FORMAT_R32G32B32A32_SFLOAT, usage);

		// GBuffer Targets
		VulkanResourceManager::CreateAllocatedImage("GBuffer_BaseColor", rtWidth, rtHeight, VK_FORMAT_R8G8B8A8_UNORM, usage);
		VulkanResourceManager::CreateAllocatedImage("GBuffer_Normal", rtWidth, rtHeight, VK_FORMAT_R8G8B8A8_UNORM, usage);
		VulkanResourceManager::CreateAllocatedImage("GBuffer_RMA", rtWidth, rtHeight, VK_FORMAT_R8G8B8A8_UNORM, usage);

		// UI and Display Targets
		VulkanResourceManager::CreateAllocatedImage("LaptopDisplay", LAPTOP_DISPLAY_WIDTH, LAPTOP_DISPLAY_HEIGHT, VK_FORMAT_R8G8B8A8_UNORM, usage);
		VulkanResourceManager::CreateAllocatedImage("Composite", rtWidth, rtHeight, VK_FORMAT_R8G8B8A8_UNORM, usage);
		VulkanResourceManager::CreateAllocatedImage("Present", width, height, VK_FORMAT_R8G8B8A8_UNORM, usage);

		// Depth Targets
		VkImageUsageFlags depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		VulkanResourceManager::CreateAllocatedImage("Depth_Present", width, height, VK_FORMAT_D32_SFLOAT, depthUsage);
		VulkanResourceManager::CreateAllocatedImage("Depth_GBuffer", rtWidth, rtHeight, VK_FORMAT_D32_SFLOAT, depthUsage);

	}

	void CreatePipelines() {
		VkDevice device = VulkanDeviceManager::GetDevice();

	}

	void CreateDescriptorSets() {

	}

	void UploadGlobalGeometry() {
		const std::vector<Vertex>& vertices = AssetManager::GetVertices();
		const std::vector<uint32_t>& indices = AssetManager::GetIndices();

		// Define the usage flags once
		VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

		// Create and upload Vertex Buffer
		VulkanResourceManager::CreateBuffer("Global_VertexBuffer", vertices.size() * sizeof(Vertex), usage, VMA_MEMORY_USAGE_GPU_ONLY);
		VulkanResourceManager::GetBuffer("Global_VertexBuffer")->UploadData(vertices.data(), vertices.size() * sizeof(Vertex));

		// Create and upload Index Buffer
		VulkanResourceManager::CreateBuffer("Global_IndexBuffer", indices.size() * sizeof(uint32_t), usage, VMA_MEMORY_USAGE_GPU_ONLY);
		VulkanResourceManager::GetBuffer("Global_IndexBuffer")->UploadData(indices.data(), indices.size() * sizeof(uint32_t));

		VkBufferUsageFlags transformUsage =
			VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
		
		glm::mat3x4 identity(1.0f);

		VulkanResourceManager::CreateBuffer("Global_TransformBuffer", sizeof(glm::mat3x4), transformUsage, VMA_MEMORY_USAGE_GPU_ONLY);
		VulkanResourceManager::GetBuffer("Global_TransformBuffer")->UploadData(&identity, sizeof(glm::mat3x4));
	}

	void BuildAllBLAS() {
		for (Mesh& mesh : AssetManager::GetMeshes()) {
			if (mesh.indexCount == 0) continue;
			//mesh.m_vulkanAccelerationStructure = VulkanRaytracingManager::CreateBottomLevelAS(&mesh);
		}
	}



	void Cleanup() {
		// Manually cleanup the BLAS for each mesh because they aren't stored in the ResourceManager
		for (Mesh& mesh : AssetManager::GetMeshes()) {
			mesh.m_vulkanAccelerationStructure.Cleanup(VulkanDeviceManager::GetDevice());
		}
	}

	VulkanBuffer* GetVertexBuffer() {
		return VulkanResourceManager::GetBuffer("Global_VertexBuffer");
	}

	VulkanBuffer* GetIndexBuffer() {
		return VulkanResourceManager::GetBuffer("Global_IndexBuffer");
	}
}