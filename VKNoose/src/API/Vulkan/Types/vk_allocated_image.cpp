#include "vk_allocated_image.h"

AllocatedImage::AllocatedImage(VkDevice device, VmaAllocator allocator, VkFormat imageFormat, VkExtent3D imageExtent, VkImageUsageFlags usage, std::string debugName) {
    m_format = imageFormat;
    m_extent = imageExtent;
    m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = m_format;
    imageInfo.extent = m_extent;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = usage;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    vmaCreateImage(allocator, &imageInfo, &allocInfo, &m_image, &m_allocation, nullptr);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.image = m_image;
    viewInfo.format = m_format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    vkCreateImageView(device, &viewInfo, nullptr, &m_imageView);

    VkDebugUtilsObjectNameInfoEXT nameInfo = {};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
    nameInfo.objectHandle = (uint64_t)m_image;
    nameInfo.pObjectName = debugName.c_str();

    auto func = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
    if (func) {
        func(device, &nameInfo);
    }
}

AllocatedImage::AllocatedImage(AllocatedImage&& other) noexcept {
    m_image = other.m_image;
    m_imageView = other.m_imageView;
    m_allocation = other.m_allocation;
    m_extent = other.m_extent;
    m_format = other.m_format;
    m_currentLayout = other.m_currentLayout;
    m_currentAccessMask = other.m_currentAccessMask;
    m_currentStageFlags = other.m_currentStageFlags;

    // Nullify the other object so its cleanup doesn't destroy our handles
    other.m_image = VK_NULL_HANDLE;
    other.m_imageView = VK_NULL_HANDLE;
    other.m_allocation = VK_NULL_HANDLE;
}

AllocatedImage& AllocatedImage::operator=(AllocatedImage&& other) noexcept {
    if (this != &other) {
        m_image = other.m_image;
        m_imageView = other.m_imageView;
        m_allocation = other.m_allocation;
        m_extent = other.m_extent;
        m_format = other.m_format;
        m_currentLayout = other.m_currentLayout;
        m_currentAccessMask = other.m_currentAccessMask;
        m_currentStageFlags = other.m_currentStageFlags;

        other.m_image = VK_NULL_HANDLE;
        other.m_imageView = VK_NULL_HANDLE;
        other.m_allocation = VK_NULL_HANDLE;
    }
    return *this;
}

void AllocatedImage::TransitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout, VkAccessFlags dstAccess, VkPipelineStageFlags dstStage) {
    if (m_currentLayout == newLayout) {
        return;
    }

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = m_currentLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = m_currentAccessMask;
    barrier.dstAccessMask = dstAccess;

    vkCmdPipelineBarrier(cmd, m_currentStageFlags, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    m_currentLayout = newLayout;
    m_currentAccessMask = dstAccess;
    m_currentStageFlags = dstStage;
}

void AllocatedImage::Cleanup(VkDevice device, VmaAllocator allocator) {
    if (m_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_imageView, nullptr);
    }
    if (m_image != VK_NULL_HANDLE) {
        vmaDestroyImage(allocator, m_image, m_allocation);
    }
}