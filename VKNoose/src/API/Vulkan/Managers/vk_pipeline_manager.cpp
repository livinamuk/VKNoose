#include "vk_pipeline_manager.h"
#include "API/Vulkan/Managers/vk_resource_manager.h"
#include "API/Vulkan/Managers/vk_descriptor_manager.h"
#include "API/Vulkan/Managers/vk_device_manager.h"
#include "Noose/Types.h"

#include <iostream>

namespace VulkanPipelineManager {

    std::unordered_map<std::string, VulkanPipeline> g_pipelines;

    void CreateTextBlitterPipeline() {
        VkDevice device = VulkanDeviceManager::GetDevice();
        VulkanShader* shader = VulkanResourceManager::GetShader("TextBlitter");

        if (!shader) {
            std::cerr << "[Pipeline Manager] Could not find shader: TextBlitter\n";
            return;
        }

        VulkanPipeline& pipeline = g_pipelines["TextBlitter"];
        pipeline.Cleanup(device);

        pipeline.PushDescriptorSetLayout(VulkanDescriptorManager::GetDynamicSetLayout());
        pipeline.PushDescriptorSetLayout(VulkanDescriptorManager::GetStaticSetLayout());

        pipeline.SetVertexDescription<Vertex>();
        pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pipeline.SetColorBlending(true);
        pipeline.SetDepthTest(false);

        // Building for a single color attachment (Loading Screen / Present)
        pipeline.Build(device, shader->GetVertexShader(), shader->GetFragmentShader(), 1, VK_FORMAT_R8G8B8A8_UNORM);
    }

    void CreateLinesPipeline() {
        VkDevice device = VulkanDeviceManager::GetDevice();
        VulkanShader* shader = VulkanResourceManager::GetShader("SolidColor");

        if (!shader) {
            std::cerr << "[Pipeline Manager] Could not find shader: SolidColor\n";
            return;
        }

        VulkanPipeline& pipeline = g_pipelines["Lines"];
        pipeline.Cleanup(device);

        pipeline.PushDescriptorSetLayout(VulkanDescriptorManager::GetDynamicSetLayout());
        pipeline.PushDescriptorSetLayout(VulkanDescriptorManager::GetStaticSetLayout());

        pipeline.SetPushConstant(64, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

        pipeline.SetVertexDescription<VertexDebug>();
        pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pipeline.SetColorBlending(false);
        pipeline.SetDepthTest(false);

        pipeline.Build(device, shader->GetVertexShader(), shader->GetFragmentShader(), 1, VK_FORMAT_R8G8B8A8_UNORM);
    }

    void CreateCompositePipeline() {
        VkDevice device = VulkanDeviceManager::GetDevice();
        VulkanShader* shader = VulkanResourceManager::GetShader("Composite");
        if (!shader) return;

        VulkanPipeline& pipeline = g_pipelines["Composite"];
        pipeline.Cleanup(device);

        pipeline.PushDescriptorSetLayout(VulkanDescriptorManager::GetDynamicSetLayout());
        pipeline.PushDescriptorSetLayout(VulkanDescriptorManager::GetStaticSetLayout());
        pipeline.PushDescriptorSetLayout(VulkanDescriptorManager::GetSamplerSetLayout());

        pipeline.SetVertexDescription<Vertex>();
        pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pipeline.SetColorBlending(false);
        pipeline.SetDepthTest(false);

        pipeline.Build(device, shader->GetVertexShader(), shader->GetFragmentShader(), 1, VK_FORMAT_R8G8B8A8_UNORM);
    }

    bool Init() {
        CreateTextBlitterPipeline();
        CreateLinesPipeline();
        CreateCompositePipeline();
        std::cout << "[Pipeline Manager] Initialized\n";

        return true;
    }

    void Cleanup() {
        VkDevice device = VulkanDeviceManager::GetDevice();
        for (auto& [name, pipeline] : g_pipelines) {
            pipeline.Cleanup(device);
        }
        g_pipelines.clear();
    }

    VulkanPipeline* GetPipeline(const std::string& name) {
        auto it = g_pipelines.find(name);
        if (it != g_pipelines.end()) {
            return &it->second;
        }
        std::cout << "VulkanPipelineManager::GetPipeline(..) failed to get '" << name << "'\n";
        return nullptr;
    }

    void ReloadAll() {
        std::cout << "[Pipeline Manager] Reloading all pipelines...\n";
        Init();
    }
}