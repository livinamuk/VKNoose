#include "vk_renderer.h"
#include "API/Vulkan/Managers/vk_device_manager.h" // Refactor so I'm not needed here
#include "API/Vulkan/Managers/vk_resource_manager.h"
#include "API/Vulkan/Renderer/vk_descriptor_indices.h"

namespace VulkanRenderer {

    void CreateFrameData();
    void CreatePipelines();
    void CreateRenderTargets();
    void CreateSamplers();
    void CreateStaticDescriptorSet();
    void CreateTlasDescriptorSets();

    bool Init() {
        LoadShaders();
        CreateSamplers();
        CreateRenderTargets();
        CreatePipelines();
        CreateStaticDescriptorSet();
        CreateTlasDescriptorSets();
        CreateFrameData();
        UpdateStaticDescriptorSet();
        return true;
    }

    void LoadShaders() {
        // Vertex/Fragment shaders
        VulkanResourceManager::CreateShader("TextBlitter", { "vk_text_blitter.vert", "vk_text_blitter.frag" });
        VulkanResourceManager::CreateShader("SolidColor", { "vk_solid_color.vert", "vk_solid_color.frag" });
        VulkanResourceManager::CreateShader("GBuffer", { "vk_gbuffer.vert", "vk_gbuffer.frag" });
        VulkanResourceManager::CreateShader("Composite", { "vk_composite.vert", "vk_composite.frag" });

        // Path Tracer Raytracing Shaders
        VulkanResourceManager::CreateShader("Path_RayGen", { "path_raygen.rgen" });
        VulkanResourceManager::CreateShader("Path_Miss", { "path_miss.rmiss" });
        VulkanResourceManager::CreateShader("Path_Shadow", { "path_shadow.rmiss" });
        VulkanResourceManager::CreateShader("Path_Hit", { "path_closesthit.rchit" });

        // Mouse Pick Raytracing Shaders
        VulkanResourceManager::CreateShader("Mouse_RayGen", { "mousepick_raygen.rgen" });
        VulkanResourceManager::CreateShader("Mouse_Miss", { "mousepick_miss.rmiss" });
        VulkanResourceManager::CreateShader("Mouse_Hit", { "mousepick_closesthit.rchit" });
    }

    void CreateSamplers() {
        const VkPhysicalDeviceProperties& properties = VulkanDeviceManager::GetProperties();
        int maxAnisotropy = properties.limits.maxSamplerAnisotropy;

        VulkanResourceManager::CreateSampler("Linear", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, maxAnisotropy);
        VulkanResourceManager::CreateSampler("Nearest", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, maxAnisotropy);
    }

    void CreateRenderTargets() {
        // Present resoltion
        uint32_t width = PRESENT_WIDTH;
        uint32_t height = PRESENT_HEIGHT;

        // Raytrace resolution
        int scale = 2;
        uint32_t rtWidth = PRESENT_WIDTH * scale;
        uint32_t rtHeight = PRESENT_HEIGHT * scale;

        VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        VulkanResourceManager::CreateAllocatedImage("LoadingScreen", 1024, 576, VK_FORMAT_R8G8B8A8_UNORM, usage);

        // Raytracing Storage Images
        VulkanResourceManager::CreateAllocatedImage("RT_FirstHit_Color", rtWidth, rtHeight, VK_FORMAT_R32G32B32A32_SFLOAT, usage);
        VulkanResourceManager::CreateAllocatedImage("RT_FirstHit_Normals", rtWidth, rtHeight, VK_FORMAT_R32G32B32A32_SFLOAT, usage);
        VulkanResourceManager::CreateAllocatedImage("RT_FirstHit_BaseColor", rtWidth, rtHeight, VK_FORMAT_R32G32B32A32_SFLOAT, usage);
        VulkanResourceManager::CreateAllocatedImage("RT_SecondHit_Color", rtWidth, rtHeight, VK_FORMAT_R32G32B32A32_SFLOAT, usage);

        // GBuffer Targets
        VulkanResourceManager::CreateAllocatedImage("GBuffer_BaseColor", rtWidth, rtHeight, VK_FORMAT_R8G8B8A8_UNORM, usage);
        VulkanResourceManager::CreateAllocatedImage("GBuffer_Normal", rtWidth, rtHeight, VK_FORMAT_R8G8B8A8_UNORM, usage);
        VulkanResourceManager::CreateAllocatedImage("GBuffer_RMA", rtWidth, rtHeight, VK_FORMAT_R8G8B8A8_UNORM, usage);

        // UI and Display Targets
        VulkanResourceManager::CreateAllocatedImage("LaptopDisplay", LAPTOP_DISPLAY_WIDTH, LAPTOP_DISPLAY_HEIGHT, VK_FORMAT_R8G8B8A8_UNORM, usage);
        VulkanResourceManager::CreateAllocatedImage("Composite", rtWidth, rtHeight, VK_FORMAT_R8G8B8A8_UNORM, usage);
        VulkanResourceManager::CreateAllocatedImage("Present", width, height, VK_FORMAT_R8G8B8A8_UNORM, usage);

        // Depth Targets
        VkImageUsageFlags depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        VulkanResourceManager::CreateAllocatedImage("Depth_Present", width, height, VK_FORMAT_D32_SFLOAT, depthUsage);
        VulkanResourceManager::CreateAllocatedImage("Depth_GBuffer", rtWidth, rtHeight, VK_FORMAT_D32_SFLOAT, depthUsage);
    }

    void CreatePipelines() {
        VkDevice device = VulkanDeviceManager::GetDevice();
    }

    void CreateStaticDescriptorSet() {
        VkDevice device = VulkanDeviceManager::GetDevice();

        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            { DESC_IDX_SAMPLERS, VK_DESCRIPTOR_TYPE_SAMPLER, 16, VK_SHADER_STAGE_ALL },
            { DESC_IDX_TEXTURES, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10000, VK_SHADER_STAGE_ALL },
            { DESC_IDX_UBOS,     VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 128, VK_SHADER_STAGE_ALL },
            { DESC_IDX_SSBOS,    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024, VK_SHADER_STAGE_ALL },

            // Grouped Storage Images by Format
            { DESC_IDX_STORAGE_IMAGES_RGBA32F, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100, VK_SHADER_STAGE_ALL },
            { DESC_IDX_STORAGE_IMAGES_RGBA16F, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100, VK_SHADER_STAGE_ALL },
            { DESC_IDX_STORAGE_IMAGES_RGBA8,   VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100, VK_SHADER_STAGE_ALL }
        };

        std::vector<VkDescriptorBindingFlags> flags(bindings.size(), 0);
        flags[1] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

        VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
        flagsInfo.bindingCount = (uint32_t)flags.size();
        flagsInfo.pBindingFlags = flags.data();

        VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layoutInfo.pNext = &flagsInfo;
        layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        layoutInfo.bindingCount = (uint32_t)bindings.size();
        layoutInfo.pBindings = bindings.data();

        VulkanResourceManager::CreateDescriptorSet("StaticDescriptorSet", layoutInfo, DescriptorSetLifetime::STATIC);
    }

    void CreateTlasDescriptorSets() {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = DESC_IDX_TLAS;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;

        VulkanResourceManager::CreateDescriptorSet("SceneTLASDescriptorSet", layoutInfo, DescriptorSetLifetime::PER_FRAME);
        VulkanResourceManager::CreateDescriptorSet("InventoryTLASDescriptorSet", layoutInfo, DescriptorSetLifetime::PER_FRAME);
    }
}