#include "vk_shader.h"
#include "shaderc/shaderc.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <sstream>

static shaderc_shader_kind GetShadercKind(VkShaderStageFlagBits stage) {
    switch (stage) {
    case VK_SHADER_STAGE_VERTEX_BIT:                  return shaderc_vertex_shader;
    case VK_SHADER_STAGE_FRAGMENT_BIT:                return shaderc_fragment_shader;
    case VK_SHADER_STAGE_COMPUTE_BIT:                 return shaderc_compute_shader;
    case VK_SHADER_STAGE_GEOMETRY_BIT:                return shaderc_geometry_shader;
    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:    return shaderc_tess_control_shader;
    case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return shaderc_tess_evaluation_shader;
    default: return shaderc_glsl_infer_from_source;
    }
}

static std::vector<uint32_t> CompileGLSL(const std::string& path, VkShaderStageFlagBits stage) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader source: " << path << "\n";
        return {};
    }

    std::stringstream stream;
    stream << file.rdbuf();
    std::string source = stream.str();

    static shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetSpirv(shaderc_spirv_version_1_6);
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);

    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source, GetShadercKind(stage), path.c_str(), options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << "ERROR IN: " << path << "\n" << result.GetErrorMessage();
        return {};
    }

    return { result.cbegin(), result.cend() };
}

VulkanShaderModule::VulkanShaderModule(VkDevice device, const std::string& filename, VkShaderStageFlagBits stage) {
    m_path = filename;
    m_stage = stage;
    Hotload(device);
}

VulkanShaderModule::VulkanShaderModule(VulkanShaderModule&& other) noexcept {
    m_module = other.m_module;
    m_stage = other.m_stage;
    m_path = std::move(other.m_path);
    other.m_module = VK_NULL_HANDLE;
}

VulkanShaderModule& VulkanShaderModule::operator=(VulkanShaderModule&& other) noexcept {
    if (this != &other) {
        m_module = other.m_module;
        m_stage = other.m_stage;
        m_path = std::move(other.m_path);
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

void VulkanShaderModule::Hotload(VkDevice device) {
    std::vector<uint32_t> spirv = CompileGLSL(m_path, m_stage);
    if (spirv.empty()) return;

    VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    createInfo.codeSize = spirv.size() * sizeof(uint32_t);
    createInfo.pCode = spirv.data();

    VkShaderModule newModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &newModule) == VK_SUCCESS) {
        Cleanup(device);
        m_module = newModule;
    }
}

VulkanShader::VulkanShader(VkDevice device, const std::vector<std::string>& filenames) {
    static const std::unordered_map<std::string, VkShaderStageFlagBits> shaderTypeMap = {
        {".vert", VK_SHADER_STAGE_VERTEX_BIT},
        {".frag", VK_SHADER_STAGE_FRAGMENT_BIT},
        {".geom", VK_SHADER_STAGE_GEOMETRY_BIT},
        {".tesc", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT},
        {".tese", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT},
        {".comp", VK_SHADER_STAGE_COMPUTE_BIT}
    };

    for (const std::string& filename : filenames) {
        std::string extension = std::filesystem::path(filename).extension().string();

        if (shaderTypeMap.contains(extension)) {
            std::string fullPath = "res/shaders/Vulkan/" + filename;

            if (std::filesystem::exists(fullPath)) {
                m_modules.emplace_back(VulkanShaderModule(device, fullPath, shaderTypeMap.at(extension)));
            }
            else {
                std::cerr << "SHADER FILE NOT FOUND: " << fullPath << "\n";
            }
        }
        else {
            std::cerr << "UNKNOWN SHADER EXTENSION: " << extension << " for file: " << filename << "\n";
        }
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

void VulkanShader::Hotload(VkDevice device) {
    for (auto& module : m_modules) {
        module.Hotload(device);
    }
}

VkShaderModule VulkanShader::GetVertexShader() {
    for (auto& module : m_modules) {
        if (module.GetStage() == VK_SHADER_STAGE_VERTEX_BIT) return module.GetModule();
    }
    return VK_NULL_HANDLE;
}

VkShaderModule VulkanShader::GetFragmentShader() {
    for (auto& module : m_modules) {
        if (module.GetStage() == VK_SHADER_STAGE_FRAGMENT_BIT) return module.GetModule();
    }
    return VK_NULL_HANDLE;
}

VkShaderModule VulkanShader::GetComputeShader() {
    for (auto& module : m_modules) {
        if (module.GetStage() == VK_SHADER_STAGE_COMPUTE_BIT) return module.GetModule();
    }
    return VK_NULL_HANDLE;
}

VkShaderModule VulkanShader::GetTesselationControlShader() {
    for (auto& module : m_modules) {
        if (module.GetStage() == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) return module.GetModule();
    }
    return VK_NULL_HANDLE;
}

VkShaderModule VulkanShader::GetTesselationEvaluationShader() {
    for (auto& module : m_modules) {
        if (module.GetStage() == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) return module.GetModule();
    }
    return VK_NULL_HANDLE;
}