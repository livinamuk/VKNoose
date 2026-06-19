#pragma once
#include "Hell/Constants.h"
#include "Hell/VertexAttributes.h"
#include "Types/Mesh.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

struct MeshBuffer {
    MeshBuffer() = default;
    MeshBuffer(const std::string& name);
    MeshBuffer(const MeshBuffer&) = delete;
    MeshBuffer& operator=(const MeshBuffer&) = delete;
    MeshBuffer(MeshBuffer&&) noexcept = default;
    MeshBuffer& operator=(MeshBuffer&&) noexcept = default;
    ~MeshBuffer() = default;

    void PreAllocate(size_t maxVertices, size_t maxIndices);
    void RemoveMesh(uint64_t meshIndex);
    void Reset();
    void CleanUp();
    void PrintDebugInfo();

    uint64_t AddMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::string& name = UNDEFINED_STRING);

    Mesh* GetMeshById(uint64_t meshId);
    std::span<Vertex> GetMeshVertexSpan(uint64_t meshId);
    std::span<uint32_t> GetMeshIndexSpan(uint64_t meshId);

    size_t GetMeshCount()                { return m_meshes.size(); }
    size_t GetAllocatedVertexCount()     { return m_vertices.size(); }
    size_t GetAllocatedIndexCount()      { return m_indices.size(); }
    std::vector<Vertex>& GetVertices()   { return m_vertices; }
    std::vector<uint32_t>& GetIndices()  { return m_indices; }

    uint64_t GetOpenGLId() const { return m_openGLId; }
    uint64_t GetVulkanId() const { return m_vulkanId; }

    // OpenGL
    uint32_t GetVAO() const;
    uint32_t GetVBO() const;
    uint32_t GetEBO() const;

private:
    struct MemoryBlock {
        size_t begin = 0;
        size_t end = 0;
        size_t GetSize() const { return end - begin; }
    };

    void Initialize();
    int32_t AllocateExtraVertexSpace(size_t vertexCount);
    int32_t AllocateExtraIndexSpace(size_t indexCount);
    int32_t AddVertices(const std::vector<Vertex>& newVertices);
    int32_t AddIndices(const std::vector<uint32_t>& newIndices);
    size_t CalculateNewCapacity(size_t requiredCount, size_t currentCapacity);

    std::string m_name = UNDEFINED_STRING;
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::unordered_map<uint64_t, Mesh> m_meshes;
    std::vector<MemoryBlock> m_freeVertexMemoryBlocks;
    std::vector<MemoryBlock> m_freeIndexMemoryBlocks;

    uint64_t m_nextMeshId = 0;
    size_t m_vertexCapacity = 0;
    size_t m_indexCapacity = 0;
    size_t m_minCapacity = 1024;
    bool m_initialized = false;
    float m_growthMultiplier = 1.0f;

    uint64_t m_openGLId = 0;
    uint64_t m_vulkanId = 0;
};
