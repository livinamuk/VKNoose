#pragma once
#include "API/Vulkan/Types/vk_descriptor_set.h"

namespace VulkanDescriptorManager {
    bool Init();
    void Cleanup();

    VkDescriptorSetLayout GetTlasSetLayout();

    VulkanDescriptorSet& GetSceneTlasSet(uint32_t frameIndex);
    VulkanDescriptorSet& GetInventoryTlasSet(uint32_t frameIndex);
}