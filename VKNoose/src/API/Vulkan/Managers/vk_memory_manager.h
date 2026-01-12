#pragma once
#include "API/Vulkan/vk_common.h"

namespace VulkanMemoryManager {
    bool Init(VkDevice device, VkPhysicalDevice physicalDevice);
    void Cleanup();

    VmaAllocator GetAllocator();
}