#pragma once

#include "API/Vulkan/Types/vk_buffer.h"
#include "Hell/VertexAttributes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

struct VulkanMeshBuffer {
    VulkanMeshBuffer() = default;
    VulkanMeshBuffer(const VulkanMeshBuffer&) = delete;
    VulkanMeshBuffer& operator=(const VulkanMeshBuffer&) = delete;
    VulkanMeshBuffer(VulkanMeshBuffer&&) noexcept = default;
    VulkanMeshBuffer& operator=(VulkanMeshBuffer&&) noexcept = default;

    void Init(const VertexLayoutDescription& layout);
    void Reset();
    void Cleanup();

    void InsertVertices(const std::vector<Vertex>& vertices, uint32_t insertOffset);
    void InsertIndices(const std::vector<uint32_t>& indices, uint32_t insertOffset);
    void PreAllocate(size_t vertexCapacity, size_t indexCapacity);
    void ResizeVertexBuffer(size_t newCapacity, const std::vector<Vertex>& vertices);
    void ResizeIndexBuffer(size_t newCapacity, const std::vector<uint32_t>& indices);

    void Bind(VkCommandBuffer commandBuffer) const;

    VulkanBuffer* GetVertexBuffer() { return m_vertexBuffer.GetBuffer() == VK_NULL_HANDLE ? nullptr : &m_vertexBuffer; }
    VulkanBuffer* GetIndexBuffer()  { return m_indexBuffer.GetBuffer() == VK_NULL_HANDLE ? nullptr : &m_indexBuffer; }
    const VulkanBuffer* GetVertexBuffer() const { return m_vertexBuffer.GetBuffer() == VK_NULL_HANDLE ? nullptr : &m_vertexBuffer; }
    const VulkanBuffer* GetIndexBuffer() const  { return m_indexBuffer.GetBuffer() == VK_NULL_HANDLE ? nullptr : &m_indexBuffer; }

    uint64_t GetVertexBufferAddress() const;
    uint64_t GetIndexBufferAddress() const;

private:
    VulkanBuffer CreateVertexBuffer(size_t vertexCapacity) const;
    VulkanBuffer CreateIndexBuffer(size_t indexCapacity) const;

    VulkanBuffer m_vertexBuffer;
    VulkanBuffer m_indexBuffer;
    size_t m_vertexStride = 0;
};
