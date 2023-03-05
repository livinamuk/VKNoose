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

	// Like -DMY_DEFINE=1
	options.AddMacroDefinition("MY_DEFINE", "1");
	if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

	shaderc::SpvCompilationResult module =
		compiler.CompileGlslToSpv(source, kind, source_name.c_str(), options);

	if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
		std::cerr << module.GetErrorMessage();
		return std::vector<uint32_t>();
	}

	return { module.cbegin(), module.cend() };
}
bool load_shader(VkDevice device, std::string filePath,	VkShaderStageFlagBits flag, VkShaderModule* outShaderModule)
{
	shaderc_shader_kind kind;
	if (flag == VK_SHADER_STAGE_VERTEX_BIT) {
		kind = shaderc_vertex_shader;
	}
	if (flag == VK_SHADER_STAGE_FRAGMENT_BIT) {
		kind = shaderc_fragment_shader;
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




/*
Shader create_shader(VkDevice* device, Shader* outShader) {
	char stage_type_strs[OBJECT_SHADER_STAGE_COUNT][5] = { "vert", "frag" };
	VkShaderStageFlagBits stage_types[OBJECT_SHADER_STAGE_COUNT] = { VK_SHADER_STAGE_VERTEX_BIT,VK_SHADER_STAGE_FRAGMENT_BIT };

	for (int i = 0; i < OBJECT_SHADER_STAGE_COUNT; i++) {

	}
}

void destroy_shader(VkDevice* device, Shader* shader) {

}

void use_shader(VkDevice* device, Shader* shader) {

}*/