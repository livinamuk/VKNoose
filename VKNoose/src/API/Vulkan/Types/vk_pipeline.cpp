#include "vk_pipeline.h"
#include "API/Vulkan/Managers/vk_device_manager.h"
#include "API/Vulkan/Types/vk_allocated_image.h"
#include <array>
#include <limits>

namespace {
    VkFormat GetVertexAttributeFormat(const VertexAttribute& attribute) {
        if (attribute.normalized &&
            (attribute.type == VertexAttributeType::Int || attribute.type == VertexAttributeType::UnsignedInt)) {
            return VK_FORMAT_UNDEFINED;
        }

        switch (attribute.type) {
            case VertexAttributeType::Float:
                switch (attribute.componentCount) {
                    case 1: return VK_FORMAT_R32_SFLOAT;
                    case 2: return VK_FORMAT_R32G32_SFLOAT;
                    case 3: return VK_FORMAT_R32G32B32_SFLOAT;
                    case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
                }
                break;
            case VertexAttributeType::Int:
                switch (attribute.componentCount) {
                    case 1: return VK_FORMAT_R32_SINT;
                    case 2: return VK_FORMAT_R32G32_SINT;
                    case 3: return VK_FORMAT_R32G32B32_SINT;
                    case 4: return VK_FORMAT_R32G32B32A32_SINT;
                }
                break;
            case VertexAttributeType::UnsignedInt:
                switch (attribute.componentCount) {
                    case 1: return VK_FORMAT_R32_UINT;
                    case 2: return VK_FORMAT_R32G32_UINT;
                    case 3: return VK_FORMAT_R32G32B32_UINT;
                    case 4: return VK_FORMAT_R32G32B32A32_UINT;
                }
                break;
        }

        return VK_FORMAT_UNDEFINED;
    }
}

bool VulkanPipeline::CheckResult(VkResult result, const std::string& message) {
    if (result != VK_SUCCESS) {
        std::cerr << "[Vulkan Pipeline Error] " << message << " Result: " << result << "\n";
        return false;
    }
    return true;
}

void VulkanPipeline::AddColorAttachmentFormat(VkFormat format) {
    if (format == VK_FORMAT_UNDEFINED) {
        std::cerr << "[Vulkan Pipeline Error] Cannot add an undefined color attachment format\n";
        return;
    }

    m_colorAttachmentFormats.push_back(format);
}

void VulkanPipeline::AddColorAttachment(const AllocatedImage* image) {
    if (!image) {
        std::cerr << "[Vulkan Pipeline Error] Cannot add a null color attachment\n";
        return;
    }

    AddColorAttachmentFormat(image->GetFormat());
}

void VulkanPipeline::SetDepthAttachmentFormat(VkFormat format) {
    if (format == VK_FORMAT_UNDEFINED) {
        std::cerr << "[Vulkan Pipeline Error] Cannot set an undefined depth attachment format\n";
        return;
    }

    m_depthAttachmentFormat = format;
}

void VulkanPipeline::SetDepthAttachment(const AllocatedImage* image) {
    if (!image) {
        std::cerr << "[Vulkan Pipeline Error] Cannot set a null depth attachment\n";
        return;
    }

    SetDepthAttachmentFormat(image->GetFormat());
}

void VulkanPipeline::AddDescriptorSetLayout(VkDescriptorSetLayout layout) {
    m_descriptorLayouts.push_back(layout);
}

void VulkanPipeline::AddPushConstant(uint32_t size, VkShaderStageFlags stageFlags) {
    constexpr uint32_t alignment = 4;

    if (size == 0 || stageFlags == 0) {
        std::cerr << "[Vulkan Pipeline Error] Push constants require a non-zero size and shader stage flags\n";
        return;
    }

    const uint32_t alignedSize = (size + alignment - 1) & ~(alignment - 1);
    const uint32_t offset = m_pushConstants.empty() ? 0 : m_pushConstants.back().offset + m_pushConstants.back().size;

    VkPushConstantRange range{};
    range.offset = offset;
    range.size = alignedSize;
    range.stageFlags = stageFlags;
    m_pushConstants.push_back(range);
}

void VulkanPipeline::SetShader(const VulkanShader* shader) {
    m_shader = shader;
}

void VulkanPipeline::SetTopology(VkPrimitiveTopology topology) { 
    m_topology = topology; 
}

void VulkanPipeline::SetPolygonMode(VkPolygonMode mode) { 
    m_polygonMode = mode; 
}

void VulkanPipeline::SetFrontFace(VkFrontFace frontFace) {
    m_frontFace = frontFace;
}

void VulkanPipeline::SetCullMode(VkCullModeFlags cullMode) { 
    m_cullMode = cullMode; 
}

void VulkanPipeline::SetColorBlending(bool enabled) { 
    m_colorBlending = enabled; 
}

