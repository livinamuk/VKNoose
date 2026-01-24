#include "vk_buffer.h"
#include "API/Vulkan/Managers/vk_device_manager.h"
#include "API/Vulkan/Managers/vk_memory_manager.h"
#include "API/Vulkan/Managers/vk_command_manager.h"

VulkanBuffer::VulkanBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags vmaFlags) {
    m_size = size;

    VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaAllocInfo{};
    vmaAllocInfo.usage = memoryUsage;
    vmaAllocInfo.flags = vmaFlags;

    vmaCreateBuffer(VulkanMemoryManager::GetAllocator(), &bufferInfo, &vmaAllocInfo, &m_buffer, &m_allocation, nullptr);
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept {
    *this = std::move(other);
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_buffer = other.m_buffer;
        m_allocation = other.m_allocation;
        m_size = other.m_size;
        m_mappedPtr = other.m_mappedPtr;

        other.m_buffer = VK_NULL_HANDLE;
        other.m_allocation = VK_NULL_HANDLE;
        other.m_size = 0;
        other.m_mappedPtr = nullptr;
    }
    return *this;
}

void VulkanBuffer::Cleanup() {
    if (m_buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(VulkanMemoryManager::GetAllocator(), m_buffer, m_allocation);
        m_buffer = VK_NULL_HANDLE;
        m_allocation = VK_NULL_HANDLE;
        m_size = 0;
        m_mappedPtr = nullptr;
    }
}

void VulkanBuffer::UploadData(const void* data, VkDeviceSize size) {
    // Create a staging buffer
    VkBufferCreateInfo stagingInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    stagingInfo.size = size;
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer stagingBuffer;
    VmaAllocation stagingAlloc;
    VmaAllocationInfo stagingResult;
    vmaCreateBuffer(VulkanMemoryManager::GetAllocator(), &stagingInfo, &stagingAllocInfo, &stagingBuffer, &stagingAlloc, &stagingResult);

    // Copy CPU data to staging
    memcpy(stagingResult.pMappedData, data, size);

    // Record and submit the GPU copy
    VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
        VkBufferCopy copyRegion{ 0, 0, size };
        vkCmdCopyBuffer(cmd, stagingBuffer, m_buffer, 1, &copyRegion);
        });

    // Cleanup staging
    vmaDestroyBuffer(VulkanMemoryManager::GetAllocator(), stagingBuffer, stagingAlloc);
}

void VulkanBuffer::Map(void** data) {
    if (m_mappedPtr) {
        *data = m_mappedPtr;
    }
    else {
        vmaMapMemory(VulkanMemoryManager::GetAllocator(), m_allocation, data);
        m_mappedPtr = *data;
    }
}

void VulkanBuffer::Unmap() {
    if (m_mappedPtr) {
        vmaUnmapMemory(VulkanMemoryManager::GetAllocator(), m_allocation);
        m_mappedPtr = nullptr;
    }
}

uint64_t VulkanBuffer::GetDeviceAddress() const {
    VkBufferDeviceAddressInfo addressInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
    addressInfo.buffer = m_buffer;
    return vkGetBufferDeviceAddressKHR(VulkanDeviceManager::GetDevice(), &addressInfo);
}

VkDescriptorBufferInfo VulkanBuffer::GetDescriptorInfo() const {
    return VkDescriptorBufferInfo{ m_buffer, 0, m_size };
}