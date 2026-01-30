#include "vk_descriptor_set.h"
#include "API/Vulkan/vk_backend.h"
#include "API/Vulkan/Managers/vk_memory_manager.h"

VulkanDescriptorSet::VulkanDescriptorSet(VkDescriptorSetLayoutCreateInfo layoutInfo) {
    VkDevice device = VulkanBackEnd::GetDevice();
    VkDescriptorPool pool = VulkanMemoryManager::GetDescriptorPool();

    // Create the layout using the bindings already pointed to in layoutInfo
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_layout);

    // Allocate the set
    VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_layout;

    vkAllocateDescriptorSets(device, &allocInfo, &m_handle);
}

void VulkanDescriptorSet::Cleanup() {
    VkDevice device = VulkanBackEnd::GetDevice();

    if (m_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_layout, nullptr);
    }

    m_handle = VK_NULL_HANDLE;
    m_layout = VK_NULL_HANDLE;
    m_writes.clear();
    m_bufferInfos.clear();
    m_imageInfos.clear();
    m_asInfos.clear();
}

void VulkanDescriptorSet::WriteBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize range, VkDescriptorType type, uint32_t arrayElement) {
    VkDescriptorBufferInfo info{};
    info.buffer = buffer;
    info.offset = 0;
    info.range = range;
    m_bufferInfos.push_back(info);

    VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet = m_handle;
    write.dstBinding = binding;
    write.dstArrayElement = arrayElement;
    write.descriptorCount = 1;
    write.descriptorType = type;
    m_writes.push_back(write);
}

void VulkanDescriptorSet::WriteImage(uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout layout, VkDescriptorType type, uint32_t arrayElement) {
    VkDescriptorImageInfo info{};
    info.imageView = imageView;
    info.sampler = sampler;
    info.imageLayout = layout;
    m_imageInfos.push_back(info);

    VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet = m_handle;
    write.dstBinding = binding;
    write.dstArrayElement = arrayElement;
    write.descriptorCount = 1;
    write.descriptorType = type;
    m_writes.push_back(write);
}

void VulkanDescriptorSet::WriteAccelerationStructure(uint32_t binding, VkAccelerationStructureKHR tlas, uint32_t arrayElement) {
    VkWriteDescriptorSetAccelerationStructureKHR asInfo{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
    asInfo.accelerationStructureCount = 1;

    // Map the handle pointer for the pNext chain
    VkAccelerationStructureKHR* pAsHandle = new VkAccelerationStructureKHR(tlas);
    asInfo.pAccelerationStructures = pAsHandle;
    m_asInfos.push_back(asInfo);

    VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet = m_handle;
    write.dstBinding = binding;
    write.dstArrayElement = arrayElement;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    m_writes.push_back(write);
}

void VulkanDescriptorSet::Update() {
    if (m_writes.empty()) return;

    VkDevice device = VulkanBackEnd::GetDevice();
    uint32_t bufIdx = 0;
    uint32_t imgIdx = 0;
    uint32_t asIdx = 0;

    // Patch pointers to vectors now that they are finished growing
    for (auto& write : m_writes) {
        if (write.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || write.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
            write.pBufferInfo = &m_bufferInfos[bufIdx++];
        }
        else if (write.descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR) {
            write.pNext = &m_asInfos[asIdx++];
        }
        else {
            write.pImageInfo = &m_imageInfos[imgIdx++];
        }
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(m_writes.size()), m_writes.data(), 0, nullptr);

    // Clean up temporary handle allocations
    for (auto& asInfo : m_asInfos) {
        delete asInfo.pAccelerationStructures;
    }

    m_writes.clear();
    m_bufferInfos.clear();
    m_imageInfos.clear();
    m_asInfos.clear();
}