void VulkanPipeline::SetDepthTest(bool enabled, bool writeEnabled) {
    m_depthTest = enabled;
    m_depthWrite = writeEnabled;
}

void VulkanPipeline::SetVertexDescription(const VertexLayoutDescription& layout) {
    m_bindingDescription = {};
    m_attributeDescriptions.clear();

    if (layout.attributes.empty()) {
        return;
    }

    if (layout.stride > std::numeric_limits<uint32_t>::max()) {
        std::cerr << "[Vulkan Pipeline Error] Vertex stride exceeds Vulkan's uint32_t limit\n";
        return;
    }

    m_bindingDescription.binding = 0;
    m_bindingDescription.stride = static_cast<uint32_t>(layout.stride);
    m_bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    m_attributeDescriptions.reserve(layout.attributes.size());

    for (const VertexAttribute& attribute : layout.attributes) {
        const VkFormat format = GetVertexAttributeFormat(attribute);

        if (format == VK_FORMAT_UNDEFINED) {
            std::cerr << "[Vulkan Pipeline Error] Unsupported vertex attribute at location "
                      << attribute.location << "\n";
            m_bindingDescription = {};
            m_attributeDescriptions.clear();
            return;
        }

        if (attribute.offset > std::numeric_limits<uint32_t>::max()) {
            std::cerr << "[Vulkan Pipeline Error] Vertex attribute offset exceeds Vulkan's uint32_t limit\n";
            m_bindingDescription = {};
            m_attributeDescriptions.clear();
            return;
        }

        VkVertexInputAttributeDescription description{};
        description.binding = 0;
        description.location = attribute.location;
        description.format = format;
        description.offset = static_cast<uint32_t>(attribute.offset);
        m_attributeDescriptions.push_back(description);
    }
}

bool VulkanPipeline::Build() {
    if (!m_shader) {
        std::cerr << "[Vulkan Pipeline Error] Cannot build pipeline with a null shader\n";
        return false;
    }

    VkDevice device = VulkanDeviceManager::GetDevice();

    // Pipeline Layout
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = (uint32_t)m_descriptorLayouts.size();
    layoutInfo.pSetLayouts = m_descriptorLayouts.data();
    layoutInfo.pushConstantRangeCount = (uint32_t)m_pushConstants.size();
    layoutInfo.pPushConstantRanges = m_pushConstants.data();

    if (!CheckResult(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_layout), "Failed to create pipeline layout")) {
        return false;
    }

    // Shader Stages
    constexpr VkShaderStageFlags graphicsStageMask =
        VK_SHADER_STAGE_VERTEX_BIT |
        VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT |
        VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
        VK_SHADER_STAGE_GEOMETRY_BIT |
        VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    for (const VkPipelineShaderStageCreateInfo& stageInfo : m_shader->GetStageCreateInfos()) {
        if (stageInfo.stage & graphicsStageMask) {
            shaderStages.push_back(stageInfo);
        }
    }

    if (shaderStages.empty()) {
        std::cerr << "[Vulkan Pipeline Error] Shader contains no graphics shader stages\n";
        vkDestroyPipelineLayout(device, m_layout, nullptr);
        m_layout = VK_NULL_HANDLE;
        return false;
    }

    // Vertex Input
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    if (m_attributeDescriptions.size()) {
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &m_bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)m_attributeDescriptions.size();
        vertexInputInfo.pVertexAttributeDescriptions = m_attributeDescriptions.data();
    }

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = m_topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport & Scissor (Dynamic)
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = m_polygonMode;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = m_cullMode;
    rasterizer.frontFace = m_frontFace;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth Stencil
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = m_depthTest ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = m_depthWrite ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // Color Blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = m_colorBlending ? VK_TRUE : VK_FALSE;
    if (m_colorBlending) {
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(
        m_colorAttachmentFormats.size(),
        colorBlendAttachment
    );

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
    colorBlending.pAttachments = colorBlendAttachments.empty() ? nullptr : colorBlendAttachments.data();

    // Dynamic State
    std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = (uint32_t)dynamicStates.size();
    dynamicStateInfo.pDynamicStates = dynamicStates.data();

    // Dynamic Rendering Info
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = static_cast<uint32_t>(m_colorAttachmentFormats.size());
    renderingInfo.pColorAttachmentFormats = m_colorAttachmentFormats.empty() ? nullptr : m_colorAttachmentFormats.data();
    renderingInfo.depthAttachmentFormat = m_depthAttachmentFormat;

    // Final Creation
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = m_layout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;

    return CheckResult(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_handle), "Failed to create graphics pipeline");
}

void VulkanPipeline::Cleanup() {
    VkDevice device = VulkanDeviceManager::GetDevice();

    if (m_layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_layout, nullptr);
    if (m_handle != VK_NULL_HANDLE) vkDestroyPipeline(device, m_handle, nullptr);
    m_layout = VK_NULL_HANDLE;
    m_handle = VK_NULL_HANDLE;
}
