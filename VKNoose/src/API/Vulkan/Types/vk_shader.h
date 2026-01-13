#pragma once

#include "API/Vulkan/vk_common.h"
#include <string>
#include <vector>

struct VulkanShaderModule {
    VulkanShaderModule() = default;
    VulkanShaderModule(VkDevice device, const std::string& filename, VkShaderStageFlagBits stage);
    VulkanShaderModule(const VulkanShaderModule&) = delete;
    VulkanShaderModule& operator=(const VulkanShaderModule&) = delete;
    VulkanShaderModule(VulkanShaderModule&& other) noexcept;
    VulkanShaderModule& operator=(VulkanShaderModule&& other) noexcept;

    void Cleanup(VkDevice device);

    VkShaderModule GetModule() const { return m_module; }
    VkShaderStageFlagBits GetStage() const { return m_stage; }

private:
    VkShaderModule m_module = VK_NULL_HANDLE;
    VkShaderStageFlagBits m_stage = VK_SHADER_STAGE_ALL;
};

struct VulkanShader {
    VulkanShader() = default;
    VulkanShader(const VulkanShader&) = delete;
    VulkanShader& operator=(const VulkanShader&) = delete;
    VulkanShader(VulkanShader&& other) noexcept;
    VulkanShader& operator=(VulkanShader&& other) noexcept;

    //void AddStage(const std::string& path, VkShaderStageFlagBits stage);

    void AddStage(VulkanShaderModule&& module);
    std::vector<VkPipelineShaderStageCreateInfo> GetStageInfos() const;
    void Cleanup(VkDevice device);

private:
    std::vector<VulkanShaderModule> m_modules;
    const char* m_entryPoint = "main";
};