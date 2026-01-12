#include "vk_instance_manager.h"
#include "BackEnd/GLFWIntegration.h"

#include "VkBootstrap.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <cstring>
#include <stdexcept>

#include <iostream>

#include "../vk_backend.h"

namespace VulkanInstanceManager {
    VkInstance g_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT g_debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR g_surface = VK_NULL_HANDLE;
    bool g_validationEnabled = true;

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
        (void)messageSeverity;
        (void)messageTypes;
        (void)userData;
        std::cout << callbackData->pMessage << "\n";
        return VK_FALSE;
    }

    bool Init() {
        vkb::InstanceBuilder builder;
        builder.set_app_name("Unloved");
        builder.request_validation_layers(g_validationEnabled);
        builder.use_default_debug_messenger();
        builder.require_api_version(1, 3, 0);

        vkb::Instance& bootstrapInstance = VulkanBackEnd::GetBootstrapInstance();
        bootstrapInstance = builder.build().value();
        g_instance = bootstrapInstance.instance;
        g_debugMessenger = bootstrapInstance.debug_messenger;

        // Create surface
        GLFWwindow* window = (GLFWwindow*)GLFWIntegration::GetWindowPointer();
        glfwCreateWindowSurface(g_instance, window, nullptr, &g_surface);

        std::cout << "VulkanInstanceManager::Init()\n";
        return true;
    }

    bool InitBroken() {
        // Required GLFW instance extensions
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        if (glfwExtensions == nullptr || glfwExtensionCount == 0) {
            throw std::runtime_error("glfwGetRequiredInstanceExtensions failed");
        }

        std::vector<const char*> instanceExtensions;
        instanceExtensions.reserve(glfwExtensionCount + 4);
        for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
            instanceExtensions.push_back(glfwExtensions[i]);
        }

        if (g_validationEnabled) {
            instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        // Validation layers (enable only if available)
        std::vector<const char*> instanceLayers;
        if (g_validationEnabled) {
            uint32_t layerCount = 0;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
            std::vector<VkLayerProperties> layers(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

            bool hasKhronosValidation = false;
            for (const VkLayerProperties& lp : layers) {
                if (strcmp(lp.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
                    hasKhronosValidation = true;
                    break;
                }
            }

            if (hasKhronosValidation) {
                instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
            }
            else {
                // If you prefer hard fail, throw here instead.
                g_validationEnabled = false;
            }
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Unloved";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Unloved";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();
        createInfo.enabledLayerCount = (uint32_t)instanceLayers.size();
        createInfo.ppEnabledLayerNames = instanceLayers.data();

        if (vkCreateInstance(&createInfo, nullptr, &g_instance) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateInstance failed");
        }

        // Create debug messenger (optional)
        g_debugMessenger = VK_NULL_HANDLE;
        if (g_validationEnabled) {
            PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT_fn =
                (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_instance, "vkCreateDebugUtilsMessengerEXT");

            if (vkCreateDebugUtilsMessengerEXT_fn) {
                VkDebugUtilsMessengerCreateInfoEXT dbgInfo{};
                dbgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
                dbgInfo.messageSeverity =
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                dbgInfo.messageType =
                    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

                // You need to provide this callback somewhere in your manager cpp
                dbgInfo.pfnUserCallback = DebugCallback;
                dbgInfo.pUserData = nullptr;

                // If this fails, you still have a valid instance, just no messenger.
                vkCreateDebugUtilsMessengerEXT_fn(g_instance, &dbgInfo, nullptr, &g_debugMessenger);
            }
        }

        // Create surface
        GLFWwindow* window = (GLFWwindow*)GLFWIntegration::GetWindowPointer();
        if (glfwCreateWindowSurface(g_instance, window, nullptr, &g_surface) != VK_SUCCESS) {
            throw std::runtime_error("glfwCreateWindowSurface failed");
        }

        std::cout << "VulkanInstanceManager::Init()\n";
        return true;
    }

    void CleanUp() {
        // TODO: Currently cleaned up in vk_backend.cpp
    }

    VkDebugUtilsMessengerEXT GetDebugMessenger() {
        return g_debugMessenger;
    }

    VkInstance GetInstance() {
        return g_instance;
    }

    VkSurfaceKHR GetSurface() {
        return g_surface;
    }

    bool ValidationEnabled() {
        return g_validationEnabled;
    }
}