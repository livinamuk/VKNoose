#include "vk_shader.h"
#include "shaderc/shaderc.hpp"
#include "API/Vulkan/vk_backend.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <sstream>

static std::string ReadShaderFileWithIncludes(std::string filepath) {
    std::string line;
    std::ifstream stream(filepath);
    std::stringstream ss;

    if (!stream.is_open()) {
        std::cerr << "Could not open shader file: " << filepath << "\n";
        return "";
    }

    while (getline(stream, line)) {
        if (line.length() >= 8 && line.substr(0, 8) == "#include") {
            size_t firstQuote = line.find('"');
            size_t lastQuote = line.rfind('"');

            if (firstQuote != std::string::npos && lastQuote != std::string::npos && firstQuote != lastQuote) {
                std::string filename = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);

                std::string includePath = "res/shaders/Vulkan/" + filename;
                std::string includeContent = ReadShaderFileWithIncludes(includePath);
                ss << includeContent << "\n";
            }
        }
        else {
            ss << line << "\n";
        }
    }
    return ss.str();
}

static shaderc_shader_kind GetShadercKind(VkShaderStageFlagBits stage) {
    switch (stage) {
    case VK_SHADER_STAGE_VERTEX_BIT:                  return shaderc_vertex_shader;
    case VK_SHADER_STAGE_FRAGMENT_BIT:                return shaderc_fragment_shader;
    case VK_SHADER_STAGE_COMPUTE_BIT:                 return shaderc_compute_shader;
    case VK_SHADER_STAGE_GEOMETRY_BIT:                return shaderc_geometry_shader;
    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:    return shaderc_tess_control_shader;
    case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return shaderc_tess_evaluation_shader;
    case VK_SHADER_STAGE_RAYGEN_BIT_KHR:              return shaderc_raygen_shader;
    case VK_SHADER_STAGE_MISS_BIT_KHR:                return shaderc_miss_shader;
    case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:         return shaderc_closesthit_shader;
    default: return shaderc_glsl_infer_from_source;
    }
}

static std::vector<uint32_t> CompileGLSL(const std::string& path, VkShaderStageFlagBits stage) {
    // Use the manual include resolver instead of direct ifstream
    std::string source = ReadShaderFileWithIncludes(path);
    if (source.empty()) return {};

    static shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    options.SetTargetSpirv(shaderc_spirv_version_1_6);
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

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

        VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.objectType = VK_OBJECT_TYPE_SHADER_MODULE;
        nameInfo.objectHandle = (uint64_t)m_module;
        nameInfo.pObjectName = m_path.c_str();
        vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
    }
}

VulkanShader::VulkanShader(VkDevice device, const std::vector<std::string>& filenames) {
    static const std::unordered_map<std::string, VkShaderStageFlagBits> shaderTypeMap = {
        {".vert", VK_SHADER_STAGE_VERTEX_BIT},
        {".frag", VK_SHADER_STAGE_FRAGMENT_BIT},
        {".comp", VK_SHADER_STAGE_COMPUTE_BIT},
        {".rgen", VK_SHADER_STAGE_RAYGEN_BIT_KHR},
        {".rmiss", VK_SHADER_STAGE_MISS_BIT_KHR},
        {".rchit", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR}
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

//std::vector<VkPipelineShaderStageCreateInfo> VulkanShader::GetStageInfos() const {
//    std::vector<VkPipelineShaderStageCreateInfo> infos;
//    for (const auto& module : m_modules) {
//        VkPipelineShaderStageCreateInfo info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
//        info.stage = module.GetStage();
//        info.module = module.GetModule();
//        info.pName = m_entryPoint;
//        infos.push_back(info);
//    }
//    return infos;
//}

VkPipelineShaderStageCreateInfo VulkanShader::GetStageCreateInfo(VkShaderStageFlagBits stage) const {
    for (const auto& module : m_modules) {
        if (module.GetStage() == stage) {
            VkPipelineShaderStageCreateInfo info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
            info.stage = module.GetStage();
            info.module = module.GetModule();
            info.pName = m_entryPoint;
            return info;
        }
    }
    return {}; // Returns empty/null if stage not found
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