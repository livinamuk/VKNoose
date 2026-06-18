#pragma once
#include "API/Vulkan/vk_common.h"
#include "API/Vulkan/Types/vk_acceleration_structure.h"
#include "API/Vulkan/Types/vk_allocated_image.h"
#include "API/Vulkan/Types/vk_buffer.h"
#include "API/Vulkan/Types/vk_descriptor_set.h"
#include "API/Vulkan/Types/vk_pipeline.h"
#include "API/Vulkan/Types/vk_raytracing_pipeline.h"
#include "API/Vulkan/Types/vk_sampler.h"
#include "API/Vulkan/Types/vk_shader.h"
#include <unordered_map>
#include <string>

namespace VulkanResourceManager {
    void Cleanup();

    // Acceleration Structures
    uint64_t CreateAccelerationStructure();
    VulkanAccelerationStructure* GetAccelerationStructure(uint64_t id);

    // Allocated Images
    AllocatedImage& CreateAllocatedImage(const std::string& name, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);
    AllocatedImage* GetAllocatedImage(const std::string& name);
    bool AllocatedImageExists(const std::string& name);

    // Buffers
    uint64_t CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags vmaFlags = 0);
    VulkanBuffer* GetBuffer(uint64_t id);
    void UploadBufferData(uint64_t id, const void* data, VkDeviceSize size);

    // Descriptor Sets
    VulkanDescriptorSetResource& CreateDescriptorSet(const std::string& name, VkDescriptorSetLayoutCreateInfo layoutInfo, DescriptorSetLifetime lifetime);
    VulkanDescriptorSetResource* GetDescriptorSetResource(const std::string& name);
    VulkanDescriptorSet* GetDescriptorSet(const std::string& name);
    VkDescriptorSetLayout GetDescriptorSetLayout(const std::string& name);

    // Pipelines
    VulkanPipeline& CreatePipeline(const std::string& name);
    VulkanPipeline* GetPipeline(const std::string& name);
    VulkanRaytracingPipeline& CreateRaytracingPipeline(const std::string& name);
    VulkanRaytracingPipeline* GetRaytracingPipeline(const std::string& name);
    void CleanUpPipelines();

    // Samplers
    VulkanSampler& CreateSampler(const std::string& name, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode, float maxAnisotropy = 1.0f);
    VulkanSampler* GetSampler(const std::string& name);
    bool SamplerExists(const std::string& name);

    // Shaders
    VulkanShader& CreateShader(const std::string& name, const std::vector<std::string>& paths);
    VulkanShader* GetShader(const std::string& name);
    bool ShaderExists(const std::string& name);
    bool HotloadShaders();
}
