#include "GenericMesh.h"

//#include "API/OpenGL/GL_resource_manager.h"
#include "Backend/BackEnd.h"
#include "Hell/Logging.h"

GenericMesh::GenericMesh(const std::string& name) {
    m_name = name;
}

void GenericMesh::UpdateVertexData(const void* vertices, size_t vertexCount, const VertexLayoutDescription& layout) {
    m_vertexCount = vertexCount;

    if (BackEnd::GetAPI() == API::OPENGL) {
        //if (m_openGLId == 0) {
        //    m_openGLId = OpenGLResourceManager::CreateGenericMesh();
        //}
        //
        //OpenGLResourceManager::GetGenericMesh(m_openGLId).UpdateVertexData(vertices, vertexCount, layout);
    }
}

void GenericMesh::UpdateIndexData(const std::vector<uint32_t>& indices) {
    m_indexCount = indices.size();

    if (BackEnd::GetAPI() == API::OPENGL) {
        //if (m_openGLId == 0) {
        //    m_openGLId = OpenGLResourceManager::CreateGenericMesh();
        //}
        //
        //OpenGLResourceManager::GetGenericMesh(m_openGLId).UpdateIndexData(indices);
    }
}

void GenericMesh::CleanUp() {
    //if (m_openGLId != 0) {
    //    OpenGLResourceManager::RemoveGenericMesh(m_openGLId);
    //    m_openGLId = 0;
    //}

    m_vertexCount = 0;
    m_indexCount = 0;
}

uint32_t GenericMesh::GetVAO() const {
    //if (BackEnd::GetAPI() == API::OPENGL) {
    //    OpenGLGenericMesh& mesh = OpenGLResourceManager::GetGenericMesh(m_openGLId);
    //    return mesh.GetVAO();
    //}
    //else {
    //    Logging::Error() << "GenericMesh::GetVAO() was called but API is Vulkan\n";
    //    return 0;
    //}

    return 0;
}
