#pragma once
#include "API/Vulkan/vk_common.h"
#include "API/Vulkan/vk_types.h"
#include "API/Vulkan/Types/vk_acceleration_structure.h"
#include "API/Vulkan/Types/vk_buffer.h"
#include "Types/Mesh.h" // use me when u can

// Forward declaration to avoid including the full mesh header here
struct MeshOLD;

namespace VulkanRaytracingManager {
    VulkanAccelerationStructure CreateTopLevelAS(const std::vector<VkAccelerationStructureInstanceKHR>& instances);
    VulkanAccelerationStructure CreateBottomLevelAS(MeshOLD* mesh);

    // Helpers now return the new VulkanBuffer class
    VulkanBuffer CreateScratchBuffer(VkDeviceSize size);
    void CreateASBuffer(VulkanAccelerationStructure& as, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);
}