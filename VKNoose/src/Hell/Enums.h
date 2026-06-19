#pragma once
#include <cstdint>

enum class API {
    OPENGL,
    VULKAN,
    UNDEFINED
};

enum class ObjectType : uint16_t {
    GL_OBJECT,
    VK_ACCELERATION_STRUCTURE,
    VK_BUFFER,
    UNDEFINED
};

enum class WindowedMode {
    WINDOWED,
    FULLSCREEN
};