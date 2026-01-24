#pragma once
#include "API/Vulkan/vk_common.h"
#include "API/Vulkan/Types/vk_buffer.h"

struct VulkanAccelerationStructure {
	VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
	uint64_t deviceAddress = 0;
	VulkanBuffer buffer;

	void Cleanup(VkDevice device) {
		if (handle != VK_NULL_HANDLE) {
			vkDestroyAccelerationStructureKHR(device, handle, nullptr);
			handle = VK_NULL_HANDLE;
		}
		buffer.Cleanup();
		deviceAddress = 0;
	}
};