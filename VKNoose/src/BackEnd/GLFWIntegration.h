#pragma once
#include "NooseEnums.h"

namespace GLFWIntegration {
    bool Init(WindowedMode windowedMode);
    void SetWindowedMode(const WindowedMode& windowedMode);
    void ToggleFullscreen();
    void ToggleFullscreen();
    void ForceCloseWindow();
    void WaitUntilNotMinimized();

    bool WindowIsOpen();
    bool WindowHasFocus();
    bool WindowHasNotBeenForceClosed();
    bool WindowIsMinimized();

    int GetWindowedWidth();
    int GetWindowedHeight();
    int GetCurrentWindowWidth();
    int GetCurrentWindowHeight();
    int GetFullScreenWidth();
    int GetFullScreenHeight();

    const WindowedMode& GetWindowedMode();
    void* GetWindowPointer();
}