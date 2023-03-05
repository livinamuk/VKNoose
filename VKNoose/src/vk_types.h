#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <iostream>

#define VK_CHECK(x)                                             \
do                                                              \
{                                                               \
	VkResult err = x;                                           \
	if (err)                                                    \
	{                                                           \
		std::cout <<"Detected Vulkan error: " << err << "\n";	\
		abort();                                                \
	}                                                           \
} while (0)

struct AllocatedBuffer {
	VkBuffer _buffer;
	VmaAllocation _allocation;
};

struct AllocatedImage {
	VkImage _image;
	VmaAllocation _allocation;
	int _width;
	int _height;
};

typedef struct VulkanShaderStage {
	VkShaderModuleCreateInfo _createInfo;
	VkShaderModule _handle;
	VkPipelineShaderStageCreateInfo _shaderStageCreateInfo;
} VulkanShaderStage;

typedef struct VulkanPipeline {
	VkPipeline handle;
	VkPipelineLayout pipelineLayout;
} VulkanPipeline;

#define OBJECT_SHADER_STAGE_COUNT 2

struct Shader {
	VulkanShaderStage stages[OBJECT_SHADER_STAGE_COUNT];
	VulkanPipeline pipeline;
};