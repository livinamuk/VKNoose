#include "vk_descriptor_set.h"

void VulkanDescriptorSet::Init(VkDescriptorSet handle) {
    m_handle = handle;
}

void VulkanDescriptorSet::WriteBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize range, VkDescriptorType type) {
    VkDescriptorBufferInfo info{};
    info.buffer = buffer;
    info.offset = 0;
    info.range = range;
    m_bufferInfos.push_back(info);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_handle;
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = nullptr; // Set in Update()
    m_writes.push_back(write);
}

void VulkanDescriptorSet::WriteImage(uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout layout, VkDescriptorType type) {
    VkDescriptorImageInfo info{};
    info.imageView = imageView;
    info.sampler = sampler;
    info.imageLayout = layout;
    m_imageInfos.push_back(info);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_handle;
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = nullptr; // Set in Update()
    m_writes.push_back(write);
}

void VulkanDescriptorSet::Update(VkDevice device) {
    if (m_writes.empty()) return;

    // Patch the pointers back to the vectors now that they are finished growing
    uint32_t bufferIdx = 0;
    uint32_t imageIdx = 0;

    for (auto& write : m_writes) {
        if (write.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || write.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
            write.pBufferInfo = &m_bufferInfos[bufferIdx++];
        }
        else {
            write.pImageInfo = &m_imageInfos[imageIdx++];
        }
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(m_writes.size()), m_writes.data(), 0, nullptr);

    // Clear the scratchpad for the next update
    m_writes.clear();
    m_bufferInfos.clear();
    m_imageInfos.clear();
}