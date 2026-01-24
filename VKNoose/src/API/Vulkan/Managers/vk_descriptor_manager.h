#pragma once
#include "API/Vulkan/vk_common.h"
#include "API/Vulkan/vk_backend.h"

namespace VulkanDescriptorManager {
    bool Init();
    void Cleanup();

    // We will store the layouts here so pipelines can access them easily
    VkDescriptorSetLayout GetStaticSetLayout();
    VkDescriptorSetLayout GetDynamicSetLayout();
    VkDescriptorSetLayout GetSamplerSetLayout();

    // Global sets
    HellDescriptorSet& GetStaticDescriptorSet();
    HellDescriptorSet& GetSamplerDescriptorSet();

    // Per-frame sets
    HellDescriptorSet& GetDynamicDescriptorSet(uint32_t frameIndex);
    HellDescriptorSet& GetDynamicInventoryDescriptorSet(uint32_t frameIndex);
}