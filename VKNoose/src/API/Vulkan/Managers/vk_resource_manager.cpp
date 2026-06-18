#include "vk_resource_manager.h"
#include "API/Vulkan/Managers/vk_device_manager.h"
#include "API/Vulkan/Managers/vk_memory_manager.h"
#include "AssetManagement/AssetManager.h"

#include "Hell/Core/Logging.h"
#include "Hell/Core/UniqueID.h"
#include "Hell/Containers/SlotMap.h"

#include <iostream>

namespace VulkanResourceManager {
    std::unordered_map<std::string, AllocatedImage> g_allocatedImages;
    std::unordered_map<std::string, VulkanDescriptorSetResource> g_descriptorSets;
    std::unordered_map<std::string, VulkanPipeline> g_pipelines;
    std::unordered_map<std::string, VulkanRaytracingPipeline> g_raytracingPipelines;
    std::unordered_map<std::string, VulkanSampler> g_samplers;
    std::unordered_map<std::string, VulkanShader> g_shaders;

    Hell::SlotMap<VulkanAccelerationStructure> g_accelerationStructures;
    Hell::SlotMap<VulkanBuffer> g_buffers;

    void Cleanup() {
        VkDevice device = VulkanDeviceManager::GetDevice();
        VmaAllocator allocator = VulkanMemoryManager::GetAllocator();

        for (auto& object : g_accelerationStructures)      { object.Cleanup(); } g_accelerationStructures.clear();
        for (auto& object : g_buffers)                     { object.Cleanup(); } g_buffers.clear();
        for (auto& [name, object] : g_descriptorSets)      { object.Cleanup(); } g_descriptorSets.clear();
        for (auto& [name, object] : g_allocatedImages)     { object.Cleanup(device, allocator); } g_allocatedImages.clear();
        for (auto& [name, object] : g_samplers)            { object.Cleanup(); }       g_samplers.clear();
        for (auto& [name, shader] : g_shaders)             { shader.Cleanup(device); } g_shaders.clear();

        CleanUpPipelines();
    }

    void CleanUpPipelines() {
        VkDevice device = VulkanDeviceManager::GetDevice();
        for (auto& [name, object] : g_pipelines) { object.Cleanup(device); } g_pipelines.clear();
        for (auto& [name, object] : g_raytracingPipelines) { object.Cleanup(); }       g_raytracingPipelines.clear();
    }

                                                                                         
     /*  ▄▄                       ▄▄                                          ▄▄▄▄▄
       ▄█▀▀█▄                      ██                   █▄                   ██▀▀▀▀█▄  █▄                    █▄                         
       ██  ██                      ██       ▄          ▄██▄ ▀▀       ▄       ▀██▄  ▄▀ ▄██▄ ▄                ▄██▄       ▄                
       ██▀▀██   ▄███▀ ▄███▀ ▄█▀█▄  ██ ▄█▀█▄ ████▄ ▄▀▀█▄ ██  ██ ▄███▄ ████▄     ▀██▄▄   ██  ████▄ ██ ██ ▄███▀ ██  ██ ██ ████▄ ▄█▀█▄ ▄██▀█
     ▄ ██  ██   ██    ██    ██▄█▀  ██ ██▄█▀ ██    ▄█▀██ ██  ██ ██ ██ ██ ██   ▄   ▀██▄  ██  ██    ██ ██ ██    ██  ██ ██ ██    ██▄█▀ ▀███▄
     ▀██▀  ▀█▄█▄▀███▄▄▀███▄▄▀█▄▄▄ ▄██▄▀█▄▄▄▄█▀   ▄▀█▄██▄██ ▄██▄▀███▀▄██ ▀█   ▀██████▀ ▄██ ▄█▀   ▄▀██▀█▄▀███▄▄██ ▄▀██▀█▄█▀   ▄▀█▄▄▄█▄▄██▀
                                                                                                                                        */
                                                                        
    uint64_t CreateAccelerationStructure() {
        const uint64_t id = UniqueID::GetNextObjectId(ObjectType::VK_ACCELERATION_STRUCTURE);
        g_accelerationStructures.emplace_with_id(id);
        return id;
    }

