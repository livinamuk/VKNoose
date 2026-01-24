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
    VkDescriptorPool g_descriptorPool = VK_NULL_HANDLE;

    bool CreateAllocator();
    bool CreateDescriptorPool();

    bool Init() {
        if (!CreateAllocator())      return false;
        if (!CreateDescriptorPool()) return false;
        return true;
    }

    bool CreateAllocator() {
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

    bool CreateDescriptorPool() {
        VkDevice device = VulkanDeviceManager::GetDevice();

        // Create Pool
        std::vector<VkDescriptorPoolSize> sizes = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 8 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64 },
            { VK_DESCRIPTOR_TYPE_SAMPLER, 64 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 256 },
            { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 4 }
        };

        VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolInfo.maxSets = 20; // Increased slightly for safety
        poolInfo.poolSizeCount = (uint32_t)sizes.size();
        poolInfo.pPoolSizes = sizes.data();

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &g_descriptorPool) != VK_SUCCESS) {
            return false;
        }
        return true;
    }

    void Cleanup() {
        VkDevice device = VulkanDeviceManager::GetDevice();

        if (g_allocator != VK_NULL_HANDLE) {
            vmaDestroyAllocator(g_allocator);
            g_allocator = VK_NULL_HANDLE;
        }

        if (g_descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, g_descriptorPool, nullptr);
        }
        std::cout << "VulkanMemoryManager::Cleanup()\n";
    }

    VmaAllocator GetAllocator() { 
        return g_allocator; 
    }

    VkDescriptorPool GetDescriptorPool() { 
        return g_descriptorPool; 
    }
}