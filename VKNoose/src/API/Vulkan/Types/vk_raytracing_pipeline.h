#pragma once
#include "API/Vulkan/vk_common.h"
#include "API/Vulkan/Types/vk_buffer.h"

#include <string>
#include <vector>

struct VulkanShaderBindingTable{
    VulkanBuffer buffer;
    VkStridedDeviceAddressRegionKHR raygen{};
    VkStridedDeviceAddressRegionKHR miss{};
    VkStridedDeviceAddressRegionKHR hit{};
    VkStridedDeviceAddressRegionKHR callable{};
};

struct VulkanRaytracingPipeline {
    void AddRayGen(const std::string& shaderName);
    void AddMiss(const std::string& shaderName);
    void AddClosestHit(const std::string& shaderName);
    bool Build(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, const std::vector<VkPushConstantRange>& pushConstantRanges, uint32_t maxRecursionDepth);
    void Cleanup();

    VkPipeline GetHandle() const                                  { return m_handle; }
    VkPipelineLayout GetLayout() const                            { return m_layout; }
    const VulkanShaderBindingTable& GetShaderBindingTable() const { return m_shaderBindingTable; }

private:
    VkPipeline m_handle = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;

    std::vector<VkPipelineShaderStageCreateInfo> m_stages;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_groups;

    VulkanShaderBindingTable m_shaderBindingTable;

};