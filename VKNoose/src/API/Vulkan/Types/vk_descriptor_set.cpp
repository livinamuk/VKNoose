#include "vk_descriptor_set.h"

#include "API/Vulkan/Managers/vk_device_manager.h"
#include "API/Vulkan/Managers/vk_memory_manager.h"
#include "API/Vulkan/Renderer/vk_renderer.h"

VulkanDescriptorSet::VulkanDescriptorSet(VkDescriptorSetLayout layout) {
    VkDevice device = VulkanDeviceManager::GetDevice();
    VkDescriptorPool pool = VulkanMemoryManager::GetDescriptorPool();

    VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    vkAllocateDescriptorSets(device, &allocInfo, &m_handle);
}

void VulkanDescriptorSet::Cleanup() {
    for (VkWriteDescriptorSetAccelerationStructureKHR& asInfo : m_asInfos) {
        delete asInfo.pAccelerationStructures;
    }

    m_handle = VK_NULL_HANDLE;
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

void VulkanDescriptorSet::WriteAccelerationStructure(uint32_t binding, VkAccelerationStructureKHR accelerationStructure, uint32_t arrayElement) {
    VkWriteDescriptorSetAccelerationStructureKHR asInfo{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR
    };

    asInfo.accelerationStructureCount = 1;
    asInfo.pAccelerationStructures = new VkAccelerationStructureKHR(accelerationStructure);
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

    VkDevice device = VulkanDeviceManager::GetDevice();

    uint32_t bufferIndex = 0;
    uint32_t imageIndex = 0;
    uint32_t accelerationStructureIndex = 0;

    for (VkWriteDescriptorSet& write : m_writes) {
        switch (write.descriptorType) {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            write.pBufferInfo = &m_bufferInfos[bufferIndex++];
            break;

        case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
            write.pNext = &m_asInfos[accelerationStructureIndex++];
            break;

        default:
            write.pImageInfo = &m_imageInfos[imageIndex++];
            break;
        }
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(m_writes.size()), m_writes.data(), 0, nullptr);

    for (VkWriteDescriptorSetAccelerationStructureKHR& asInfo : m_asInfos) {
        delete asInfo.pAccelerationStructures;
    }

    m_writes.clear();
    m_bufferInfos.clear();
    m_imageInfos.clear();
    m_asInfos.clear();
}

VulkanDescriptorSetResource::VulkanDescriptorSetResource(VkDescriptorSetLayoutCreateInfo layoutInfo, DescriptorSetLifetime lifetime) {
    VkDevice device = VulkanDeviceManager::GetDevice();

    m_lifetime = lifetime;

    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_layout);

    uint32_t setCount = 1;

    if (m_lifetime == DescriptorSetLifetime::PER_FRAME) {
        setCount = FRAME_OVERLAP;
    }

    m_sets.reserve(setCount);

    for (uint32_t i = 0; i < setCount; i++) {
        m_sets.emplace_back(m_layout);
    }
}

void VulkanDescriptorSetResource::Cleanup() {
    VkDevice device = VulkanDeviceManager::GetDevice();

    for (VulkanDescriptorSet& set : m_sets) {
        set.Cleanup();
    }

    m_sets.clear();

    if (m_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_layout, nullptr);
        m_layout = VK_NULL_HANDLE;
    }
}

VulkanDescriptorSet& VulkanDescriptorSetResource::GetSet() {
    uint32_t setIndex = 0;

    if (m_lifetime == DescriptorSetLifetime::PER_FRAME) {
        setIndex = VulkanRenderer::GetCurrentFrameIndex();
    }

    return m_sets[setIndex];
}

const VulkanDescriptorSet& VulkanDescriptorSetResource::GetSet() const {
    uint32_t setIndex = 0;

    if (m_lifetime == DescriptorSetLifetime::PER_FRAME) {
        setIndex = VulkanRenderer::GetCurrentFrameIndex();
    }

    return m_sets[setIndex];
}