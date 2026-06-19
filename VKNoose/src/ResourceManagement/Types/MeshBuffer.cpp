#include "MeshBuffer.h"

//#include "API/OpenGL/GL_resource_manager.h"
#include "Backend/BackEnd.h"
#include "Hell/Logging.h"
#include "Util/Util.h"

#include <algorithm>
#include <limits>
#include <utility>

MeshBuffer::MeshBuffer(const std::string& name) {
    m_name = name;
}

void MeshBuffer::Initialize() {
    Reset();

    if (BackEnd::GetAPI() == API::OPENGL) {
        //if (m_openGLId == 0) {
        //    m_openGLId = OpenGLResourceManager::CreateMeshBuffer();
        //}
        //
        //OpenGLMeshBuffer& meshBuffer = OpenGLResourceManager::GetMeshBuffer(m_openGLId);
        //meshBuffer.Init(Vertex::GetLayout());
    }

    m_initialized = true;
}

void MeshBuffer::Reset() {
    m_meshes.clear();
    m_vertices.clear();
    m_indices.clear();

    m_freeVertexMemoryBlocks.clear();
    m_freeIndexMemoryBlocks.clear();

    m_nextMeshId = 0;
    m_vertexCapacity = 0;
    m_indexCapacity = 0;

    if (BackEnd::GetAPI() == API::OPENGL && m_openGLId != 0) {
        //OpenGLMeshBuffer& meshBuffer = OpenGLResourceManager::GetMeshBuffer(m_openGLId);
        //meshBuffer.Reset();
    }

    m_initialized = false;
}

void MeshBuffer::CleanUp() {
    Reset();

    //if (m_openGLId != 0) {
    //    OpenGLResourceManager::RemoveMeshBuffer(m_openGLId);
    //    m_openGLId = 0;
    //}
}

uint64_t MeshBuffer::AddMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::string& name) {
    if (!m_initialized) Initialize();

    if (vertices.empty() || indices.empty()) return 0;

    m_nextMeshId++;

    Mesh& mesh = m_meshes[m_nextMeshId];
    mesh.baseVertex = AddVertices(vertices);
    mesh.baseIndex = AddIndices(indices);
    mesh.vertexCount = static_cast<uint32_t>(vertices.size());
    mesh.indexCount = static_cast<uint32_t>(indices.size());
    mesh.SetName(name);

    // Compute axis aligned bounding box limits
    glm::vec3 aabbMin(std::numeric_limits<float>::max());
    glm::vec3 aabbMax(std::numeric_limits<float>::lowest());

    for (const Vertex& vertex : vertices) {
        aabbMin = glm::min(aabbMin, vertex.position);
        aabbMax = glm::max(aabbMax, vertex.position);
    }

    mesh.aabbMin = aabbMin;
    mesh.aabbMax = aabbMax;
    mesh.extents = (aabbMax - aabbMin) * 0.5f;

    return m_nextMeshId;
}

