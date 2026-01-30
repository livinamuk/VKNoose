#include "vk_buffer.h"
#include "API/Vulkan/Managers/vk_device_manager.h"
#include "API/Vulkan/Managers/vk_memory_manager.h"
#include "API/Vulkan/Managers/vk_command_manager.h"
#include "Hell/Core/Logging.h"

VulkanBuffer::VulkanBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags vmaFlags) {
    m_size = size;

    VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaAllocInfo{};
    vmaAllocInfo.usage = memoryUsage;
    vmaAllocInfo.flags = vmaFlags;

    VmaAllocationInfo allocInfo; // Capture this to get the pointer
    vmaCreateBuffer(VulkanMemoryManager::GetAllocator(), &bufferInfo, &vmaAllocInfo, &m_buffer, &m_allocation, &allocInfo);

    // If you requested the buffer to be mapped at creation, store the pointer
    if (vmaFlags & VMA_ALLOCATION_CREATE_MAPPED_BIT) {
        m_mappedPtr = allocInfo.pMappedData;
    }
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

void VulkanBuffer::UpdateData(const void* data, VkDeviceSize size) {
    if (size == 0) return;

    // Use the direct mapping path if the pointer exists
    if (m_mappedPtr) {
        memcpy(m_mappedPtr, data, size);

        // Ensure changes are visible to the GPU if memory is not coherent
        // Most desktop GPUs are coherent, but this keeps it portable
        VmaAllocationInfo allocInfo;
        vmaGetAllocationInfo(VulkanMemoryManager::GetAllocator(), m_allocation, &allocInfo);

        VkMemoryPropertyFlags memFlags;
        vmaGetMemoryTypeProperties(VulkanMemoryManager::GetAllocator(), allocInfo.memoryType, &memFlags);

        if (!(memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            vmaFlushAllocation(VulkanMemoryManager::GetAllocator(), m_allocation, 0, size);
        }
    }
    else {
        Logging::Error() << "VulkanBuffer::UpdateData(..) failed because it was called on a buffer with no mapped pointer!\n"
                         << "Ensure the buffer was created with VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT and VMA_ALLOCATION_CREATE_MAPPED_BIT.\n";
    }
}

void VulkanBuffer::UploadData(const void* data, VkDeviceSize size) {
    // Only use this for static/GPU_ONLY buffers
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

    // Copy to the temporary staging memory
    memcpy(stagingResult.pMappedData, data, size);

    // Execute the transfer command
    VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
        VkBufferCopy copyRegion{ 0, 0, size };
        vkCmdCopyBuffer(cmd, stagingBuffer, m_buffer, 1, &copyRegion);

        // Add a barrier if this buffer is used immediately after in a build or compute task
        VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        barrier.buffer = m_buffer;
        barrier.offset = 0;
        barrier.size = size;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
        });

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