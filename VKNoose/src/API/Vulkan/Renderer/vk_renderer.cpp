#include "vk_renderer.h"

#include "API/Vulkan/Managers/vk_device_manager.h"
#include "API/Vulkan/Managers/vk_memory_manager.h"
#include "API/Vulkan/Managers/vk_raytracing_manager.h"
#include "API/Vulkan/Managers/vk_resource_manager.h"

#include "API/Vulkan/Renderer/vk_device_addresses.h"
#include "API/Vulkan/Renderer/vk_descriptor_indices.h"

#include "AssetManagement/Assetmanager.h"

#include "Hell/Logging.h"
#include "Hell/Constants.h"
#include "Hell/Types.h"
#include "Hell/VertexAttributes.h"
#include "ResourceManagement/ResourceManager.h"

namespace VulkanRenderer {
	VulkanFrameData g_frameData[FRAME_OVERLAP];
	uint32_t g_frameNumber = 0;

	VulkanMeshBuffer* GetStaticGeometryMeshBuffer() {
		MeshBuffer* meshBuffer = ResourceManager::GetMeshBufferPtr(ResourceManager::STATIC_GEOMETRY_MESH_BUFFER_NAME);
		if (!meshBuffer || meshBuffer->GetVulkanId() == 0) {
			return nullptr;
		}

		return VulkanResourceManager::GetMeshBuffer(meshBuffer->GetVulkanId());
	}

	VulkanBuffer* GetVertexBuffer() {
		VulkanMeshBuffer* meshBuffer = GetStaticGeometryMeshBuffer();
		return meshBuffer ? meshBuffer->GetVertexBuffer() : nullptr;
	}

	VulkanBuffer* GetIndexBuffer() {
		VulkanMeshBuffer* meshBuffer = GetStaticGeometryMeshBuffer();
		return meshBuffer ? meshBuffer->GetIndexBuffer() : nullptr;
	}

	uint64_t GetVertexBufferAddress() {
		VulkanMeshBuffer* meshBuffer = GetStaticGeometryMeshBuffer();
		return meshBuffer ? meshBuffer->GetVertexBufferAddress() : 0;
	}

    uint64_t GetIndexBufferAddress() {
		VulkanMeshBuffer* meshBuffer = GetStaticGeometryMeshBuffer();
		return meshBuffer ? meshBuffer->GetIndexBufferAddress() : 0;
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
