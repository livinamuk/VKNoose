#pragma once
#include "API/Vulkan/vk_common.h"
#include "API/Vulkan/Types/vk_pipeline.h"

#include <string>
#include <unordered_map>

namespace VulkanPipelineManager {
    bool Init();
    void Cleanup();

    VulkanPipeline* GetPipeline(const std::string& name);

    void ReloadAll();
}