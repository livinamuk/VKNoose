#pragma once
#include "API/Vulkan/vk_common.h"
#include "API/Vulkan/vk_backend.h"

namespace VulkanDescriptorManager {
    bool Init();
    void Cleanup();

    // Store the layouts here so pipelines can access them easily
    VkDescriptorSetLayout GetDynamicSetLayout();

    // Per-frame sets
    HellDescriptorSet& GetDynamicDescriptorSet(uint32_t frameIndex);
    HellDescriptorSet& GetDynamicInventoryDescriptorSet(uint32_t frameIndex);
}