#pragma once
#include "ResourceManagement/Types/GenericMesh.h"
#include "ResourceManagement/Types/MeshBuffer.h"

#include <cstdint>
#include <string>

namespace ResourceManager {
    void Init();
    void CleanUp();

    GenericMesh& CreateGenericMesh(const std::string& name);
    GenericMesh& GetGenericMesh(const std::string& name);
    GenericMesh* GetGenericMeshPtr(const std::string& name);

    MeshBuffer& CreateMeshBuffer(const std::string& name);
    MeshBuffer& GetMeshBuffer(const std::string& name);
    MeshBuffer* GetMeshBufferPtr(const std::string& name);
}
