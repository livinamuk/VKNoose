#include "vk_renderer.h"

#include "API/Vulkan/Managers/vk_device_manager.h"
#include "API/Vulkan/Managers/vk_memory_manager.h"
#include "API/Vulkan/Managers/vk_resource_manager.h"
#include "API/Vulkan/Renderer/vk_device_addresses.h"
#include "API/Vulkan/Renderer/vk_descriptor_indices.h"

#include "API/Vulkan/Managers/vk_raytracing_manager.h" // Needed?

#include "AssetManagement/Assetmanager.h"

#include "Hell/Core/Logging.h"
#include "Hell/Constants.h"
#include "Hell/Types.h"

namespace VulkanRenderer {

	VulkanFrameData g_frameData[FRAME_OVERLAP];
	uint32_t g_frameNumber = 0;
	uint64_t g_vertexBuffer = 0;
	uint64_t g_indexBuffer = 0;

    void UploadGlobalGeometry() {
		Logging::Init() << "UploadGlobalGeometry\n";

		const std::vector<Vertex>& vertices = AssetManager::GetVertices();
		const std::vector<uint32_t>& indices = AssetManager::GetIndices();

		// Define the usage flags once
		VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

		// Create and upload Vertex Buffer
		g_vertexBuffer = VulkanResourceManager::CreateBuffer(vertices.size() * sizeof(Vertex), usage, VMA_MEMORY_USAGE_GPU_ONLY);
		VulkanResourceManager::UploadBufferData(g_vertexBuffer, vertices.data(), vertices.size() * sizeof(Vertex));

		// Create and upload Index Buffer
		g_indexBuffer = VulkanResourceManager::CreateBuffer(indices.size() * sizeof(uint32_t), usage, VMA_MEMORY_USAGE_GPU_ONLY);
		VulkanResourceManager::UploadBufferData(g_indexBuffer, indices.data(), indices.size() * sizeof(uint32_t));
	}

	void BuildAllBLAS() {
		Logging::Init() << "BuildAllBLAS\n";

		for (Mesh& mesh : AssetManager::GetMeshes()) {
			mesh.m_vulkanAccelerationStructureId = VulkanRaytracingManager::CreateBottomLevelAS(&mesh);
		}
	}

	void CreateFrameData() {
		for (int i = 0; i < FRAME_OVERLAP; i++) {
			VulkanFrameData& frameData = g_frameData[i];
		
			VkBufferUsageFlags usageUniform = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
			VkBufferUsageFlags usageStorage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
			VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_AUTO;
			VmaAllocationCreateFlags vmaFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		
			// Buffers
			frameData.buffers.sceneCameraData = VulkanResourceManager::CreateBuffer(sizeof(CameraData), usageUniform, vmaUsage, vmaFlags);
			frameData.buffers.inventoryCameraData = VulkanResourceManager::CreateBuffer(sizeof(CameraData), usageUniform, vmaUsage, vmaFlags);
			frameData.buffers.uiInstances = VulkanResourceManager::CreateBuffer(sizeof(GPUObjectData2D) * MAX_RENDER_OBJECTS_2D, usageStorage, vmaUsage, vmaFlags);
			frameData.buffers.sceneInstances = VulkanResourceManager::CreateBuffer(sizeof(GPUObjectData) * MAX_RENDER_OBJECTS_2D, usageStorage, vmaUsage, vmaFlags);
			frameData.buffers.inventoryInstances = VulkanResourceManager::CreateBuffer(sizeof(GPUObjectData) * MAX_RENDER_OBJECTS_2D, usageStorage, vmaUsage, vmaFlags);
			frameData.buffers.sceneLights = VulkanResourceManager::CreateBuffer(sizeof(LightRenderInfo) * MAX_LIGHTS, usageStorage, vmaUsage, vmaFlags);
			frameData.buffers.inventoryLights = VulkanResourceManager::CreateBuffer(sizeof(LightRenderInfo) * 2, usageStorage, vmaUsage, vmaFlags);
		
			// TLAS
			frameData.tlas.scene = VulkanResourceManager::CreateAccelerationStructure();
			frameData.tlas.inventory = VulkanResourceManager::CreateAccelerationStructure();
		}
	}

	void Cleanup() {
		// Manually cleanup the BLAS for each mesh because they aren't stored in the ResourceManager
		for (Mesh& mesh : AssetManager::GetMeshes()) {

			// Or are they?
			
			//VulkanResourceManager::
			//mesh.m_vulkanAccelerationStructure.Cleanup();
		}
	}

	VulkanBuffer* GetVertexBuffer() {
		return VulkanResourceManager::GetBuffer(g_vertexBuffer);
	}

	VulkanBuffer* GetIndexBuffer() {
		return VulkanResourceManager::GetBuffer(g_indexBuffer);
	}

	uint64_t GetVertexBufferAddress() {
        if (VulkanBuffer* buffer = GetVertexBuffer()) {
            return buffer->GetDeviceAddress();
        }
        return 0;
	}

    uint64_t GetIndexBufferAddress() {
		if (VulkanBuffer* buffer = GetIndexBuffer()) {
			return buffer->GetDeviceAddress();
		}
        return 0;
	}

	VulkanFrameData& GetCurrentFrameData() {
		return g_frameData[g_frameNumber % FRAME_OVERLAP];
	}

	VulkanFrameData& GetFrameDataByIndex(uint32_t frameIndex) {
		return g_frameData[frameIndex]; // Warning: No bounds check. Remove this whole function when you can!
	}

	uint32_t GetCurrentFrameIndex() {
		return g_frameNumber % FRAME_OVERLAP;
	}

	void IncrementFrame() {
		g_frameNumber++;
	}
}