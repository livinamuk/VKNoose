#include "vk_descriptor_manager.h"

#include "API/Vulkan/vk_backend.h"
#include "API/Vulkan/Managers/vk_memory_manager.h"

#include <vector>
#include <iostream>

namespace VulkanDescriptorManager {

    // Sets
    HellDescriptorSet g_staticDescriptorSet;
    HellDescriptorSet g_samplerDescriptorSet;

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
            frame.dynamicDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
            frame.dynamicDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
            frame.dynamicDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, 1, VK_SHADER_STAGE_VERTEX_BIT);
            frame.dynamicDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
            frame.dynamicDescriptorSet.BuildSetLayout(device);
            frame.dynamicDescriptorSet.AllocateSet(device, descriptorPool);
            VulkanBackEnd::add_debug_name(frame.dynamicDescriptorSet.layout, "DynamicDescriptorSetLayout");

            // Inventory Set
            frame.dynamicDescriptorSetInventory.AddBinding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 0, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
            frame.dynamicDescriptorSetInventory.AddBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
            frame.dynamicDescriptorSetInventory.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
            frame.dynamicDescriptorSetInventory.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, 1, VK_SHADER_STAGE_VERTEX_BIT);
            frame.dynamicDescriptorSetInventory.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
            frame.dynamicDescriptorSetInventory.BuildSetLayout(device);
            frame.dynamicDescriptorSetInventory.AllocateSet(device, descriptorPool);
            VulkanBackEnd::add_debug_name(frame.dynamicDescriptorSetInventory.layout, "InventoryDescriptorSetLayout");
        }

        // Setup Global Sets (Static)
        g_staticDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_SAMPLER, 0, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT);
        g_staticDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, TEXTURE_ARRAY_SIZE, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT);
        g_staticDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        g_staticDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        g_staticDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        g_staticDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        g_staticDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 6, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        g_staticDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 7, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        g_staticDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 8, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        g_staticDescriptorSet.BuildSetLayout(device);
        g_staticDescriptorSet.AllocateSet(device, descriptorPool);
        VulkanBackEnd::add_debug_name(g_staticDescriptorSet.layout, "StaticDescriptorSetLayout");

        // Samplers Set
        g_samplerDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        g_samplerDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        g_samplerDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        g_samplerDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        g_samplerDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        g_samplerDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        g_samplerDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        g_samplerDescriptorSet.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 7, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        g_samplerDescriptorSet.BuildSetLayout(device);
        g_samplerDescriptorSet.AllocateSet(device, descriptorPool);
        VulkanBackEnd::add_debug_name(g_samplerDescriptorSet.layout, "SamplerDescriptorSetLayout");

        std::cout << "VulkanDescriptorManager::Init()\n";
        return true;
    }

    void Cleanup() {
        VkDevice device = VulkanBackEnd::GetDevice();

        g_staticDescriptorSet.Destroy(device);
        g_samplerDescriptorSet.Destroy(device);

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            g_frameData[i].dynamicDescriptorSet.Destroy(device);
            g_frameData[i].dynamicDescriptorSetInventory.Destroy(device);
        }
    }


    VkDescriptorSetLayout GetStaticSetLayout() { return g_staticDescriptorSet.layout; }
    VkDescriptorSetLayout GetDynamicSetLayout() { return g_frameData[0].dynamicDescriptorSet.layout; }
    VkDescriptorSetLayout GetSamplerSetLayout() { return g_samplerDescriptorSet.layout; }

    HellDescriptorSet& GetStaticDescriptorSet() { return g_staticDescriptorSet; }
    HellDescriptorSet& GetSamplerDescriptorSet() { return g_samplerDescriptorSet; }

    HellDescriptorSet& GetDynamicDescriptorSet(uint32_t frameIndex) {
        return g_frameData[frameIndex].dynamicDescriptorSet;
    }

    HellDescriptorSet& GetDynamicInventoryDescriptorSet(uint32_t frameIndex) {
        return g_frameData[frameIndex].dynamicDescriptorSetInventory;
    }
}