int32_t MeshBuffer::AddVertices(const std::vector<Vertex>& newVertices) {
    int32_t freeMemoryBlockIndex = -1;
    int32_t insertOffset = 0;

    // Search for free vertex block
    for (int32_t i = 0; i < static_cast<int32_t>(m_freeVertexMemoryBlocks.size()); i++) {
        MemoryBlock& memoryBlock = m_freeVertexMemoryBlocks[i];

        if (newVertices.size() <= memoryBlock.GetSize()) {
            freeMemoryBlockIndex = i;
            break;
        }
    }

    // Allocate memory and insert data
    if (freeMemoryBlockIndex == -1) {
        insertOffset = static_cast<uint32_t>(m_vertices.size());
        size_t requiredCount = m_vertices.size() + newVertices.size();

        // Grow capacity
        size_t newCount = std::max(requiredCount, static_cast<size_t>(m_vertices.size() * m_growthMultiplier));

        if (newCount > m_vertexCapacity) {
            m_vertexCapacity = CalculateNewCapacity(newCount, m_vertexCapacity);

            if (BackEnd::GetAPI() == API::OPENGL) {
                //OpenGLMeshBuffer& meshBuffer = OpenGLResourceManager::GetMeshBuffer(m_openGLId);
                //meshBuffer.ResizeVertexBuffer(m_vertexCapacity, m_vertices);
            }
        }

        m_vertices.resize(newCount);
        std::copy(newVertices.begin(), newVertices.end(), m_vertices.begin() + insertOffset);

        if (BackEnd::GetAPI() == API::OPENGL) {
            //OpenGLMeshBuffer& meshBuffer = OpenGLResourceManager::GetMeshBuffer(m_openGLId);
            //meshBuffer.InsertVertices(newVertices, insertOffset);
        }

        // Register newly allocated excess space as a free block
        if (newCount > requiredCount) {
            MemoryBlock extraBlock;
            extraBlock.begin = static_cast<uint32_t>(requiredCount);
            extraBlock.end = static_cast<uint32_t>(newCount);
            m_freeVertexMemoryBlocks.push_back(extraBlock);
        }
    }

    // Insert into free memory
    else {
        MemoryBlock& memoryBlock = m_freeVertexMemoryBlocks[freeMemoryBlockIndex];
        insertOffset = static_cast<int32_t>(memoryBlock.begin);

        std::copy(newVertices.begin(), newVertices.end(), m_vertices.begin() + insertOffset);

        if (BackEnd::GetAPI() == API::OPENGL) {
            //OpenGLMeshBuffer& meshBuffer = OpenGLResourceManager::GetMeshBuffer(m_openGLId);
            //meshBuffer.InsertVertices(newVertices, insertOffset);
        }

        // Handle block resizing
        if (memoryBlock.GetSize() == newVertices.size()) {
            m_freeVertexMemoryBlocks.erase(m_freeVertexMemoryBlocks.begin() + freeMemoryBlockIndex);
        }
        else {
            memoryBlock.begin += newVertices.size();
        }
    }

    return insertOffset;
}

int32_t MeshBuffer::AddIndices(const std::vector<uint32_t>& newIndices) {
    int32_t freeMemoryBlockIndex = -1;
    int32_t insertOffset = 0;

    // Search for free index block
    for (int32_t i = 0; i < static_cast<int32_t>(m_freeIndexMemoryBlocks.size()); i++) {
        MemoryBlock& memoryBlock = m_freeIndexMemoryBlocks[i];

        if (newIndices.size() <= memoryBlock.GetSize()) {
            freeMemoryBlockIndex = i;
            break;
        }
    }

    // Allocate memory and insert data
    if (freeMemoryBlockIndex == -1) {
        insertOffset = static_cast<uint32_t>(m_indices.size());
        size_t requiredCount = m_indices.size() + newIndices.size();

        // Grow capacity
        size_t newCount = std::max(requiredCount, static_cast<size_t>(m_indices.size() * m_growthMultiplier));

        if (newCount > m_indexCapacity) {
            m_indexCapacity = CalculateNewCapacity(newCount, m_indexCapacity);

            if (BackEnd::GetAPI() == API::OPENGL) {
                //OpenGLMeshBuffer& meshBuffer = OpenGLResourceManager::GetMeshBuffer(m_openGLId);
                //meshBuffer.ResizeIndexBuffer(m_indexCapacity, m_indices);
            }
        }

        m_indices.resize(newCount);
        std::copy(newIndices.begin(), newIndices.end(), m_indices.begin() + insertOffset);

        if (BackEnd::GetAPI() == API::OPENGL) {
            //OpenGLMeshBuffer& meshBuffer = OpenGLResourceManager::GetMeshBuffer(m_openGLId);
            //meshBuffer.InsertIndices(newIndices, insertOffset);
        }

        // Register newly allocated excess space as a free block
        if (newCount > requiredCount) {
            MemoryBlock extraBlock;
            extraBlock.begin = static_cast<uint32_t>(requiredCount);
            extraBlock.end = static_cast<uint32_t>(newCount);
            m_freeIndexMemoryBlocks.push_back(extraBlock);
        }
    }

    // Insert into free memory
    else {
        MemoryBlock& memoryBlock = m_freeIndexMemoryBlocks[freeMemoryBlockIndex];
        insertOffset = static_cast<int32_t>(memoryBlock.begin);

        std::copy(newIndices.begin(), newIndices.end(), m_indices.begin() + insertOffset);

        if (BackEnd::GetAPI() == API::OPENGL) {
            //OpenGLMeshBuffer& meshBuffer = OpenGLResourceManager::GetMeshBuffer(m_openGLId);
            //meshBuffer.InsertIndices(newIndices, insertOffset);
        }

        // Handle block resizing
        if (memoryBlock.GetSize() == newIndices.size()) {
            m_freeIndexMemoryBlocks.erase(m_freeIndexMemoryBlocks.begin() + freeMemoryBlockIndex);
        }
        else {
            memoryBlock.begin += newIndices.size();
        }
    }

    return insertOffset;
}

