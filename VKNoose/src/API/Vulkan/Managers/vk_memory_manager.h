#pragma once
#include "API/Vulkan/vk_common.h"

namespace VulkanMemoryManager {
    bool Init();
    void Cleanup();

    VkDescriptorPool GetDescriptorPool();
    VmaAllocator GetAllocator();
}