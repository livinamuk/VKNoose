#pragma once
#include <glm/glm.hpp>
#include <string>
#include "API/Vulkan/Types/vk_acceleration_structure.h"

struct Mesh {
    int32_t baseVertex = 0;
    uint32_t baseIndex = 0;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    uint64_t meshBvhId = 0;
    int32_t parentIndex = -1;
    glm::vec3 aabbMin = glm::vec3(0);
    glm::vec3 aabbMax = glm::vec3(0);
    glm::vec3 extents = glm::vec3(0);
    float boundingSphereRadius = 0;
    glm::mat4 localTransform = glm::mat4(1.0f);
    glm::mat4 inverseBindTransform = glm::mat4(1.0f);
    VulkanAccelerationStructure m_vulkanAccelerationStructure;

    void SetName(const std::string& name);

    int32_t GetBaseVertex() const       { return baseVertex; }
    int32_t GetBaseIndex() const        { return baseIndex; }
    int32_t GetVertexCount() const      { return vertexCount; }
    int32_t GetIndexCount() const       { return indexCount; }
    const std::string& GetName() const  { return name; }

private:
    std::string name = "undefined";
};
