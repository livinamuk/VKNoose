#pragma once
#include "API/Vulkan/vk_common.h"
#include <vector>

struct VulkanDescriptorSet {
public:
    VulkanDescriptorSet() = default;
    VulkanDescriptorSet(VkDescriptorSetLayoutCreateInfo layoutInfo);
    VulkanDescriptorSet(const VulkanDescriptorSet&) = delete;
    VulkanDescriptorSet& operator=(const VulkanDescriptorSet&) = delete;
    VulkanDescriptorSet(VulkanDescriptorSet&&) noexcept = default;
    VulkanDescriptorSet& operator=(VulkanDescriptorSet&&) noexcept = default;
    ~VulkanDescriptorSet() = default;

    void Cleanup();

    void WriteBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize range, VkDescriptorType type, uint32_t arrayElement = 0);
    void WriteImage(uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VkDescriptorType type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, uint32_t arrayElement = 0);
    void WriteAccelerationStructure(uint32_t binding, VkAccelerationStructureKHR tlas, uint32_t arrayElement = 0);

    void Update();

    VkDescriptorSetLayout GetLayout() const         { return m_layout; }
    VkDescriptorSet GetHandle() const               { return m_handle; }
    const VkDescriptorSet* GetHandlePtr() const     { return &m_handle; }

private:
    VkDescriptorSet m_handle = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;

    std::vector<VkWriteDescriptorSet> m_writes;
    std::vector<VkDescriptorBufferInfo> m_bufferInfos;
    std::vector<VkDescriptorImageInfo> m_imageInfos;
    std::vector<VkWriteDescriptorSetAccelerationStructureKHR> m_asInfos;
};