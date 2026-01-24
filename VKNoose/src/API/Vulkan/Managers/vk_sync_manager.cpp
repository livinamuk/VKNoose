#include "vk_sync_manager.h"
#include "vk_device_manager.h"
#include "vk_swapchain_manager.h"
#include "API/Vulkan/vk_backend.h"
#include <iostream>

namespace VulkanSyncManager {
    struct FrameSyncData {
        VkSemaphore presentSemaphore;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        VkFence renderFence;
    };

    FrameSyncData g_frames[FRAME_OVERLAP];
    VkFence g_uploadFence = VK_NULL_HANDLE;

    bool Init() {
        VkDevice device = VulkanDeviceManager::GetDevice();
        uint32_t swapchainImageCount = static_cast<uint32_t>(VulkanSwapchainManager::GetSwapchainImages().size());

        VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

        VkFenceCreateInfo signaledFenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        signaledFenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint32_t i = 0; i < FRAME_OVERLAP; i++) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &g_frames[i].presentSemaphore) != VK_SUCCESS ||
                vkCreateFence(device, &signaledFenceInfo, nullptr, &g_frames[i].renderFence) != VK_SUCCESS) {
                return false;
            }

            g_frames[i].renderFinishedSemaphores.resize(swapchainImageCount);
            for (uint32_t j = 0; j < swapchainImageCount; j++) {
                if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &g_frames[i].renderFinishedSemaphores[j]) != VK_SUCCESS) {
                    return false;
                }
            }
        }

        // IMPORTANT: Create upload fence signaled so the first immediate_submit wait doesn't fail
        if (vkCreateFence(device, &signaledFenceInfo, nullptr, &g_uploadFence) != VK_SUCCESS) {
            return false;
        }

        std::cout << "VulkanSyncManager::Init()\n";
        return true;
    }

    void Cleanup() {
        VkDevice device = VulkanDeviceManager::GetDevice();

        for (uint32_t i = 0; i < FRAME_OVERLAP; i++) {
            vkDestroySemaphore(device, g_frames[i].presentSemaphore, nullptr);
            vkDestroyFence(device, g_frames[i].renderFence, nullptr);

            for (VkSemaphore semaphore : g_frames[i].renderFinishedSemaphores) {
                vkDestroySemaphore(device, semaphore, nullptr);
            }
        }

        vkDestroyFence(device, g_uploadFence, nullptr);
    }

    VkSemaphore GetPresentSemaphore(uint32_t frameIndex) {
        return g_frames[frameIndex].presentSemaphore;
    }

    VkSemaphore GetRenderFinishedSemaphore(uint32_t frameIndex, uint32_t swapchainImageIndex) {
        return g_frames[frameIndex].renderFinishedSemaphores[swapchainImageIndex];
    }

    VkFence GetRenderFence(uint32_t frameIndex) {
        return g_frames[frameIndex].renderFence;
    }

    VkFence GetUploadFence() {
        return g_uploadFence;
    }

    void WaitForRenderFence(uint32_t frameIndex) {
        vkWaitForFences(VulkanDeviceManager::GetDevice(), 1, &g_frames[frameIndex].renderFence, VK_TRUE, UINT64_MAX);
    }

    void ResetRenderFence(uint32_t frameIndex) {
        vkResetFences(VulkanDeviceManager::GetDevice(), 1, &g_frames[frameIndex].renderFence);
    }

    void WaitForUploadFence() {
        vkWaitForFences(VulkanDeviceManager::GetDevice(), 1, &g_uploadFence, VK_TRUE, UINT64_MAX);
    }

    void ResetUploadFence() {
        vkResetFences(VulkanDeviceManager::GetDevice(), 1, &g_uploadFence);
    }
}