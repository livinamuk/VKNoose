#pragma once
#include "API/Vulkan/Types/vk_buffer.h"

namespace VulkanRenderer {
    bool Init();
    
    void UploadGlobalGeometry();
    void BuildAllBLAS();

    VulkanBuffer* GetVertexBuffer();
    VulkanBuffer* GetIndexBuffer();

    void Cleanup();
}