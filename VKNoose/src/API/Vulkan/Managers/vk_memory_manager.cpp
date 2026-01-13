#define VMA_IMPLEMENTATION
#include "API/Vulkan/vk_common.h"

#include "vk_memory_manager.h"
#include "vk_device_manager.h"
#include "vk_instance_manager.h"

#include <vector>
#include <set>
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace VulkanMemoryManager {
    VmaAllocator g_allocator = VK_NULL_HANDLE;

    bool Init() {
        VkInstance instance = VulkanInstanceManager::GetInstance();
        VkDevice device = VulkanDeviceManager::GetDevice();
        VkPhysicalDevice physicalDevice = VulkanDeviceManager::GetPhysicalDevice();

        VmaVulkanFunctions vulkanFunctions{};
        vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.instance = instance;
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.pVulkanFunctions = &vulkanFunctions;
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

        VkResult result = vmaCreateAllocator(&allocatorInfo, &g_allocator);
        return result == VK_SUCCESS;
    }

    void Cleanup() {

    }

    VmaAllocator GetAllocator() { return g_allocator; }
}