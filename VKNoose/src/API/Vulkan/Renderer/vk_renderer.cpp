#include "vk_renderer.h"

#include "API/Vulkan/Managers/vk_device_manager.h"
#include "API/Vulkan/Managers/vk_memory_manager.h"
#include "API/Vulkan/Managers/vk_raytracing_manager.h"
#include "API/Vulkan/Managers/vk_resource_manager.h"

#include "API/Vulkan/Renderer/vk_device_addresses.h"
#include "API/Vulkan/Renderer/vk_descriptor_indices.h"

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
        Logging::Init() << "VulkanRenderer::UploadGlobalGeometry()\n";

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