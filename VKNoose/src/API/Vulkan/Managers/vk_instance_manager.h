#pragma once
#include "API/Vulkan/vk_common.h"

struct GLFWwindow;

namespace VulkanInstanceManager {
    bool Init();
    void Cleanup(); // TODO: Currently cleaned up in vk_backend.cpp

    VkDebugUtilsMessengerEXT GetDebugMessenger();
    VkInstance GetInstance();
    VkSurfaceKHR GetSurface();

    bool ValidationEnabled();
}