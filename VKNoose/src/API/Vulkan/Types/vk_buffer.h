#pragma once

#include "API/Vulkan/vk_common.h"

struct VulkanBuffer {
    VulkanBuffer() = default;
    VulkanBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags vmaFlags = 0);

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    VulkanBuffer(VulkanBuffer&& other) noexcept;
    VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

    void Cleanup();

    void UploadData(const void* data, VkDeviceSize size);
    void Map(void** data);
    void Unmap();

    uint64_t GetDeviceAddress() const;
    VkDescriptorBufferInfo GetDescriptorInfo() const;

    VkBuffer GetBuffer() const   { return m_buffer; }
    VkDeviceSize GetSize() const { return m_size; }

private:
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VkDeviceSize m_size = 0;
    void* m_mappedPtr = nullptr;
};