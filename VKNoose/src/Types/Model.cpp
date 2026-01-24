#include "Model.h"
#include "AssetManagement/AssetManager.h"
//#include "HellLogging.h"
//#include <stack>

void Model::SetFileInfo(FileInfo fileInfo) {
    m_fileInfo = fileInfo;
}

void Model::AddMeshIndex(uint32_t index) {
    m_meshIndices.push_back(index);

    // Map global mesh index to mesh name
    //if (Mesh* mesh = AssetManager::GetMeshByIndex(index)) {
    //    m_meshNameToGlobalMeshIndexMap[mesh->GetName()] = index;
    //}
}

void Model::SetName(std::string modelName) {
    m_name = modelName;
}

void Model::SetAABB(glm::vec3 aabbMin, glm::vec3 aabbMax) {
    m_aabbMin = aabbMin;
    m_aabbMax = aabbMax;
}

void Model::SetLoadingState(LoadingState loadingState) {
    m_loadingState = loadingState;
}

LoadingState Model::GetLoadingState() const {
    return m_loadingState.GetLoadingState();
}

//int32_t Model::GetGlobalMeshIndexByMeshName(const std::string& meshName) {
//    auto it = m_meshNameToGlobalMeshIndexMap.find(meshName);
//    if (it == m_meshNameToGlobalMeshIndexMap.end()) return -1;
//    return (int32_t)it->second;
//}

//const glm::mat4& Model::GetBoneLocalMatrix(const std::string& boneName) const {
//    static const glm::mat4 identity(1.0f);
//
//    for (const ArmatureData& armatureData : m_modelData.armatures) {
//        for (const Bone& bone : armatureData.bones) {
//            if (bone.name == boneName) {
//                return bone.localRestPose;
//            }
//        }
//    }
//
//    Logging::Warning() << "Model::GetBoneLocalMatrix(..) failed: " << boneName << " not found in " << m_name;
//
//    // Print bones
//    for (const ArmatureData& armatureData : m_modelData.armatures) {
//        for (const Bone& bone : armatureData.bones) {
//            std::cout << bone.name << "\n";
//        }
//    }
//
//    return identity;
//}