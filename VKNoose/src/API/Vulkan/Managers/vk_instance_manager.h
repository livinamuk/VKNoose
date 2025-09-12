#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace VulkanInstanceManager {
    bool Init(const char** extraExtensions = nullptr, uint32_t extraCount = 0, bool enableValidation = true);
    void CleanUp();

    VkInstance GetInstance();
    uint32_t GetApiVersion();
    bool ValidationEnabled();

    VkSurfaceKHR CreateSurface(void* windowHandle);
}