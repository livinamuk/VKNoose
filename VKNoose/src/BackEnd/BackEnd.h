#pragma once
#include "Hell/Enums.h"

namespace BackEnd {
    bool Init(WindowedMode windowedMode);
    void Update();

    void LazyKeypresses();

    void SetAPI(API api);
    const API GetAPI();

    // Window
    void* GetWindowPointer();
    void SetWindowedMode(const WindowedMode& windowedMode);
    void ToggleFullscreen();
    void ForceCloseWindow();
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
}