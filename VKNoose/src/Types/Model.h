#pragma once
#include "Common.h"
//#include "HellEnums.h"
//#include "HellTypes.h"
//#include "LoadingState.h"
//#include "Mesh.h"
#include "Noose/Types.h"

#include <limits>
#include <string>
#include <vector>
#include "LoadingState.h"

struct Bone {
    char name[64];
    glm::mat4 localRestPose = glm::mat4(1.0f);
    glm::mat4 inverseBindPose = glm::mat4(1.0f);
    int32_t parentIndex = -1;
    int32_t deformFlag = 0; // 0: non-deforming 1: deforming
};

struct ArmatureData {
    std::string name = UNDEFINED_STRING;
    uint32_t boneCount = 0;
    std::vector<Bone> bones;
};

struct MeshData {
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    glm::vec3 aabbMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 aabbMax = glm::vec3(-std::numeric_limits<float>::max());
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    int32_t parentIndex = -1;
    glm::mat4 localTransform = glm::mat4(1.0f);
    glm::mat4 inverseBindTransform = glm::mat4(1.0f);
};

struct ModelData {
    std::string name = UNDEFINED_STRING;
    uint32_t meshCount = 0;
    uint32_t armatureCount = 0;
    uint64_t timestamp = 0;
    std::vector<MeshData> meshes;
    std::vector<ArmatureData> armatures;
    glm::vec3 aabbMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 aabbMax = glm::vec3(-std::numeric_limits<float>::max());
};

struct Model {
    Model() = default;

    void SetFileInfo(FileInfo fileInfo);
    void AddMeshIndex(uint32_t index);
    void SetName(std::string modelName);
    void SetAABB(glm::vec3 aabbMin, glm::vec3 aabbMax);
    void SetLoadingState(LoadingState loadingState);
    //int32_t GetGlobalMeshIndexByMeshName(const std::string& meshName);
    //const glm::mat4& GetBoneLocalMatrix(const std::string& boneName) const;
    //
    LoadingState GetLoadingState() const;
    const FileInfo& GetFileInfo() const                 { return m_fileInfo; }
    const size_t GetMeshCount()  const                  { return m_meshIndices.size(); }
    const glm::vec3& GetAABBMin() const                 { return m_aabbMin; }
    const glm::vec3& GetAABBMax() const                 { return m_aabbMax; }
    const glm::vec3& GetExtents() const                 { return m_aabbMax - m_aabbMin; }
    const std::string GetName() const                   { return m_name; }
    const std::vector<uint32_t>& GetMeshIndices() const { return m_meshIndices; }
    //
    //bool m_awaitingLoadingFromDisk = true;
    ModelData m_modelData;


private:
    FileInfo m_fileInfo;
    LoadingState m_loadingState{ LoadingState::Value::AWAITING_LOADING_FROM_DISK };
    glm::vec3 m_aabbMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 m_aabbMax = glm::vec3(-std::numeric_limits<float>::max());
    std::string m_name = "undefined";
    std::vector<uint32_t> m_meshIndices;
    //std::unordered_map<std::string, uint32_t> m_meshNameToGlobalMeshIndexMap;
};
