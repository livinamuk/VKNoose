#pragma once
#include "API/Vulkan/vk_common.h"

struct ScenePushConstants {
    VkDeviceAddress verticesPtr;
    VkDeviceAddress indicesPtr;
    VkDeviceAddress instancesPtr;
    VkDeviceAddress lightsPtr;
    VkDeviceAddress sceneDataPtr;
    uint32_t lightCount;
    uint32_t padding0;
};