int32_t MeshBuffer::AllocateExtraVertexSpace(size_t vertexCount) {
    size_t blockBegin = m_vertices.size();
    size_t blockEnd = m_vertices.size() + vertexCount;

    if (blockEnd > m_vertexCapacity) {
        m_vertexCapacity = CalculateNewCapacity(blockEnd, m_vertexCapacity);

        if (BackEnd::GetAPI() == API::OPENGL) {
            //OpenGLMeshBuffer& meshBuffer = OpenGLResourceManager::GetMeshBuffer(m_openGLId);
            //meshBuffer.ResizeVertexBuffer(m_vertexCapacity, m_vertices);
        }
    }

    MemoryBlock& block = m_freeVertexMemoryBlocks.emplace_back();
    block.begin = blockBegin;
    block.end = blockEnd;

    m_vertices.resize(blockEnd);

    return static_cast<int32_t>(m_freeVertexMemoryBlocks.size() - 1);
}

int32_t MeshBuffer::AllocateExtraIndexSpace(size_t indexCount) {
    size_t blockBegin = m_indices.size();
    size_t blockEnd = m_indices.size() + indexCount;

    if (blockEnd > m_indexCapacity) {
        m_indexCapacity = CalculateNewCapacity(blockEnd, m_indexCapacity);

        if (BackEnd::GetAPI() == API::OPENGL) {
            //OpenGLMeshBuffer& meshBuffer = OpenGLResourceManager::GetMeshBuffer(m_openGLId);
            //meshBuffer.ResizeIndexBuffer(m_indexCapacity, m_indices);
        }
    }

    MemoryBlock& block = m_freeIndexMemoryBlocks.emplace_back();
    block.begin = blockBegin;
    block.end = blockEnd;

    m_indices.resize(blockEnd);

    return static_cast<int32_t>(m_freeIndexMemoryBlocks.size() - 1);
}

uint32_t MeshBuffer::GetVBO() const {
    //if (BackEnd::GetAPI() == API::OPENGL) {
    //    OpenGLMeshBuffer& meshBuffer = OpenGLResourceManager::GetMeshBuffer(m_openGLId);
    //    return meshBuffer.GetVBO();
    //}
    //else {
    //    Logging::Error() << "MeshBuffer::GetVBO() was called by API is Vulkan\n";
    //    return 0;
    //}
    return 0;
}

uint32_t MeshBuffer::GetEBO() const {
    //if (BackEnd::GetAPI() == API::OPENGL) {
    //    OpenGLMeshBuffer& meshBuffer = OpenGLResourceManager::GetMeshBuffer(m_openGLId);
    //    return meshBuffer.GetEBO();
    //}
    //else {
    //    Logging::Error() << "MeshBuffer::GetEBO() was called by API is Vulkan\n";
    //    return 0;
    //}
    return 0;
}

uint32_t MeshBuffer::GetVAO() const {
    //if (BackEnd::GetAPI() == API::OPENGL) {
    //    OpenGLMeshBuffer& meshBuffer = OpenGLResourceManager::GetMeshBuffer(m_openGLId);
    //    return meshBuffer.GetVAO();
    //}
    //else {
    //    Logging::Error() << "MeshBuffer::GetVAO() was called by API is Vulkan\n";
    //    return 0;
    //}
    return 0;
}

