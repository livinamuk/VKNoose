#pragma once
#include "API/Vulkan/vk_common.h"
#include "API/Vulkan/Types/vk_shader.h"
#include "Hell/VertexAttributes.h"

#include <vector>
#include <string>
#include <iostream>

struct AllocatedImage;

struct  VulkanPipeline {
    VulkanPipeline() = default;

    void AddColorAttachmentFormat(VkFormat format);
    void AddColorAttachment(const AllocatedImage* image);
    void SetDepthAttachmentFormat(VkFormat format);
    void SetDepthAttachment(const AllocatedImage* image);
    void AddDescriptorSetLayout(VkDescriptorSetLayout layout);
    void AddPushConstant(uint32_t size, VkShaderStageFlags stageFlags);
    void SetShader(const VulkanShader* shader);
    void SetTopology(VkPrimitiveTopology topology);
    void SetPolygonMode(VkPolygonMode mode);
    void SetFrontFace(VkFrontFace frontFace);
    void SetCullMode(VkCullModeFlags cullMode);
    void SetColorBlending(bool enabled);
    void SetDepthTest(bool enabled, bool writeEnabled = true);
    void SetVertexDescription(const VertexLayoutDescription& layout);

    template<typename T>
    void SetVertexDescription() {
        SetVertexDescription(T::GetLayout());
    }

    bool Build();
    void Cleanup();

    VkPipeline GetHandle() const { return m_handle; }
    VkPipelineLayout GetLayout() const { return m_layout; }

private:
    bool CheckResult(VkResult result, const std::string& message);

    VkPipeline m_handle = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;
    const VulkanShader* m_shader = nullptr;

    std::vector<VkFormat> m_colorAttachmentFormats;
    VkFormat m_depthAttachmentFormat = VK_FORMAT_UNDEFINED;
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
