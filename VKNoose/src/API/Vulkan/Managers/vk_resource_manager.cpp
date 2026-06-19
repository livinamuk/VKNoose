#include "vk_resource_manager.h"

#include "API/Vulkan/Managers/vk_device_manager.h"

#include "AssetManagement/AssetManager.h"
#include "Hell/Logging.h"
#include "Hell/Core/UniqueID.h"
#include "Hell/Containers/SlotMap.h"

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
        for (auto& object : g_accelerationStructures)  { object.Cleanup(); } g_accelerationStructures.clear();
        for (auto& object : g_buffers)                 { object.Cleanup(); } g_buffers.clear();

        for (auto& [name, object] : g_descriptorSets)  { object.Cleanup(); } g_descriptorSets.clear();
        for (auto& [name, object] : g_allocatedImages) { object.Cleanup(); } g_allocatedImages.clear();
        for (auto& [name, object] : g_samplers)        { object.Cleanup(); } g_samplers.clear();
        for (auto& [name, shader] : g_shaders)         { shader.Cleanup(); } g_shaders.clear();

        CleanUpPipelines();
    }

    void CleanUpPipelines() {
        for (auto& [name, object] : g_pipelines)           { object.Cleanup(); } g_pipelines.clear();
        for (auto& [name, object] : g_raytracingPipelines) { object.Cleanup(); } g_raytracingPipelines.clear();
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

        if (!g_accelerationStructures.get(id)) {
            Logging::Error() << "VulkanResourceManager::CreateAccelerationStructure(..) failed to create acceleration structure with id '" << id << "'.\n";
            __debugbreak();
        }

        return id;
    }

    VulkanAccelerationStructure* GetAccelerationStructure(uint64_t id) {
        VulkanAccelerationStructure* accelerationStructure = g_accelerationStructures.get(id);

        if (!accelerationStructure) {
            Logging::Error() << "VulkanResourceManager::GetAccelerationStructure(..) no acceleration structure with id '" << id << "'.\n";
        }

        return accelerationStructure;
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
            Logging::Error() << "VulkanResourceManager::CreateAllocatedImage(..) zero dimension image '" << name << "' requested.\n";
            __debugbreak();
        }

        auto [it, inserted] = g_allocatedImages.try_emplace(name);

        if (!inserted) {
            it->second.Cleanup();
        }

        VkExtent3D extent{ width, height, 1 };
        it->second = AllocatedImage(format, extent, usage, name);
        return it->second;
    }

    AllocatedImage* GetAllocatedImage(const std::string& name) {
        auto it = g_allocatedImages.find(name);
        if (it != g_allocatedImages.end()) {
            return &it->second;
        }

        Logging::Error() << "VulkanResourceManager::GetAllocatedImage(..) no allocated image named '" << name << "'.\n";
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
        if (size == 0) {
            Logging::Error() << "VulkanResourceManager::CreateBuffer(..) zero-sized buffer requested.\n";
            __debugbreak();
        }

        if (usage == 0) {
            Logging::Error() << "VulkanResourceManager::CreateBuffer(..) buffer with no usage flags requested.\n";
            __debugbreak();
        }

        const uint64_t id = UniqueID::GetNextObjectId(ObjectType::VK_BUFFER);
        g_buffers.emplace_with_id(id, size, usage, memoryUsage, vmaFlags);

        VulkanBuffer* buffer = g_buffers.get(id);
        if (!buffer || buffer->GetBuffer() == VK_NULL_HANDLE) {
            Logging::Error() << "VulkanResourceManager::CreateBuffer(..) failed to create buffer with id '" << id << "'.\n";
            __debugbreak();
        }

        return id;
    }

    VulkanBuffer* GetBuffer(uint64_t id) {
        VulkanBuffer* buffer = g_buffers.get(id);

        if (!buffer) {
            Logging::Error() << "VulkanResourceManager::GetBuffer(..) no buffer with id '" << id << "'.\n";
        }

        return buffer;
    }

    void UploadBufferData(uint64_t id, const void* data, VkDeviceSize size) {
        if (VulkanBuffer* buffer = g_buffers.get(id)) {
            buffer->UploadData(data, size);
        }
        else {
            Logging::Error() << "VulkanResourceManager::UploadBufferData(..) no buffer with id '" << id << "'.\n";
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
        if (name.empty()) {
            Logging::Error() << "VulkanResourceManager::CreateDescriptorSet(..) empty resource name requested.\n";
            __debugbreak();
        }

        if (layoutInfo.sType != VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO) {
            Logging::Error() << "VulkanResourceManager::CreateDescriptorSet(..) invalid descriptor set layout create info for '" << name << "'.\n";
            __debugbreak();
        }

        if (layoutInfo.bindingCount > 0 && !layoutInfo.pBindings) {
            Logging::Error() << "VulkanResourceManager::CreateDescriptorSet(..) null bindings pointer for '" << name << "'.\n";
            __debugbreak();
        }

        auto [it, inserted] = g_descriptorSets.try_emplace(name);

        if (!inserted) {
            it->second.Cleanup();
        }

        it->second = VulkanDescriptorSetResource(layoutInfo, lifetime);

        if (it->second.GetLayout() == VK_NULL_HANDLE) {
            Logging::Error() << "VulkanResourceManager::CreateDescriptorSet(..) failed to create descriptor set resource '" << name << "'.\n";
            __debugbreak();
        }

        return it->second;
    }

    VulkanDescriptorSetResource* GetDescriptorSetResource(const std::string& name) {
        auto it = g_descriptorSets.find(name);

        if (it == g_descriptorSets.end()) {
            Logging::Error() << "VulkanResourceManager::GetDescriptorSetResource(..) no descriptor set resource named '" << name << "'.\n";
            return nullptr;
        }

        return &it->second;
    }

    VulkanDescriptorSet* GetDescriptorSet(const std::string& name) {
        auto it = g_descriptorSets.find(name);

        if (it == g_descriptorSets.end()) {
            Logging::Error() << "VulkanResourceManager::GetDescriptorSet(..) no descriptor set resource named '" << name << "'.\n";
            return nullptr;
        }

        return &it->second.GetSet();
    }

    VkDescriptorSetLayout GetDescriptorSetLayout(const std::string& name) {
        auto it = g_descriptorSets.find(name);

        if (it == g_descriptorSets.end()) {
            Logging::Error() << "VulkanResourceManager::GetDescriptorSetLayout(..) no descriptor set resource named '" << name << "'.\n";
            return VK_NULL_HANDLE;
        }

        return it->second.GetLayout();
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
        if (name.empty()) {
            Logging::Error() << "VulkanResourceManager::CreatePipeline(..) empty resource name requested.\n";
            __debugbreak();
        }

        auto it = g_pipelines.find(name);
        if (it != g_pipelines.end()) {
            it->second.Cleanup();
            g_pipelines.erase(it);
        }

        return g_pipelines[name];
    }

    VulkanRaytracingPipeline& CreateRaytracingPipeline(const std::string& name) {
        if (name.empty()) {
            Logging::Error() << "VulkanResourceManager::CreateRaytracingPipeline(..) empty resource name requested.\n";
            __debugbreak();
        }

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

        Logging::Error() << "VulkanResourceManager::GetPipeline(..) no graphics pipeline named '" << name << "'.\n";
        return nullptr;
    }

    VulkanRaytracingPipeline* GetRaytracingPipeline(const std::string& name) {
        auto it = g_raytracingPipelines.find(name);
        if (it != g_raytracingPipelines.end()) {
            return &it->second;
        }

        Logging::Error() << "VulkanResourceManager::GetRaytracingPipeline(..) no raytracing pipeline named '" << name << "'.\n";
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
        if (name.empty()) {
            Logging::Error() << "VulkanResourceManager::CreateSampler(..) empty resource name requested.\n";
            __debugbreak();
        }

        if (maxAnisotropy <= 0.0f) {
            Logging::Error() << "VulkanResourceManager::CreateSampler(..) non-positive max anisotropy requested for '" << name << "'.\n";
            __debugbreak();
        }

        // Get iterator to the new or existing element
        auto [it, inserted] = g_samplers.try_emplace(name);

        // If it already existed, clean up the old one
        if (!inserted) {
            it->second.Cleanup();
        }

        // Move a new sampler into the slot
        it->second = VulkanSampler(magFilter, minFilter, addressMode, maxAnisotropy);

        if (it->second.GetSampler() == VK_NULL_HANDLE) {
            Logging::Error() << "VulkanResourceManager::CreateSampler(..) failed to create sampler '" << name << "'.\n";
            __debugbreak();
        }

        return it->second;
    }

    VulkanSampler* GetSampler(const std::string& name) {
        auto it = g_samplers.find(name);
        if (it != g_samplers.end()) {
            return &it->second;
        }

        Logging::Error() << "VulkanResourceManager::GetSampler(..) no sampler named '" << name << "'.\n";
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
        if (name.empty()) {
            Logging::Error() << "VulkanResourceManager::CreateShader(..) empty resource name requested.\n";
            __debugbreak();
        }

        if (paths.empty()) {
            Logging::Error() << "VulkanResourceManager::CreateShader(..) no shader paths supplied for '" << name << "'.\n";
            __debugbreak();
        }

        g_shaders.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(name),
            std::forward_as_tuple(paths)
        );

        VulkanShader& shader = g_shaders.at(name);
        if (shader.GetStageCreateInfos().empty()) {
            Logging::Error() << "VulkanResourceManager::CreateShader(..) failed to create any shader modules for '" << name << "'.\n";
            __debugbreak();
        }

        return shader;
    }

    VulkanShader* GetShader(const std::string& name) {
        auto it = g_shaders.find(name);
        if (it != g_shaders.end()) {
            return &it->second;
        }

        Logging::Error() << "VulkanResourceManager::GetShader(..) no shader named '" << name << "'.\n";
        return nullptr;
    }

    bool ShaderExists(const std::string& name) {
        return g_shaders.find(name) != g_shaders.end();
    }

    bool HotloadShaders() {
        bool success = true;

        for (auto& [name, shader] : g_shaders) {
            if (!shader.Hotload()) {
                success = false;
            }
        }

        return success;
    }
}
