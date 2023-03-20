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
	//void* _mapped = nullptr; // used only by uploading uniform buffers RT function
};

struct AllocatedImage {
	VkImage _image;
	VmaAllocation _allocation;
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

struct AccelerationStructure {
	VkAccelerationStructureKHR handle;
	uint64_t deviceAddress = 0;
	VkDeviceMemory memory;
	AllocatedBuffer buffer;
};

struct Texture {
	AllocatedImage image;
	VkImageView imageView;
	int _width = 0;
	int _height = 0;
	int _channelCount = 0;
	uint32_t _mipLevels = 1;
	std::string _filename;
};