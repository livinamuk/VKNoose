#include "vk_command_manager.h"
#include "vk_device_manager.h"
#include "vk_sync_manager.h"

#include "API/Vulkan/vk_backend.h"
#include "API/Vulkan/vk_initializers.h"

#include <iostream>

namespace VulkanCommandManager {
    struct CommandData {
        VkCommandPool graphicsPool;
        VkCommandBuffer graphicsBuffer;
    };

    CommandData g_frames[FRAME_OVERLAP];

    VkCommandPool g_uploadPool = VK_NULL_HANDLE;
    VkCommandBuffer g_uploadBuffer = VK_NULL_HANDLE;

    bool Init() {
        VkDevice device = VulkanDeviceManager::GetDevice();
        uint32_t graphicsFamily = VulkanDeviceManager::GetGraphicsQueueFamily();

        // Create per-frame graphics pools and buffers
        VkCommandPoolCreateInfo poolInfo = vkinit::command_pool_create_info(graphicsFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            if (vkCreateCommandPool(device, &poolInfo, nullptr, &g_frames[i].graphicsPool) != VK_SUCCESS) {
                return false;
            }

            VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(g_frames[i].graphicsPool, 1);
            if (vkAllocateCommandBuffers(device, &cmdAllocInfo, &g_frames[i].graphicsBuffer) != VK_SUCCESS) {
                return false;
            }
        }

        // Create upload context pool and buffer
        VkCommandPoolCreateInfo uploadPoolInfo = vkinit::command_pool_create_info(graphicsFamily);
        if (vkCreateCommandPool(device, &uploadPoolInfo, nullptr, &g_uploadPool) != VK_SUCCESS) {
            return false;
        }

        VkCommandBufferAllocateInfo uploadCmdAllocInfo = vkinit::command_buffer_allocate_info(g_uploadPool, 1);
        if (vkAllocateCommandBuffers(device, &uploadCmdAllocInfo, &g_uploadBuffer) != VK_SUCCESS) {
            return false;
        }

        std::cout << "VulkanCommandManager::Init()\n";
        return true;
    }

    void Cleanup() {
        VkDevice device = VulkanDeviceManager::GetDevice();

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            vkDestroyCommandPool(device, g_frames[i].graphicsPool, nullptr);
        }
        vkDestroyCommandPool(device, g_uploadPool, nullptr);
    }

    VkCommandPool GetGraphicsCommandPool(uint32_t frameIndex) {
        return g_frames[frameIndex].graphicsPool;
    }

    VkCommandBuffer GetGraphicsCommandBuffer(uint32_t frameIndex) {
        return g_frames[frameIndex].graphicsBuffer;
    }

    VkCommandPool GetUploadCommandPool() {
        return g_uploadPool;
    }

    VkCommandBuffer GetUploadCommandBuffer() {
        return g_uploadBuffer;
    }

    void SubmitImmediate(std::function<void(VkCommandBuffer cmd)>&& function) {
        VkCommandBuffer cmd = g_uploadBuffer;

        VkCommandBufferBeginInfo beginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

        function(cmd);

        VK_CHECK(vkEndCommandBuffer(cmd));

        VkSubmitInfo submit = vkinit::submit_info(&cmd);
        VkFence uploadFence = VulkanSyncManager::GetUploadFence();

        VulkanSyncManager::ResetUploadFence();
        VK_CHECK(vkQueueSubmit(VulkanDeviceManager::GetGraphicsQueue(), 1, &submit, uploadFence));

        VulkanSyncManager::WaitForUploadFence();
        vkResetCommandPool(VulkanDeviceManager::GetDevice(), g_uploadPool, 0);
    }
}