void MeshBuffer::RemoveMesh(uint64_t meshIndex) {
    auto it = m_meshes.find(meshIndex);
    if (it == m_meshes.end()) return;

    Mesh& mesh = it->second;

    // Create free vertex memory block
    MemoryBlock vertexBlock;
    vertexBlock.begin = mesh.baseVertex;
    vertexBlock.end = mesh.baseVertex + mesh.vertexCount;
    m_freeVertexMemoryBlocks.push_back(vertexBlock);

    // Sort vertex blocks by starting offset
    std::sort(m_freeVertexMemoryBlocks.begin(), m_freeVertexMemoryBlocks.end(), [](const MemoryBlock& a, const MemoryBlock& b) {
        return a.begin < b.begin;
    });

    // Merge adjacent free vertex memory blocks
    std::vector<MemoryBlock> mergedVertexBlocks;

    for (const MemoryBlock& block : m_freeVertexMemoryBlocks) {
        if (mergedVertexBlocks.empty()) {
            mergedVertexBlocks.push_back(block);
        }
        else {
            MemoryBlock& last = mergedVertexBlocks.back();
            if (last.end >= block.begin) {
                last.end = std::max(last.end, block.end);
            }
            else {
                mergedVertexBlocks.push_back(block);
            }
        }
    }
    m_freeVertexMemoryBlocks = std::move(mergedVertexBlocks);

    // Create free index memory block
    MemoryBlock indexBlock;
    indexBlock.begin = mesh.baseIndex;
    indexBlock.end = mesh.baseIndex + mesh.indexCount;
    m_freeIndexMemoryBlocks.push_back(indexBlock);

    // Sort index blocks by starting offset
    std::sort(m_freeIndexMemoryBlocks.begin(), m_freeIndexMemoryBlocks.end(), [](const MemoryBlock& a, const MemoryBlock& b) {
        return a.begin < b.begin;
    });

    // Merge adjacent free index memory blocks
    std::vector<MemoryBlock> mergedIndexBlocks;

    for (const MemoryBlock& block : m_freeIndexMemoryBlocks) {
        if (mergedIndexBlocks.empty()) {
            mergedIndexBlocks.push_back(block);
        }
        else {
            MemoryBlock& last = mergedIndexBlocks.back();
            if (last.end >= block.begin) {
                last.end = std::max(last.end, block.end);
            }
            else {
                mergedIndexBlocks.push_back(block);
            }
        }
    }
    m_freeIndexMemoryBlocks = std::move(mergedIndexBlocks);

    // Remove the mesh
    m_meshes.erase(it);
}

void MeshBuffer::PreAllocate(size_t maxVertices, size_t maxIndices) {
    if (maxVertices == 0 || maxIndices == 0) {
        Logging::Warning() << "MeshBuffer::PreAllocate() called with zero " << maxVertices << " vertices and " << maxIndices << " indices\n";
        return;
    }

    Initialize();

    m_vertices.resize(maxVertices);
    m_indices.resize(maxIndices);
    m_vertexCapacity = maxVertices;
    m_indexCapacity = maxIndices;

    // Allocate new GPU memory
    if (BackEnd::GetAPI() == API::OPENGL) {
        //OpenGLMeshBuffer& meshBuffer = OpenGLResourceManager::GetMeshBuffer(m_openGLId);
        //meshBuffer.PreAllocate(m_vertexCapacity, m_indexCapacity);
    }

    // Add one continuous free vertex memory block
    MemoryBlock initialVertexBlock;
    initialVertexBlock.begin = 0;
    initialVertexBlock.end = static_cast<uint32_t>(maxVertices);
    m_freeVertexMemoryBlocks.push_back(initialVertexBlock);

    // Add one continuous free index memory block
    MemoryBlock initialIndexBlock;
    initialIndexBlock.begin = 0;
    initialIndexBlock.end = static_cast<uint32_t>(maxIndices);
    m_freeIndexMemoryBlocks.push_back(initialIndexBlock);

    m_initialized = true;
}

size_t MeshBuffer::CalculateNewCapacity(size_t requiredCount, size_t currentCapacity) {
    if (currentCapacity == 0) {
        return std::max(requiredCount, m_minCapacity);
    }

    return std::max(requiredCount, static_cast<size_t>(currentCapacity * m_growthMultiplier));
}

Mesh* MeshBuffer::GetMeshById(uint64_t meshId) {
    auto it = m_meshes.find(meshId);
    if (it != m_meshes.end()) {
        return &it->second;
    }

    return nullptr;
}

std::span<Vertex> MeshBuffer::GetMeshVertexSpan(uint64_t meshId) {
    Mesh* mesh = GetMeshById(meshId);
    if (!mesh) return {};

    return std::span<Vertex>(m_vertices.data() + mesh->baseVertex, mesh->vertexCount);
}

std::span<uint32_t> MeshBuffer::GetMeshIndexSpan(uint64_t meshId) {
    Mesh* mesh = GetMeshById(meshId);
    if (!mesh) return {};

    return std::span<uint32_t>(m_indices.data() + mesh->baseIndex, mesh->indexCount);
}

