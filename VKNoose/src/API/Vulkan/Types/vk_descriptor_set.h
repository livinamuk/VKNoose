#pragma once
#include "API/Vulkan/vk_common.h"

#include <vector>
#include <string>
#include <map>

// Forward declarations
struct VulkanBuffer;
struct AllocatedImage;
struct Texture;

struct VulkanDescriptorSet {
public:
    VulkanDescriptorSet() = default;

    void Init(VkDescriptorSet handle);

    void WriteBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize range, VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    void WriteImage(uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VkDescriptorType type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    void WriteImageArray(uint32_t binding, const std::vector<VkDescriptorImageInfo>& imageInfos);

    void Update(VkDevice device);

    VkDescriptorSet GetHandle() const { return m_handle; }

private:
    VkDescriptorSet m_handle = VK_NULL_HANDLE;

    std::vector<VkWriteDescriptorSet> m_writes;
    std::vector<VkDescriptorBufferInfo> m_bufferInfos;
    std::vector<VkDescriptorImageInfo> m_imageInfos;
};