    VulkanAccelerationStructure* GetAccelerationStructure(uint64_t id) {
        return g_accelerationStructures.get(id);
    }

                                                                                                
    /*   ▄▄     ▄▄  ▄▄                                       ▄▄▄▄▄▄
       ▄█▀▀█▄    ██  ██                   █▄           █▄   █▀ ██                                   
       ██  ██    ██  ██                  ▄██▄          ██      ██   ▄                 ▄▄            
       ██▀▀██    ██  ██ ▄███▄ ▄███▀ ▄▀▀█▄ ██  ▄█▀█▄ ▄████      ██   ███▄███▄ ▄▀▀█▄ ▄████ ▄█▀█▄ ▄██▀█
     ▄ ██  ██    ██  ██ ██ ██ ██    ▄█▀██ ██  ██▄█▀ ██ ██      ██   ██ ██ ██ ▄█▀██ ██ ██ ██▄█▀ ▀███▄
     ▀██▀  ▀█▄█ ▄██ ▄██▄▀███▀▄▀███▄▄▀█▄██▄██ ▄▀█▄▄▄▄█▀███    ▄▄██▄▄▄██ ██ ▀█▄▀█▄██▄▀████▄▀█▄▄▄█▄▄██▀
                                                                                      ██            
                                                                                    ▀▀▀             */           
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

                                          
    /*  ▄▄▄           ▄▄  ▄▄
       ██▀▀█▄        ██  ██                   
       ██ ▄█▀       ▄██▄▄██▄       ▄          
       ██▀▀█▄  ██ ██ ██  ██  ▄█▀█▄ ████▄ ▄██▀█
     ▄ ██  ▄█  ██ ██ ██  ██  ██▄█▀ ██    ▀███▄
     ▀██████▀ ▄▀██▀█▄██ ▄██ ▄▀█▄▄▄▄█▀   █▄▄██▀ 
                     ██  ██                   
                    ▀▀  ▀▀                    */                  

