#include "BackEnd.h"
#include "Input/Input.h"
#include "GLFWIntegration.h"
#include "API/Vulkan/vk_backend.h"

namespace BackEnd {
    void UpdateSubSystems();
    void LazyKeypresses();

    bool Init(WindowedMode windowedMode) {
        if (!GLFWIntegration::Init(windowedMode)) return false;
        if (!VulkanBackEnd::InitMinimum())        return false;
        return true;
    }

    void Update() {
        UpdateSubSystems();
        LazyKeypresses();
    }

    void UpdateSubSystems() {
        Input::Update();
    }

    void LazyKeypresses() {
        if (Input::KeyPressed(HELL_KEY_BACKSPACE) || Input::KeyPressed(HELL_KEY_ESCAPE)) {
            GLFWIntegration::ForceCloseWindow();
        }
    }

    // Window
    void* GetWindowPointer() {
        return GLFWIntegration::GetWindowPointer();;
    }

    const WindowedMode& GetWindowedMode() {
        return GLFWIntegration::GetWindowedMode();
    }

    void BackEnd::SetWindowedMode(const WindowedMode& windowedMode) {
        GLFWIntegration::SetWindowedMode(windowedMode);
    }

    void BackEnd::ToggleFullscreen() {
        GLFWIntegration::ToggleFullscreen();
    }

    void BackEnd::ForceCloseWindow() {
        GLFWIntegration::ForceCloseWindow();
    }

    bool BackEnd::WindowIsOpen() {
        return GLFWIntegration::WindowIsOpen();
    }

    bool BackEnd::WindowHasFocus() {
        return GLFWIntegration::WindowHasFocus();
    }

    bool BackEnd::WindowHasNotBeenForceClosed() {
        return GLFWIntegration::WindowHasNotBeenForceClosed();
    }

    bool BackEnd::WindowIsMinimized() {
        return GLFWIntegration::WindowIsMinimized();
    }

    int BackEnd::GetWindowedWidth() {
        return GLFWIntegration::GetWindowedWidth();
    }

    int BackEnd::GetWindowedHeight() {
        return GLFWIntegration::GetWindowedHeight();
    }

    int BackEnd::GetCurrentWindowWidth() {
        return GLFWIntegration::GetCurrentWindowWidth();
    }

    int BackEnd::GetCurrentWindowHeight() {
        return GLFWIntegration::GetCurrentWindowHeight();
    }

    int BackEnd::GetFullScreenWidth() {
        return GLFWIntegration::GetFullScreenWidth();
    }

    int BackEnd::GetFullScreenHeight() {
        return GLFWIntegration::GetFullScreenHeight();
    }
}