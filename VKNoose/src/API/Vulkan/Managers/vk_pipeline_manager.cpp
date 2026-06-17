#include "vk_pipeline_manager.h"
#include "API/Vulkan/Managers/vk_resource_manager.h"
#include "API/Vulkan/Managers/vk_descriptor_manager.h"
#include "API/Vulkan/Managers/vk_device_manager.h"
#include "API/Vulkan/Renderer/vk_renderer.h"
#include "API/Vulkan/Renderer/vk_push_constants.h"
#include "Hell/Types.h"

#include <iostream>

namespace VulkanPipelineManager {
    std::unordered_map<std::string, VulkanPipeline> g_pipelines;
    std::unordered_map<std::string, VulkanRaytracingPipeline> g_raytracingPipelines;

    void CreateTextBlitterPipeline();
    void CreateLinesPipeline();
    void CreateCompositePipeline();
    void CreatePathRaytracingPipeline();
    void CreateMousePickRaytracingPipeline();

    VulkanPipeline& CreatPipeline(const std::string& name);
    VulkanRaytracingPipeline& CreateRaytracingPipeline(const std::string& name);

    bool Init() {
        CreateTextBlitterPipeline();
        CreateLinesPipeline();
        CreateCompositePipeline();
        CreatePathRaytracingPipeline();
        CreateMousePickRaytracingPipeline();

        std::cout << "[Pipeline Manager] Initialized\n";

        return true;
    }

    void Cleanup() {
        VkDevice device = VulkanDeviceManager::GetDevice();

        for (auto& [name, pipeline] : g_pipelines)           { pipeline.Cleanup(device); }
        for (auto& [name, pipeline] : g_raytracingPipelines) { pipeline.Cleanup(); }

        g_pipelines.clear();
        g_raytracingPipelines.clear();
    }

    void CreateTextBlitterPipeline() {
        VkDevice device = VulkanDeviceManager::GetDevice();
        VulkanShader* shader = VulkanResourceManager::GetShader("TextBlitter");

        if (!shader) {
            std::cerr << "[Pipeline Manager] Could not find shader: TextBlitter\n";
            return;
        }

        VulkanPipeline& pipeline = g_pipelines["TextBlitter"];
        pipeline.Cleanup(device);
        pipeline.PushDescriptorSetLayout(VulkanDescriptorManager::GetStaticSetLayout());
        pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pipeline.SetColorBlending(true);
        pipeline.SetDepthTest(false);
        pipeline.SetPushConstant(sizeof(UIPushConstant), VK_SHADER_STAGE_VERTEX_BIT);

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

        VulkanDescriptorSet& staticDescriptorSet = VulkanRenderer::GetStaticDescriptorSet();

        VulkanPipeline& pipeline = g_pipelines["Composite"];
        pipeline.Cleanup(device);

        pipeline.PushDescriptorSetLayout(staticDescriptorSet.GetLayout());

        pipeline.SetVertexDescription<Vertex>();
        pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pipeline.SetColorBlending(false);
        pipeline.SetDepthTest(false);

        pipeline.Build(device, shader->GetVertexShader(), shader->GetFragmentShader(), 1, VK_FORMAT_R8G8B8A8_UNORM);
    }

    // TODO: clean me up
    std::vector<VkDescriptorSetLayout> GetRaytracingDescriptorSetLayouts() {
        VulkanDescriptorSet& bindlessStaticSet = VulkanRenderer::GetStaticDescriptorSet();

        uint64_t id = VulkanRenderer::GetFrameDataByIndex(0).dynamicDescriptorSet;
        VulkanDescriptorSet* bindlessDynamicSet = VulkanResourceManager::GetDescriptorSet(id);

        if (!bindlessDynamicSet) {
            std::cerr << "[Pipeline Manager] Missing bindless dynamic descriptor set\n";
            return {};
        }

        return {
            VulkanDescriptorManager::GetDynamicSetLayout(),
            VulkanDescriptorManager::GetStaticSetLayout(),
            VulkanDescriptorManager::GetSamplerSetLayout(),
            bindlessStaticSet.GetLayout(),
            bindlessDynamicSet->GetLayout()
        };
    }

    void CreatePathRaytracingPipeline() {
        std::vector<VkDescriptorSetLayout> layouts = GetRaytracingDescriptorSetLayouts();
        if (layouts.empty()) return;

        std::vector<VkPushConstantRange> pushConstantRanges;
        VkPushConstantRange& pushConstantRange = pushConstantRanges.emplace_back();
        pushConstantRange = {};
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(ScenePushConstants);
        pushConstantRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        uint32_t maxRecursionDepth = 5;

        VulkanRaytracingPipeline& pipeline = CreateRaytracingPipeline("PathTrace");
        pipeline.AddRayGen("Path_RayGen");
        pipeline.AddMiss("Path_Miss");
        pipeline.AddMiss("Path_Shadow");
        pipeline.AddClosestHit("Path_Hit");
        pipeline.Build(layouts, pushConstantRanges, maxRecursionDepth);
    }

    void CreateMousePickRaytracingPipeline() {
        std::vector<VkDescriptorSetLayout> layouts = GetRaytracingDescriptorSetLayouts();
        if (layouts.empty()) return;

        std::vector<VkPushConstantRange> pushConstantRanges;
        VkPushConstantRange& pushConstantRange = pushConstantRanges.emplace_back();
        pushConstantRange = {};
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(ScenePushConstants);
        pushConstantRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        uint32_t maxRecursionDepth = 5;

        VulkanRaytracingPipeline& pipeline = CreateRaytracingPipeline("MousePick");
        pipeline.AddRayGen("Mouse_RayGen");
        pipeline.AddMiss("Mouse_Miss");
        pipeline.AddClosestHit("Mouse_Hit");
        pipeline.Build(layouts, pushConstantRanges, maxRecursionDepth);
    }

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

    void ReloadAll() {
        std::cout << "[Pipeline Manager] Reloading all pipelines...\n";
        Init();
    }
}