    uint64_t CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags vmaFlags) {
        const uint64_t id = UniqueID::GetNextObjectId(ObjectType::VK_BUFFER);
        g_buffers.emplace_with_id(id, size, usage, memoryUsage, vmaFlags);
        return id;
    }

    VulkanBuffer* GetBuffer(uint64_t id) {
        return g_buffers.get(id);
    }

    void UploadBufferData(uint64_t id, const void* data, VkDeviceSize size) {
        if (VulkanBuffer* buffer = GetBuffer(id)) {
            buffer->UploadData(data, size);
        }
        else {
            Logging::Error() << "VulkanResourceManager::UploadBufferData(..) failed. No VulkanBuffer with id '" << id << "'";
        }
    }

                                                                                  
  /*  ▄▄▄▄▄▄                                                        ▄▄▄▄▄
     █▀██▀▀██                                    █▄                ██▀▀▀▀█▄        █▄       
       ██   ██                    ▄     ▀▀      ▄██▄       ▄       ▀██▄  ▄▀       ▄██▄      
       ██   ██  ▄█▀█▄ ▄██▀█ ▄███▀ ████▄ ██ ████▄ ██  ▄███▄ ████▄     ▀██▄▄   ▄█▀█▄ ██  ▄██▀█
     ▄ ██   ██  ██▄█▀ ▀███▄ ██    ██    ██ ██ ██ ██  ██ ██ ██      ▄   ▀██▄  ██▄█▀ ██  ▀███▄
     ▀██▀███▀  ▄▀█▄▄▄█▄▄██▀▄▀███▄▄█▀   ▄██▄████▀▄██ ▄▀███▀▄█▀      ▀██████▀ ▄▀█▄▄▄▄██ █▄▄██▀
                                           ██                                               
                                           ▀                                                */                                              
    VulkanDescriptorSetResource& CreateDescriptorSet(const std::string& name, VkDescriptorSetLayoutCreateInfo layoutInfo, DescriptorSetLifetime lifetime) {
        auto [it, inserted] = g_descriptorSets.try_emplace(name);

        if (!inserted) {
            it->second.Cleanup();
        }

        it->second = VulkanDescriptorSetResource(layoutInfo, lifetime);
        return it->second;
    }

    VulkanDescriptorSetResource* GetDescriptorSetResource(const std::string& name) {
        auto it = g_descriptorSets.find(name);

        if (it == g_descriptorSets.end()) {
            return nullptr;
        }

        return &it->second;
    }

    VulkanDescriptorSet* GetDescriptorSet(const std::string& name) {
        VulkanDescriptorSetResource* resource = GetDescriptorSetResource(name);

        if (!resource) {
            return nullptr;
        }

        return &resource->GetSet();
    }

    VkDescriptorSetLayout GetDescriptorSetLayout(const std::string& name) {
        VulkanDescriptorSetResource* resource = GetDescriptorSetResource(name);

        if (!resource) {
            return VK_NULL_HANDLE;
        }

        return resource->GetLayout();
    }
    
                                                   
  /*  ▄▄▄▄▄▄                   ▄▄
     █▀██▀▀▀█▄                  ██                     
       ██▄▄▄█▀  ▀▀              ██ ▀▀ ▄                
       ██▀▀▀    ██ ████▄ ▄█▀█▄  ██ ██ ████▄ ▄█▀█▄ ▄██▀█
     ▄ ██       ██ ██ ██ ██▄█▀  ██ ██ ██ ██ ██▄█▀ ▀███▄
     ▀██▀      ▄██▄████▀▄▀█▄▄▄ ▄██▄██▄██ ▀█▄▀█▄▄▄█▄▄██▀
                   ██                                  
                   ▀                                    */                                 
    VulkanPipeline& CreatePipeline(const std::string& name) {
        VkDevice device = VulkanDeviceManager::GetDevice();

        auto it = g_pipelines.find(name);
        if (it != g_pipelines.end()) {
            it->second.Cleanup(device);
            g_pipelines.erase(it);
        }

        return g_pipelines[name];
    }

    VulkanRaytracingPipeline& CreateRaytracingPipeline(const std::string& name) {
        auto it = g_raytracingPipelines.find(name);
        if (it != g_raytracingPipelines.end()) {
            it->second.Cleanup();
            g_raytracingPipelines.erase(it);
        }

        return g_raytracingPipelines[name];
    }

    VulkanPipeline* GetPipeline(const std::string& name) {
        auto it = g_pipelines.find(name);
        if (it != g_pipelines.end()) {
            return &it->second;
        }
        std::cout << "VulkanPipelineManager::GetPipeline(..) failed to get '" << name << "'\n";
        return nullptr;
    }

    VulkanRaytracingPipeline* GetRaytracingPipeline(const std::string& name) {
        auto it = g_raytracingPipelines.find(name);
        if (it != g_raytracingPipelines.end()) {
            return &it->second;
        }
        std::cout << "VulkanPipelineManager::GetRaytracingPipeline(..) failed to get '" << name << "'\n";
        return nullptr;
    }


  /*  ▄▄▄▄▄                         ▄▄
     ██▀▀▀▀█▄                        ██                  
     ▀██▄  ▄▀        ▄               ██       ▄          
       ▀██▄▄   ▄▀▀█▄ ███▄███▄ ████▄  ██ ▄█▀█▄ ████▄ ▄██▀█
     ▄   ▀██▄  ▄█▀██ ██ ██ ██ ██ ██  ██ ██▄█▀ ██    ▀███▄
     ▀██████▀ ▄▀█▄██▄██ ██ ▀█▄████▀ ▄██▄▀█▄▄▄▄█▀   █▄▄██▀
                              ██                         
                              ▀                          */                         
    VulkanSampler& CreateSampler(const std::string& name, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode, float maxAnisotropy) {
        // Get iterator to the new or existing element
        auto [it, inserted] = g_samplers.try_emplace(name);

        // If it already existed, clean up the old one
        if (!inserted) {
            it->second.Cleanup();
        }

        // Move a new sampler into the slot
        it->second = VulkanSampler(magFilter, minFilter, addressMode, maxAnisotropy);

        return it->second;
    }

    VulkanSampler* GetSampler(const std::string& name) {
        auto it = g_samplers.find(name);
        if (it != g_samplers.end()) {
            return &it->second;
        }
        return nullptr;
    }

    bool SamplerExists(const std::string& name) {
        return g_samplers.find(name) != g_samplers.end();
    }


  /*  ▄▄▄▄▄
     ██▀▀▀▀█▄  █▄             █▄
     ▀██▄  ▄▀  ██             ██       ▄
       ▀██▄▄   ████▄ ▄▀▀█▄ ▄████ ▄█▀█▄ ████▄ ▄██▀█
     ▄   ▀██▄  ██ ██ ▄█▀██ ██ ██ ██▄█▀ ██    ▀███▄
     ▀██████▀ ▄██ ██▄▀█▄██▄█▀███▄▀█▄▄▄▄█▀   █▄▄██▀ 
                                                   */

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

    void HotloadShaders() {
        VkDevice device = VulkanDeviceManager::GetDevice();

        for (auto& [name, shader] : g_shaders) {
            shader.Hotload(device);
        }
    }
}