void MeshBuffer::PrintDebugInfo() {
    size_t usedVertexCount = 0;
    size_t usedIndexCount = 0;

    for (const auto& [meshId, mesh] : m_meshes) {
        usedVertexCount += mesh.vertexCount;
        usedIndexCount += mesh.indexCount;
    }

    size_t freeVertexCount = 0;
    size_t largestFreeVertexBlock = 0;

    for (const MemoryBlock& block : m_freeVertexMemoryBlocks) {
        size_t blockSize = block.end - block.begin;
        freeVertexCount += blockSize;
        largestFreeVertexBlock = std::max(largestFreeVertexBlock, blockSize);
    }

    size_t freeIndexCount = 0;
    size_t largestFreeIndexBlock = 0;

    for (const MemoryBlock& block : m_freeIndexMemoryBlocks) {
        size_t blockSize = block.end - block.begin;
        freeIndexCount += blockSize;
        largestFreeIndexBlock = std::max(largestFreeIndexBlock, blockSize);
    }

    std::string message;
    message += "MeshBuffer Debug Info\n";
    message += "\n";

    message += "Meshes\n";
    message += "  Mesh count: " + std::to_string(GetMeshCount()) + "\n";
    message += "  Used vertex count: " + std::to_string(usedVertexCount) + "\n";
    message += "  Used index count: " + std::to_string(usedIndexCount) + "\n";
    message += "\n";

    message += "CPU storage\n";
    message += "  CPU allocated vertex count: " + std::to_string(GetAllocatedVertexCount()) + "\n";
    message += "  CPU allocated index count: " + std::to_string(GetAllocatedIndexCount()) + "\n";
    message += "  CPU free vertex count: " + std::to_string(freeVertexCount) + "\n";
    message += "  CPU free index count: " + std::to_string(freeIndexCount) + "\n";
    message += "\n";

    message += "GPU storage\n";
    message += "  GPU vertex capacity: " + std::to_string(m_vertexCapacity) + "\n";
    message += "  GPU index capacity: " + std::to_string(m_indexCapacity) + "\n";
    message += "\n";

    message += "Fragmentation\n";
    message += "  Free vertex block count: " + std::to_string(m_freeVertexMemoryBlocks.size()) + "\n";
    message += "  Free index block count:  " + std::to_string(m_freeIndexMemoryBlocks.size()) + "\n";
    message += "  Largest free vertex block: " + std::to_string(largestFreeVertexBlock) + "\n";
    message += "  Largest free index block:  " + std::to_string(largestFreeIndexBlock) + "\n";
    message += "\n";

    message += "Free vertex blocks\n";
    for (size_t i = 0; i < m_freeVertexMemoryBlocks.size(); i++) {
        const MemoryBlock& block = m_freeVertexMemoryBlocks[i];
        message += "  [" + std::to_string(i) + "] begin: " + std::to_string(block.begin) + ", end: " + std::to_string(block.end) + ", size: " + std::to_string(block.end - block.begin) + "\n";
    }
    message += "\n";

    message += "Free index blocks\n";
    for (size_t i = 0; i < m_freeIndexMemoryBlocks.size(); i++) {
        const MemoryBlock& block = m_freeIndexMemoryBlocks[i];
        message += "  [" + std::to_string(i) + "] begin: " + std::to_string(block.begin) + ", end: " + std::to_string(block.end) + ", size: " + std::to_string(block.end - block.begin) + "\n";
    }
    message += "\n";

    message += "Mesh list\n";
    for (const auto& [meshId, mesh] : m_meshes) {
        message += "  Mesh id: " + std::to_string(meshId) + "\n";
        message += "    Name: " + mesh.GetName() + "\n";
        message += "    Base vertex: " + std::to_string(mesh.baseVertex) + "\n";
        message += "    Vertex count: " + std::to_string(mesh.vertexCount) + "\n";
        message += "    Base index: " + std::to_string(mesh.baseIndex) + "\n";
        message += "    Index count: " + std::to_string(mesh.indexCount) + "\n";
        message += "    AABB min: " + Util::Vec3ToString(mesh.aabbMin) + "\n";
        message += "    AABB max: " + Util::Vec3ToString(mesh.aabbMax) + "\n";
        message += "    Extents: " + Util::Vec3ToString(mesh.extents) + "\n";
        message += "\n";
    }

    Logging::Debug() << message << "\n";
}
