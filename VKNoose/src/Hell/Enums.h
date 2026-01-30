#pragma once
#include <cstdint>

enum class ObjectType : uint16_t {
    INVALID = 0,

    GL_OBJECT,

    VK_ACCELERATION_STRUCTURE,
    VK_DESCRIPTOR_SET,
    VK_BUFFER
};