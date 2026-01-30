#pragma once
#include "Hell/Enums.h"
#include <cstdint>

inline constexpr uint32_t kTypeBits = 16;
inline constexpr uint64_t kTypeShift = 64 - kTypeBits;
inline constexpr uint64_t kTypeMask = ((1ull << kTypeBits) - 1) << kTypeShift;
inline constexpr uint64_t kLocalMask = ~kTypeMask;

namespace UniqueID {
    inline ObjectType GetType(uint64_t id) {
        return static_cast<ObjectType>((id >> kTypeShift) & ((1ull << kTypeBits) - 1));
    }

    inline uint64_t GetLocal(uint64_t id) {
        return id & kLocalMask;
    }

    uint64_t GetNextObjectId(ObjectType type);
    uint32_t GetNextCustomId();
    uint64_t GetNextPhysicsId();
}