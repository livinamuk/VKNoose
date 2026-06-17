#include "vk_descriptor_manager.h"

#include "API/Vulkan/vk_backend.h"
#include "API/Vulkan/Managers/vk_memory_manager.h"

#include <vector>
#include <iostream>

namespace VulkanDescriptorManager {

    struct FrameDescriptorData {
        HellDescriptorSet dynamicDescriptorSet;
        HellDescriptorSet dynamicDescriptorSetInventory;
    };
    FrameDescriptorData g_frameData[FRAME_OVERLAP];

    bool Init() {
        VkDevice device = VulkanBackEnd::GetDevice();
        VkDescriptorPool descriptorPool = VulkanMemoryManager::GetDescriptorPool();

        // Setup Per-Frame Sets (Dynamic)
        for (int i = 0; i < FRAME_OVERLAP; i++) {
            auto& frame = g_frameData[i];

            // Dynamic Set
            frame.dynamicDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 0, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
            frame.dynamicDescriptorSet.BuildSetLayout(device);
            frame.dynamicDescriptorSet.AllocateSet(device, descriptorPool);
            VulkanBackEnd::add_debug_name(frame.dynamicDescriptorSet.layout, "DynamicDescriptorSetLayout");

            // Inventory Set
            frame.dynamicDescriptorSetInventory.AddBinding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 0, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
            frame.dynamicDescriptorSetInventory.BuildSetLayout(device);
            frame.dynamicDescriptorSetInventory.AllocateSet(device, descriptorPool);
            VulkanBackEnd::add_debug_name(frame.dynamicDescriptorSetInventory.layout, "InventoryDescriptorSetLayout");
        }

        std::cout << "VulkanDescriptorManager::Init()\n";
        return true;
    }

    void Cleanup() {
        VkDevice device = VulkanBackEnd::GetDevice();

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            g_frameData[i].dynamicDescriptorSet.Destroy(device);
            g_frameData[i].dynamicDescriptorSetInventory.Destroy(device);
        }
    }

    VkDescriptorSetLayout GetDynamicSetLayout() { return g_frameData[0].dynamicDescriptorSet.layout; }

    HellDescriptorSet& GetDynamicDescriptorSet(uint32_t frameIndex) {
        return g_frameData[frameIndex].dynamicDescriptorSet;
    }

    HellDescriptorSet& GetDynamicInventoryDescriptorSet(uint32_t frameIndex) {
        return g_frameData[frameIndex].dynamicDescriptorSetInventory;
    }
}