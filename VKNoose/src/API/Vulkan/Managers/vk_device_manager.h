#pragma once
#include "API/Vulkan/vk_common.h"

namespace VulkanDeviceManager {
    void InitTest(VkPhysicalDevice g_physicalDevice);

    //bool Init();
    //void Cleanup();
    //
    //VkPhysicalDevice GetPhysicalDevice();
    //VkDevice GetDevice();
    //VmaAllocator GetAllocator();
    //
    //VkQueue GetGraphicsQueue();
    //uint32_t& GetGraphicsQueueFamily(); // make me not a reference
    //
    //VkQueue GetPresentQueue();
    //uint32_t& GetPresentQueueFamily(); // and me not a reference
    //
    //const VkPhysicalDeviceProperties& GetProperties();
    //const VkPhysicalDeviceMemoryProperties& GetMemoryProperties();

    const VkPhysicalDeviceProperties& GetProperties();
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& GetRayTracingPipelineProperties();
    const VkPhysicalDeviceAccelerationStructureFeaturesKHR& GetAccelerationStructureFeatures();
    const VkPhysicalDeviceMemoryProperties& GetMemoryProperties();
}
