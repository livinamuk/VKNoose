#pragma once
#include "API/Vulkan/vk_common.h"

#include <iostream>
#include <vector>

inline void VK_CHECK(VkResult err) {
	if (err) {
		std::cout << "Detected Vulkan error: " << err << "\n";
		abort();                                               
	}
}

struct AllocatedBufferOLD {
	VkBuffer m_buffer = VK_NULL_HANDLE;
	VmaAllocation m_allocation = VK_NULL_HANDLE;
	void* m_mapped = nullptr;
};

struct HellDescriptorSet {
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	VkDescriptorSetLayout layout;
	VkDescriptorSet handle;
	void AddBinding(VkDescriptorType type, uint32_t binding, uint32_t descriptorCount, VkShaderStageFlags stageFlags);
	void AllocateSet(VkDevice device, VkDescriptorPool descriptorPool);
	void BuildSetLayout(VkDevice device);
	void Update(VkDevice device, uint32_t binding, uint32_t descriptorCount, VkDescriptorType type, VkBuffer buffer);
	void Update(VkDevice device, uint32_t binding, uint32_t descriptorCount, VkDescriptorType type, VkDescriptorImageInfo* imageInfo);
	void Update(VkDevice device, uint32_t binding, uint32_t descriptorCount, VkDescriptorType type, VkAccelerationStructureKHR* accelerationStructure);
	void Destroy(VkDevice device);
};

struct AllocatedImageOLD {
	VkImage _image;
	VmaAllocation _allocation;
};

struct Texture {
	AllocatedImageOLD image;
	VkImageView imageView;
	int _width = 0;
	int _height = 0;
	int _channelCount = 0;
	uint32_t _mipLevels = 1;
	std::string _filename;

	VkImageLayout _currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAccessFlags _currentAccessMask = VK_ACCESS_MEMORY_READ_BIT;// VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	VkPipelineStageFlags _currentStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;// VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;

	void insertImageBarrier(VkCommandBuffer cmdbuffer, VkImageLayout newImageLayout, VkAccessFlags dstAccessMask, VkPipelineStageFlags dstStageMask) {
		VkImageSubresourceRange range;
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = 0;
		range.levelCount = 1;
		range.baseArrayLayer = 0;
		range.layerCount = 1;

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = _currentLayout;
		barrier.newLayout = newImageLayout;
		barrier.image = image._image;
		barrier.subresourceRange = range;
		barrier.srcAccessMask = _currentAccessMask;
		barrier.dstAccessMask = dstAccessMask;
		vkCmdPipelineBarrier(cmdbuffer, _currentStageFlags, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		_currentLayout = newImageLayout;
		_currentAccessMask = dstAccessMask;
		_currentStageFlags = dstStageMask;
	}
};