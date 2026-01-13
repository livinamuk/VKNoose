#pragma once
#include "API/Vulkan/vk_common.h"
#include <vector>

namespace VulkanSwapchainManager {
    bool Init();
    void CleanUp();

    void CreateSwapchain();
    void RecreateSwapchain();

    VkSwapchainKHR GetSwapchain();
    VkFormat GetSwapchainImageFormat();
    std::vector<VkImage>& GetSwapchainImages();
    std::vector<VkImageView>& GetSwapchainImageViews();
}
