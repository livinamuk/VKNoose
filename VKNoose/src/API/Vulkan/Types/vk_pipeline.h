#pragma once
#include "API/Vulkan/vk_common.h"
#include "API/Vulkan/Types/vk_vertex_descriptions.h"
#include "Noose/Types.h"

#include <vector>
#include <string>
#include <iostream>

enum struct VertexDescriptionType {
    POSITION,
    POSITION_TEXCOORD,
    ALL
};

struct  VulkanPipeline {
    VulkanPipeline() = default;

    void PushDescriptorSetLayout(VkDescriptorSetLayout layout);
    void SetPushConstant(uint32_t size, VkShaderStageFlags stageFlags);
    void SetTopology(VkPrimitiveTopology topology);
    void SetPolygonMode(VkPolygonMode mode);
    void SetFrontFace(VkFrontFace frontFace);
    void SetCullMode(VkCullModeFlags cullMode);
    void SetColorBlending(bool enabled);
    void SetDepthTest(bool enabled, bool writeEnabled = true);

    template<typename T>
    void SetVertexDescription() {
        m_bindingDescription = VulkanVertexDescription<T>::GetBinding();
        m_attributeDescriptions = VulkanVertexDescription<T>::GetAttributes();
    }

    bool Build(VkDevice device, VkShaderModule vertShader, VkShaderModule fragShader, uint32_t colorAttachmentCount, VkFormat colorFormat);
    void Cleanup(VkDevice device);

    VkPipeline GetHandle() const { return m_handle; }
    VkPipelineLayout GetLayout() const { return m_layout; }

private:
    bool CheckResult(VkResult result, const std::string& message);

    VkPipeline m_handle = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;

    std::vector<VkDescriptorSetLayout> m_descriptorLayouts;
    std::vector<VkPushConstantRange> m_pushConstants;
    VkPrimitiveTopology m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPolygonMode m_polygonMode = VK_POLYGON_MODE_FILL;
    VkCullModeFlags m_cullMode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace m_frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    bool m_colorBlending = false;
    bool m_depthTest = true;
    bool m_depthWrite = true;

    VkVertexInputBindingDescription m_bindingDescription{};
    std::vector<VkVertexInputAttributeDescription> m_attributeDescriptions;
};