#pragma once
#include <cstdint>

struct DynamicDeviceAddresses {
    uint64_t sceneCameraData = 0;
    uint64_t inventoryCameraData = 0;
    uint64_t uiInstances = 0;
};

struct StaticDeviceAddresses {
    uint64_t globalVertexBuffer = 0;
    uint64_t globalIndexBuffer = 0;
};
