#pragma once
#include "API/Vulkan/vk_common.h"
#include <vector>

namespace VulkanSyncManager {
    bool Init();
    void Cleanup();

    VkSemaphore GetPresentSemaphore(uint32_t frameIndex);
    VkSemaphore GetRenderFinishedSemaphore(uint32_t frameIndex, uint32_t swapchainImageIndex);
    VkFence GetRenderFence(uint32_t frameIndex);
    VkFence GetUploadFence();

    void WaitForRenderFence(uint32_t frameIndex);
    void ResetRenderFence(uint32_t frameIndex);
    void WaitForUploadFence();
    void ResetUploadFence();
}