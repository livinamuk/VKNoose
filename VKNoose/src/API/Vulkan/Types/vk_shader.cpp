#include "vk_shader.h"
#include <fstream>
#include <iostream>

VulkanShaderModule::VulkanShaderModule(VkDevice device, const std::string& filename, VkShaderStageFlagBits stage) {
    m_stage = stage;
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filename << "\n";
        return;
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    createInfo.codeSize = buffer.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

    if (vkCreateShaderModule(device, &createInfo, nullptr, &m_module) != VK_SUCCESS) {
        std::cerr << "Failed to create shader module from: " << filename << "\n";
    }
}

VulkanShaderModule::VulkanShaderModule(VulkanShaderModule&& other) noexcept {
    m_module = other.m_module;
    m_stage = other.m_stage;
    other.m_module = VK_NULL_HANDLE;
}

VulkanShaderModule& VulkanShaderModule::operator=(VulkanShaderModule&& other) noexcept {
    if (this != &other) {
        m_module = other.m_module;
        m_stage = other.m_stage;
        other.m_module = VK_NULL_HANDLE;
    }
    return *this;
}

void VulkanShaderModule::Cleanup(VkDevice device) {
    if (m_module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, m_module, nullptr);
        m_module = VK_NULL_HANDLE;
    }
}

VulkanShader::VulkanShader(VulkanShader&& other) noexcept {
    m_modules = std::move(other.m_modules);
}

VulkanShader& VulkanShader::operator=(VulkanShader&& other) noexcept {
    if (this != &other) {
        m_modules = std::move(other.m_modules);
    }
    return *this;
}

void VulkanShader::AddStage(VulkanShaderModule&& module) {
    m_modules.push_back(std::move(module));
}

std::vector<VkPipelineShaderStageCreateInfo> VulkanShader::GetStageInfos() const {
    std::vector<VkPipelineShaderStageCreateInfo> infos;
    for (const auto& module : m_modules) {
        VkPipelineShaderStageCreateInfo info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        info.stage = module.GetStage();
        info.module = module.GetModule();
        info.pName = m_entryPoint;
        infos.push_back(info);
    }
    return infos;
}

void VulkanShader::Cleanup(VkDevice device) {
    for (auto& module : m_modules) {
        module.Cleanup(device);
    }
    m_modules.clear();
}