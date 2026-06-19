#pragma once
#include "Hell/Constants.h"
#include "Hell/VertexAttributes.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct GenericMesh {
    GenericMesh() = default;
    GenericMesh(const std::string& name);
    GenericMesh(const GenericMesh&) = delete;
    GenericMesh& operator=(const GenericMesh&) = delete;
    GenericMesh(GenericMesh&&) noexcept = default;
    GenericMesh& operator=(GenericMesh&&) noexcept = default;
    ~GenericMesh() = default;

    template<typename TVertex>
    void UpdateVertexData(const std::vector<TVertex>& vertices) {
        UpdateVertexData(vertices.data(), vertices.size(), TVertex::GetLayout());
    }

    template<typename TVertex>
    void UpdateVertexData(const std::vector<TVertex>& vertices, const std::vector<uint32_t>& indices) {
        UpdateVertexData(vertices);
        UpdateIndexData(indices);
    }

    void UpdateIndexData(const std::vector<uint32_t>& indices);
    void CleanUp();

    size_t GetVertexCount() const      { return m_vertexCount; }
    size_t GetIndexCount() const       { return m_indexCount; }
    const std::string& GetName() const { return m_name; }
    uint64_t GetOpenGLId() const       { return m_openGLId; }
    uint64_t GetVulkanId() const       { return m_vulkanId; }

    uint32_t GetVAO() const;

private:
    void UpdateVertexData(const void* vertices, size_t vertexCount, const VertexLayoutDescription& layout);

    std::string m_name = UNDEFINED_STRING;
    size_t m_vertexCount = 0;
    size_t m_indexCount = 0;
    uint64_t m_openGLId = 0;
    uint64_t m_vulkanId = 0;
};
