#pragma once
#include "API/Vulkan/vk_common.h"
#include "API/Vulkan/Types/vk_allocated_image.h"
#include "API/Vulkan/Types/vk_buffer.h"
#include "API/Vulkan/Types/vk_descriptor_set.h"
#include "API/Vulkan/Types/vk_shader.h"
#include <unordered_map>
#include <string>

namespace VulkanResourceManager {
    void Cleanup();

    // Allocated Images
    AllocatedImage& CreateAllocatedImage(const std::string& name, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);
    AllocatedImage* GetAllocatedImage(const std::string& name);
    bool AllocatedImageExists(const std::string& name);

    // Buffers
    VulkanBuffer& CreateBuffer(const std::string& name, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags vmaFlags = 0);
    VulkanBuffer* GetBuffer(const std::string& name);
    bool BufferExists(const std::string& name);

    // Descriptor Sets
    void CreateDescriptorSet(const std::string& name, VkDescriptorSetLayout layout, VkDescriptorPool pool);
    VulkanDescriptorSet* GetDescriptorSet(const std::string& name);

    // Shaders
    VulkanShader& CreateShader(const std::string& name, const std::vector<std::string>& paths);
    VulkanShader* GetShader(const std::string& name);
    bool ShaderExists(const std::string& name);
}