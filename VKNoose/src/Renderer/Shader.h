#pragma once
#include "../vk_types.h"
#include <string>

//bool load_shader(VkDevice device, std::string filePath, VkShaderStageFlagBits flag, VkShaderModule* outShaderModule);
bool load_shader(VkDevice device, std::string filePath, VkShaderStageFlagBits flag, VkShaderModule* outShaderModule);
//bool load_ray_tracing_shader(VkDevice device, std::string filePath, VkShaderStageFlagBits flag, VkShaderModule* outShaderModule, VkRayTracingShaderGroupCreateInfoKHR createInfo);
/*
Shader create_shader(VkDevice* device, Shader* outShader);

void destroy_shader(VkDevice* device, Shader* shader);
void use_shader(VkDevice* device, Shader* shader);*/