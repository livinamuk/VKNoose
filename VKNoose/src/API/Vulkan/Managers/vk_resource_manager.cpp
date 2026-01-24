#include "vk_resource_manager.h"
#include "API/Vulkan/Managers/vk_device_manager.h"
#include "API/Vulkan/Managers/vk_memory_manager.h"
#include "AssetManagement/AssetManager.h"
#include <iostream>

namespace VulkanResourceManager {
    std::unordered_map<std::string, AllocatedImage> g_allocatedImages; 
    std::unordered_map<std::string, VulkanBuffer> g_buffers; 
    std::unordered_map<std::string, VulkanDescriptorSet> g_descriptorSets;
    std::unordered_map<std::string, VulkanShader> g_shaders;

    void Cleanup() {
        VkDevice device = VulkanDeviceManager::GetDevice();
        VmaAllocator allocator = VulkanMemoryManager::GetAllocator();

        // Allocated images
        for (auto& [name, image] : g_allocatedImages) {
            image.Cleanup(device, allocator);
        }
        g_allocatedImages.clear();

        // Buffers
        for (auto& [name, buffer] : g_buffers) {
            buffer.Cleanup();
        }
        g_buffers.clear();

        // Shaders
        for (auto& [name, shader] : g_shaders) {
            shader.Cleanup(device);
        }
        g_shaders.clear();

        std::cout << "VulkanResourceManager::Cleanup()\n";
    }

    AllocatedImage& CreateAllocatedImage(const std::string& name, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage) {
        if (width == 0 || height == 0) {
            std::cerr << "VulkanResourceManager Error: Zero dimension image '" << name << "' requested.\n";
            __debugbreak();
        }

        g_allocatedImages.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(name),
            std::forward_as_tuple(
                VulkanDeviceManager::GetDevice(),
                VulkanMemoryManager::GetAllocator(),
                format,
                VkExtent3D{ width, height, 1 },
                usage,
                name
            )
        );
        return g_allocatedImages.at(name);
    }

    AllocatedImage* GetAllocatedImage(const std::string& name) {
        auto it = g_allocatedImages.find(name);
        if (it != g_allocatedImages.end()) {
            return &it->second;
        }
        return nullptr;
    }

    bool AllocatedImageExists(const std::string& name) {
        return g_allocatedImages.find(name) != g_allocatedImages.end();
    }

    VulkanBuffer& CreateBuffer(const std::string& name, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags vmaFlags) {
        g_buffers.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(name),
            std::forward_as_tuple(size, usage, memoryUsage, vmaFlags)
        );
        return g_buffers.at(name);
    }

    VulkanBuffer* GetBuffer(const std::string& name) {
        auto it = g_buffers.find(name);
        if (it != g_buffers.end()) {
            return &it->second;
        }
        return nullptr;
    }

    bool BufferExists(const std::string& name) {
        return g_buffers.find(name) != g_buffers.end();
    }

    VulkanShader& CreateShader(const std::string& name, const std::vector<std::string>& paths) {
        g_shaders.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(name),
            std::forward_as_tuple(VulkanDeviceManager::GetDevice(), paths)
        );
        return g_shaders.at(name);
    }

    VulkanShader* GetShader(const std::string& name) {
        auto it = g_shaders.find(name);
        if (it != g_shaders.end()) {
            return &it->second;
        }
        return nullptr;
    }

    bool ShaderExists(const std::string& name) {
        return g_shaders.find(name) != g_shaders.end();
    }
}