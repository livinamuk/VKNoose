#pragma once
#include "API/Vulkan/Renderer/vk_frame_data.h"
#include "API/Vulkan/Types/vk_descriptor_set.h"
#include "API/Vulkan/Types/vk_buffer.h"

namespace VulkanRenderer {
    bool Init();
    void Cleanup();

    void RecreatePipelines();

    void LoadShaders();
    
    void UploadGlobalGeometry();
    void BuildAllBLAS();

    VulkanBuffer* GetVertexBuffer();
    VulkanBuffer* GetIndexBuffer();
    uint64_t GetVertexBufferAddress();
    uint64_t GetIndexBufferAddress();

    // Frame data
    VulkanFrameData& GetCurrentFrameData();
    VulkanFrameData& GetFrameDataByIndex(uint32_t frameIndex);
    uint32_t GetCurrentFrameIndex();
    void IncrementFrame();

    // Update
    void UpdateStaticDescriptorSet();
    void UpdateTLASDescriptorSets();
}