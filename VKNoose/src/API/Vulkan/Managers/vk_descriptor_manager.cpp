#include "vk_descriptor_manager.h"

#include "API/Vulkan/vk_backend.h"
#include "API/Vulkan/Managers/vk_memory_manager.h"
#include "API/Vulkan/Renderer/vk_descriptor_indices.h"
#include "API/Vulkan/Types/vk_descriptor_set.h"

#include <vector>
#include <iostream>

namespace VulkanDescriptorManager {

    struct FrameDescriptorData {
        VulkanDescriptorSet sceneTlasSet;
        VulkanDescriptorSet inventoryTlasSet;
    };
    FrameDescriptorData g_frameData[FRAME_OVERLAP];

    static VulkanDescriptorSet CreateTlasSet() {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = DESC_IDX_TLAS;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        binding.descriptorCount = 1;
        binding.stageFlags =
            VK_SHADER_STAGE_RAYGEN_BIT_KHR |
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        VkDescriptorSetLayoutCreateInfo layoutInfo{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
        };
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;

        return VulkanDescriptorSet(layoutInfo);
    }

    bool Init() {
        for (uint32_t i = 0; i < FRAME_OVERLAP; i++) {
            g_frameData[i].sceneTlasSet = CreateTlasSet();
            g_frameData[i].inventoryTlasSet = CreateTlasSet();
        }

        return true;
    }

    void Cleanup() {
        for (uint32_t i = 0; i < FRAME_OVERLAP; i++) {
            g_frameData[i].sceneTlasSet.Cleanup();
            g_frameData[i].inventoryTlasSet.Cleanup();
        }
    }

    VkDescriptorSetLayout GetTlasSetLayout() {
        return g_frameData[0].sceneTlasSet.GetLayout();
    }

    VulkanDescriptorSet& GetSceneTlasSet(uint32_t frameIndex) {
        return g_frameData[frameIndex].sceneTlasSet;
    }

    VulkanDescriptorSet& GetInventoryTlasSet(uint32_t frameIndex) {
        return g_frameData[frameIndex].inventoryTlasSet;
    }
}