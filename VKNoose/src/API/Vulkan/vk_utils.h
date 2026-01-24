#pragma once
#include "vk_common.h"

namespace VulkanUtils {
    inline VkImageAspectFlags GetImageAspectFlagsFromFormat(VkFormat format) {
        if (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D16_UNORM) {
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }

    inline uint64_t GetBufferDeviceAddress(VkDevice device, VkBuffer buffer) {
        VkBufferDeviceAddressInfoKHR bufferDeviceAI{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
        bufferDeviceAI.buffer = buffer;
        return vkGetBufferDeviceAddressKHR(device, &bufferDeviceAI);
    }
}