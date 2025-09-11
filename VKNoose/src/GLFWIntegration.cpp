#include "GLFWIntegration.h"
#define NOMINMAX
#include <Windows.h>
#ifdef _MSC_VER
#undef GetObject
#endif
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <iostream>

namespace GLFWIntegration {
    GLFWwindow* g_window = NULL;
    WindowedMode g_windowedMode = WindowedMode::WINDOWED;
    GLFWmonitor* g_monitor;
    const GLFWvidmode* g_mode;
    bool g_forceCloseWindow = false;
    bool g_windowHasFocus = true;
    int g_windowedWidth = 0;
    int g_windowedHeight = 0;
    int g_fullscreenWidth = 0;
    int g_fullscreenHeight = 0;
    int g_currentWindowWidth = 0;
    int g_currentWindowHeight = 0;

    //void framebuffer_size_callback(GLFWwindow* window, int width, int height);

    bool Init(WindowedMode windowedMode) {
        glfwInit();
        g_monitor = glfwGetPrimaryMonitor();
        g_mode = glfwGetVideoMode(g_monitor);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_RED_BITS, g_mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, g_mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, g_mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, g_mode->refreshRate);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        g_fullscreenWidth = g_mode->width;
        g_fullscreenHeight = g_mode->height;
        g_windowedWidth = g_fullscreenWidth * 0.75f;
        g_windowedHeight = g_fullscreenHeight * 0.75f;

        // Create window
        g_windowedMode = windowedMode;
        if (g_windowedMode == WindowedMode::WINDOWED) {
            g_currentWindowWidth = g_windowedWidth;
            g_currentWindowHeight = g_windowedHeight;
        }
        else if (windowedMode == WindowedMode::FULLSCREEN) {
            g_currentWindowWidth = g_fullscreenWidth;
            g_currentWindowHeight = g_fullscreenHeight;
        }
        g_window = glfwCreateWindow(g_currentWindowWidth, g_currentWindowHeight, "Unloved", NULL, NULL);
        
        if (g_window == NULL) {
            std::cout << "GLFWIntegration() Failed to create window\n";
            glfwTerminate();
            return false;
        }
        
        glfwSetWindowPos(g_window, 0, 0);
        glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        //glfwSetFramebufferSizeCallback(g_window, framebuffer_size_callback);
        //glfwSetWindowUserPointer(g_window, this); // THIS MIGHT NEED SORTING???
    }

    void SetWindowedMode(const WindowedMode& windowedMode) {
        g_windowedMode = windowedMode;

        if (windowedMode == WindowedMode::WINDOWED) {
            g_currentWindowWidth = g_windowedWidth;
            g_currentWindowHeight = g_windowedHeight;
        }
        else if (windowedMode == WindowedMode::FULLSCREEN) {
            g_currentWindowWidth = g_fullscreenWidth - 1;
            g_currentWindowHeight = g_fullscreenHeight - 1;
        }

        glfwSetWindowMonitor(g_window, nullptr, 0, 0, g_currentWindowWidth, g_currentWindowHeight, g_mode->refreshRate);
        glfwSetWindowPos(g_window, 0, 0);
    }

    void ToggleFullscreen() {
        if (g_windowedMode == WindowedMode::WINDOWED) {
            SetWindowedMode(WindowedMode::FULLSCREEN);
        }
        else {
            SetWindowedMode(WindowedMode::WINDOWED);
        }
    }

    void ForceCloseWindow() {
        g_forceCloseWindow = true;
    }

    bool WindowHasFocus() {
        return g_windowHasFocus;
    }

    bool WindowHasNotBeenForceClosed() {
        return !g_forceCloseWindow;
    }

    int GetWindowedWidth() {
        return g_windowedWidth;
    }

    int GetWindowedHeight() {
        return g_windowedHeight;
    }

    int GetFullScreenWidth() {
        return g_fullscreenWidth;
    }

    int GetFullScreenHeight() {
        return g_fullscreenHeight;
    }

    int GetCurrentWindowWidth() {
        return g_currentWindowWidth;
    }

    int GetCurrentWindowHeight() {
        return g_currentWindowHeight;
    }

    bool WindowIsOpen() {
        return !(glfwWindowShouldClose(g_window) || g_forceCloseWindow);
    }

    bool WindowIsMinimized() {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(g_window, &width, &height);
        return (width == 0 || height == 0);
    }

    const WindowedMode& GetWindowedMode() {
        return g_windowedMode;
    }

    void* GetWindowPointer() {
        return g_window;
    }

    //void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height) {
    //    // Nothing as of yet
    //}
}