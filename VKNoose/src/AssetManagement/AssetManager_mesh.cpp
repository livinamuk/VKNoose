#include "AssetManager.h"

#include "Hell/Logging.h"
#include "ResourceManagement/ResourceManager.h"

#include <algorithm>
#include <unordered_map>

namespace AssetManager {
    extern std::vector<uint64_t> g_meshIds;

    namespace {
        std::unordered_map<std::string, uint64_t> g_meshIdsByName;

        MeshBuffer& GetStaticGeometry() {
            return ResourceManager::GetMeshBuffer(ResourceManager::STATIC_GEOMETRY_MESH_BUFFER_NAME);
        }
    }

    uint64_t CreateMesh(const std::string& name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, glm::vec3 aabbMin, glm::vec3 aabbMax, int parentIndex, glm::mat4 localTransform, glm::mat4 inverseBindTransform) {
        const uint64_t meshId = GetStaticGeometry().AddMesh(vertices, indices, name);
        Mesh* mesh = GetStaticGeometry().GetMeshById(meshId);

        if (!mesh) {
            Logging::Error() << "AssetManager::CreateMesh(..) failed to create mesh '" << name << "'\n";
            return 0;
        }

        mesh->aabbMin = aabbMin;
        mesh->aabbMax = aabbMax;
        mesh->extents = (aabbMax - aabbMin) * 0.5f;
        mesh->boundingSphereRadius = std::max(mesh->extents.x, std::max(mesh->extents.y, mesh->extents.z));
        mesh->parentIndex = parentIndex;
        mesh->localTransform = localTransform;
        mesh->inverseBindTransform = inverseBindTransform;

        g_meshIds.push_back(meshId);
        g_meshIdsByName[name] = meshId;
        return meshId;
    }

    uint64_t CreateMesh(const std::string& name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
        if (vertices.empty() || indices.empty()) {
            Logging::Error() << "AssetManager::CreateMesh(..) cannot create empty mesh '" << name << "'\n";
            return 0;
        }

        glm::vec3 aabbMin = vertices[0].position;
        glm::vec3 aabbMax = vertices[0].position;

        for (const Vertex& vertex : vertices) {
            aabbMin = glm::min(aabbMin, vertex.position);
            aabbMax = glm::max(aabbMax, vertex.position);
        }

        return CreateMesh(name, vertices, indices, aabbMin, aabbMax, -1, glm::mat4(1.0f), glm::mat4(1.0f));
    }

    uint64_t GetMeshIdByName(const std::string& name) {
        auto it = g_meshIdsByName.find(name);

        if (it != g_meshIdsByName.end()) {
            return it->second;
        }

        Logging::Error() << "AssetManager::GetMeshIdByName(..) failed because '" << name << "' does not exist\n";
        return 0;
    }

    Mesh* GetMeshByName(const std::string& name) {
        const uint64_t meshId = GetMeshIdByName(name);
        return meshId == 0 ? nullptr : GetMeshById(meshId);
    }

    Mesh* GetMeshById(uint64_t meshId) {
        Mesh* mesh = GetStaticGeometry().GetMeshById(meshId);

        if (!mesh) {
            Logging::Error() << "AssetManager::GetMeshById(..) failed because id '" << meshId << "' does not exist\n";
        }

        return mesh;
    }

    const std::string& GetMeshNameById(uint64_t meshId) {
        Mesh* mesh = GetMeshById(meshId);

        if (mesh) {
            return mesh->GetName();
        }

        static const std::string notFound = "NOT_FOUND";
        return notFound;
    }

    std::vector<Vertex> GetMeshVertices(Mesh* mesh) {
        if (!mesh) {
            Logging::Error() << "AssetManager::GetMeshVertices(..) failed: mesh was nullptr\n";
            return {};
        }

        std::vector<Vertex>& vertices = GetVertices();
        std::vector<uint32_t>& indices = GetIndices();
        std::vector<Vertex> result;
        result.reserve(mesh->vertexCount);

        for (uint32_t i = mesh->baseIndex; i < mesh->baseIndex + mesh->indexCount; i++) {
            const uint32_t index = indices[i];
            result.push_back(vertices[index + mesh->baseVertex]);
        }

        return result;
    }

    std::span<Vertex> GetMeshVerticesSpan(Mesh* mesh) {
        std::vector<Vertex>& vertices = GetVertices();

        if (!mesh || mesh->baseVertex < 0) {
            return {};
        }

        const size_t base = static_cast<size_t>(mesh->baseVertex);
        const size_t count = static_cast<size_t>(mesh->vertexCount);

        if (base > vertices.size() || count > vertices.size() - base) {
            return {};
        }

        return { vertices.data() + base, count };
    }

    std::span<uint32_t> GetMeshIndicesSpan(Mesh* mesh) {
        std::vector<uint32_t>& indices = GetIndices();

        if (!mesh) {
            return {};
        }

        const size_t base = static_cast<size_t>(mesh->baseIndex);
        const size_t count = static_cast<size_t>(mesh->indexCount);

        if (base > indices.size() || count > indices.size() - base) {
            return {};
        }

        return { indices.data() + base, count };
    }
}
