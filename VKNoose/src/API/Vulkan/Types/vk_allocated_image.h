#pragma once

#include "API/Vulkan/vk_common.h"
#include <string>

struct AllocatedImage {
    AllocatedImage() = default;
    AllocatedImage(VkDevice device, VmaAllocator allocator, VkFormat imageFormat, VkExtent3D imageExtent, VkImageUsageFlags usage, std::string debugName);
    AllocatedImage(const AllocatedImage&) = delete;
    AllocatedImage& operator=(const AllocatedImage&) = delete;
    AllocatedImage(AllocatedImage&& other) noexcept;
    AllocatedImage& operator=(AllocatedImage&& other) noexcept;

    void TransitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout, VkAccessFlags dstAccess, VkPipelineStageFlags dstStage);
    void Cleanup(VkDevice device, VmaAllocator allocator);

    int32_t GetWidth() const            { return m_extent.width; }
    int32_t GetHeight() const           { return m_extent.height; }
    int32_t GetDepth() const            { return m_extent.depth; }
    VkExtent3D GetExtent() const        { return m_extent; }
    VkFormat GetFormat() const          { return m_format; }
    VkImage GetImage() const            { return m_image; }
    VkImageView GetImageView() const    { return m_imageView; }
    VmaAllocation GetAllocation() const { return m_allocation; }

private:
    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VkExtent3D m_extent = {};
    VkFormat m_format = VK_FORMAT_UNDEFINED;

    VkImageLayout m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkAccessFlags m_currentAccessMask = 0;
    VkPipelineStageFlags m_currentStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
};