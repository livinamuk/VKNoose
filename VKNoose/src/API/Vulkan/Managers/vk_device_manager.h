#pragma once
#include "API/Vulkan/vk_common.h"

namespace VulkanDeviceManager {
    bool Init();
    void Cleanup();
    
    VkPhysicalDevice GetPhysicalDevice();
    VkDevice GetDevice();
    VkQueue GetGraphicsQueue();
    uint32_t GetGraphicsQueueFamily();
    VkQueue GetPresentQueue();
    uint32_t GetPresentQueueFamily();
    const VkPhysicalDeviceProperties& GetProperties();
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& GetRayTracingPipelineProperties();
    const VkPhysicalDeviceAccelerationStructureFeaturesKHR& GetAccelerationStructureFeatures();
    const VkPhysicalDeviceMemoryProperties& GetMemoryProperties();
}
