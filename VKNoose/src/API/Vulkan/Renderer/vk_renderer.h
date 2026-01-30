#pragma once
#include "API/Vulkan/Renderer/vk_frame_data.h"
#include "API/Vulkan/Types/vk_descriptor_set.h"
#include "API/Vulkan/Types/vk_buffer.h"

namespace VulkanRenderer {
    bool Init();
    
    void UploadGlobalGeometry();
    void BuildAllBLAS();

    VulkanBuffer* GetVertexBuffer();
    VulkanBuffer* GetIndexBuffer();

    VulkanDescriptorSet& GetStaticDescriptorSet();

    // Frame data
    VulkanFrameData& GetCurrentFrameData();
    VulkanFrameData& GetFrameDataByIndex(uint32_t frameIndex);
    uint32_t GetCurrentFrameIndex();
    void IncrementFrame();

    // Descriptor sets
    void UpdateDynamicDescriptorSet();

    void Cleanup();
}