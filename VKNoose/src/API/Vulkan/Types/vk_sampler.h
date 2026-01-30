#pragma once
#include "API/Vulkan/vk_common.h"

struct VulkanSampler {
    VulkanSampler() = default;
    VulkanSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode, float maxAnisotropy = 1.0f);
    VulkanSampler(const VulkanSampler&) = delete;
    VulkanSampler& operator=(const VulkanSampler&) = delete;
    VulkanSampler(VulkanSampler&& other) noexcept;
    VulkanSampler& operator=(VulkanSampler&& other) noexcept;

    void Cleanup();
    
    VkDescriptorImageInfo GetDescriptorInfo() const;

    VkSampler GetSampler() const { return m_sampler; }

private:
    VkSampler m_sampler = VK_NULL_HANDLE;
};