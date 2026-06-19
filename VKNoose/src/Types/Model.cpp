#include "Model.h"
#include "AssetManagement/AssetManager.h"

void Model::SetFileInfo(FileInfo fileInfo) {
    m_fileInfo = fileInfo;
}

void Model::AddMeshId(uint64_t meshId) {
    m_meshIds.push_back(meshId);
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
