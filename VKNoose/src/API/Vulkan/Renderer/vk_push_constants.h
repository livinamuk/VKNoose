#pragma once
#include <cstdint>

struct ScenePushConstants {
    uint64_t instancesDeviceAddress = 0;
    uint64_t lightsDeviceAddress = 0;
    uint64_t cameraDeviceAddress = 0;
    uint64_t padding0 = 0;

    uint32_t lightCount;
    uint32_t padding1;
    uint32_t padding2;
    uint32_t padding3;
};

struct UIPushConstant {
    uint64_t instancesDeviceAddress;
    uint64_t padding0;
};