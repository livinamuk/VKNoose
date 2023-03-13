#include "Shader.h"
#include <fstream>
#include <sstream>
#include "shaderc/shaderc.hpp"

std::string read_file(std::string filepath)
{
	std::string line;
	std::ifstream stream(filepath);
	std::stringstream ss;

	while (getline(stream, line))
		ss << line << "\n";

	return ss.str();
}

// Compiles a shader to a SPIR-V binary. Returns the binary as a vector of 32-bit words.
std::vector<uint32_t> compile_file(const std::string& source_name, shaderc_shader_kind kind, const std::string& source,	bool optimize = false) 
{
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	options.SetTargetSpirv(shaderc_spirv_version::shaderc_spirv_version_1_6);
	options.SetTargetEnvironment(shaderc_target_env::shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
	options.SetForcedVersionProfile(460, shaderc_profile_core);

	if (optimize) 
		options.SetOptimizationLevel(shaderc_optimization_level_size);

	shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, kind, source_name.c_str(), options);

	if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
		std::cerr << module.GetErrorMessage();
		return std::vector<uint32_t>();
	}
	return { module.cbegin(), module.cend() };
}

bool load_shader(VkDevice device, std::string filePath, VkShaderStageFlagBits flag, VkShaderModule* outShaderModule)
{
	std::cout << "Loading: " << filePath << "\n";

	shaderc_shader_kind kind;
	if (flag == VK_SHADER_STAGE_VERTEX_BIT) {
		kind = shaderc_vertex_shader;
	}
	if (flag == VK_SHADER_STAGE_FRAGMENT_BIT) {
		kind = shaderc_fragment_shader;
	}
	if (flag == VK_SHADER_STAGE_RAYGEN_BIT_KHR) {
		kind = shaderc_raygen_shader;
	}
	if (flag == VK_SHADER_STAGE_MISS_BIT_KHR) {
		kind = shaderc_miss_shader;
	}
	if (flag == VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) {
		kind = shaderc_closesthit_shader;
	}
	std::string vertSource = read_file("res/shaders/" + filePath);

	std::vector<uint32_t> buffer = compile_file("shader_src", kind, vertSource, true);
	//std::cout << "Compiled to an optimized binary module with " << buffer.size() << " words." << std::endl;

	//create a new shader module, using the buffer we loaded
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;
	//codeSize has to be in bytes, so multply the ints in the buffer by size of int to know the real size of the buffer
	createInfo.codeSize = buffer.size() * sizeof(uint32_t);
	createInfo.pCode = buffer.data();

	//check that the creation goes well.
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		return false;
	}
	*outShaderModule = shaderModule;
	return true;
}

