#pragma once
#include "API/Vulkan/vk_common.h"

#include <iostream>
#include <vector>

/*#define VK_CHECK(x)                                             \
do                                                              \
{                                                               \
	VkResult err = x;                                           \
	if (err)                                                    \
	{                                                           \
		std::cout <<"Detected Vulkan error: " << err << "\n";	\
		abort();                                                \
	}                                                           \
} while (0)*/

inline void VK_CHECK(VkResult err) {
	if (err) {
		std::cout << "Detected Vulkan error: " << err << "\n";
		abort();                                               
	}
}

struct AllocatedBuffer {
	VkBuffer _buffer = VK_NULL_HANDLE;
	VmaAllocation _allocation;
	void* _mapped = nullptr;
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

struct HellBuffer {
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation;
	unsigned int size = 0;
	void Create(VmaAllocator allocator, unsigned int srcSize, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryUsageFlags);
	void Map(VmaAllocator allocator, void* srcData);
	void MapRange(VmaAllocator allocator, void* srcData, size_t size);
	void Destroy(VmaAllocator allocator);
};

struct HellDepthTarget {
	VkImage _image = VK_NULL_HANDLE;
	VkImageView _view = VK_NULL_HANDLE;
	VmaAllocation _allocation = VK_NULL_HANDLE;	
	VkExtent3D _extent = {};
	VkFormat _format;	
	VkImageLayout _currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;;
	VkAccessFlags _currentAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	VkPipelineStageFlags _currentStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;

	void Create(VkDevice device, VmaAllocator allocator, VkFormat format, VkExtent3D extent) {

		VkImageCreateInfo info = { };
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.pNext = nullptr;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.format = format;
		info.extent = extent;
		info.mipLevels = 1;
		info.arrayLayers = 1;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		VmaAllocationCreateInfo dimg_allocinfo = {};
		dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		vmaCreateImage(allocator, &info, &dimg_allocinfo, &_image, &_allocation, nullptr);

		VkImageViewCreateInfo dview_info = {};
		dview_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		dview_info.pNext = nullptr;
		dview_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		dview_info.image = _image;
		dview_info.format = format;
		dview_info.subresourceRange.baseMipLevel = 0;
		dview_info.subresourceRange.levelCount = 1;
		dview_info.subresourceRange.baseArrayLayer = 0;
		dview_info.subresourceRange.layerCount = 1;
		dview_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		VK_CHECK(vkCreateImageView(device, &dview_info, nullptr, &_view));

		_format = format;
		_extent = extent;
	}

	void InsertImageBarrier(VkCommandBuffer cmdbuffer, VkImageLayout newImageLayout, VkAccessFlags dstAccessMask, VkPipelineStageFlags dstStageMask) {
		VkImageSubresourceRange range;
		range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		range.baseMipLevel = 0;
		range.levelCount = 1;
		range.baseArrayLayer = 0;
		range.layerCount = 1;

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = _currentLayout;
		barrier.newLayout = newImageLayout;
		barrier.image = _image;
		barrier.subresourceRange = range;
		barrier.srcAccessMask = _currentAccessMask;
		barrier.dstAccessMask = dstAccessMask;
		vkCmdPipelineBarrier(cmdbuffer, _currentStageFlags, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		_currentLayout = newImageLayout;
		_currentAccessMask = dstAccessMask;
		_currentStageFlags = dstStageMask;
	}

	void Cleanup(VkDevice device, VmaAllocator allocator) {
		vkDestroyImageView(device, _view, nullptr);
		vmaDestroyImage(allocator, _image, _allocation);
	}
};

struct RenderTarget {
	VkImage _image = VK_NULL_HANDLE;
	VkImageView _view = VK_NULL_HANDLE;
	VmaAllocation _allocation = VK_NULL_HANDLE;
	VkImageLayout _currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAccessFlags _currentAccessMask = VK_ACCESS_MEMORY_READ_BIT;// VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	VkPipelineStageFlags _currentStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;// VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
	VkExtent3D _extent = {};
	VkFormat _format;

	RenderTarget() {}

	RenderTarget(VkDevice device, VmaAllocator allocator, VkFormat format, uint32_t width, uint32_t height, VkImageUsageFlags imageUsage, std::string debugName, VmaMemoryUsage memoryUsuage = VMA_MEMORY_USAGE_GPU_ONLY, VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {

		_format = format;
		_extent = { width, height, 1 };

		VmaAllocationCreateInfo img_allocinfo = {};
		img_allocinfo.usage = memoryUsuage;// VMA_MEMORY_USAGE_GPU_ONLY;
		img_allocinfo.requiredFlags = memoryFlags;// VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VkImageCreateInfo imageInfo = { };
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.pNext = nullptr;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = format;
		imageInfo.extent = _extent;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = imageUsage;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		//imageInfo.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
		vmaCreateImage(allocator, &imageInfo, &img_allocinfo, &_image, &_allocation, nullptr);

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.pNext = nullptr;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.image = _image;
		viewInfo.format = format;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &_view));

		VkDebugUtilsObjectNameInfoEXT nameInfo = {};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
		nameInfo.objectHandle = (uint64_t)_image;
		nameInfo.pObjectName = debugName.c_str(); 
		PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));
		vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
	}

	void cleanup(VkDevice device, VmaAllocator allocator) {
		vkDestroyImageView(device, _view, nullptr);
		vmaDestroyImage(allocator, _image, _allocation);
	}

	void insertImageBarrier (VkCommandBuffer cmdbuffer, VkImageLayout newImageLayout, VkAccessFlags dstAccessMask, VkPipelineStageFlags dstStageMask) {
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
		barrier.image = _image;
		barrier.subresourceRange = range;
		barrier.srcAccessMask = _currentAccessMask;
		barrier.dstAccessMask = dstAccessMask;
		vkCmdPipelineBarrier(cmdbuffer, _currentStageFlags, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		_currentLayout = newImageLayout;
		_currentAccessMask = dstAccessMask;
		_currentStageFlags = dstStageMask;
	}
};

struct AllocatedImageOLD {
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