#pragma once
#include "API/Vulkan/Renderer/vk_frame_data.h"
#include "API/Vulkan/Types/vk_allocated_image.h"
#include "API/Vulkan/Types/vk_buffer.h"
#include "API/Vulkan/Types/vk_descriptor_set.h"

namespace VulkanRenderer {
    bool Init();

    void LoadShaders();
    void RecreatePipelines();
    void UploadGlobalGeometry();

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
    void UpdateBindlessTexturesDescriptorSets();
    void UpdateRenderTargetsDescriptorSets();
    void UpdateTLASDescriptorSets();

    // Util
    void BlitAllocatedImageToSwapchain(VkCommandBuffer cmd, AllocatedImage& srcImage, uint32_t swapchainIndex);
    void BuildAllBLAS();
    void HotloadShaders();
}
