#include "AssetManager.h"
#include "Game/Scene.h"
#include "../Util.h"

namespace AssetManager {

    void CreateBlitterQuad();
    void CreateFullscreenQuad();
    void CreateHouse();

    void LoadHardcodedMesh() {
        CreateBlitterQuad();
        CreateFullscreenQuad();
        CreateHouse();

        std::cout << "AssetManager::LoadHardcodedMesh()\n";
    }

    void CreateBlitterQuad() {
        Vertex vertA, vertB, vertC, vertD;
        vertA.position = { -1.0f, -1.0f, 0.0f };
        vertB.position = { -1.0f, 1.0f, 0.0f };
        vertC.position = { 1.0f,  1.0f, 0.0f };
        vertD.position = { 1.0f,  -1.0f, 0.0f };
        vertA.uv = { 0.0f, 1.0f };
        vertB.uv = { 0.0f, 0.0f };
        vertC.uv = { 1.0f, 0.0f };
        vertD.uv = { 1.0f, 1.0f };

        std::vector<Vertex> vertices;
        vertices.push_back(vertA);
        vertices.push_back(vertB);
        vertices.push_back(vertC);
        vertices.push_back(vertD);

        std::vector<uint32_t> indices = { 0, 1, 2, 0, 2, 3 };

        ModelOLD model;
        model.m_meshIndices.push_back(CreateMeshOLD(vertices, indices));
        std::unordered_map<std::string, ModelOLD>& _models = GetModelsOLD();
        _models["blitter_quad"] = model;

        Model& model2 = AssetManager::CreateModel("blitter_quad");
        model2.AddMeshIndex(AssetManager::CreateMesh("blitter_quad_mesh", vertices, indices));
        model2.SetLoadingState(LoadingState::Value::LOADING_COMPLETE);
    }

    void CreateFullscreenQuad() {
        Vertex vertA, vertB, vertC, vertD;
        vertA.position = { -1.0f, -1.0f, 0.0f };
        vertB.position = { -1.0f, 1.0f, 0.0f };
        vertC.position = { 1.0f,  1.0f, 0.0f };
        vertD.position = { 1.0f,  -1.0f, 0.0f };
        vertA.uv = { 0.0f, 1.0f };
        vertB.uv = { 0.0f, 0.0f };
        vertC.uv = { 1.0f, 0.0f };
        vertD.uv = { 1.0f, 1.0f };

        std::vector<Vertex> vertices;
        vertices.push_back(vertA);
        vertices.push_back(vertB);
        vertices.push_back(vertC);
        vertices.push_back(vertD);

        std::vector<uint32_t> indices = { 0, 1, 2, 0, 2, 3 };

        ModelOLD model;
        model.m_meshIndices.push_back(CreateMeshOLD(vertices, indices));
        std::unordered_map<std::string, ModelOLD>& _models = GetModelsOLD();
        _models["fullscreen_quad"] = model;

        Model& model2 = AssetManager::CreateModel("fullscreen_quad");
        model2.AddMeshIndex(AssetManager::CreateMesh("fullscreen_quad_mesh", vertices, indices));
        model2.SetLoadingState(LoadingState::Value::LOADING_COMPLETE);
    }

    void CreateHouse() {
        std::unordered_map<std::string, ModelOLD>& _models = GetModelsOLD();

        {
            // Floor 
            Vertex vert0, vert1, vert2, vert3;
            float yPos = -0.005f;
            float size = 10;
            float texSize = 10;
            vert0.position = { -size, yPos, -size };
            vert1.position = { -size, yPos, size };
            vert2.position = { size, yPos, size };
            vert3.position = { size, yPos, -size };
            vert0.uv = { 0, texSize };
            vert1.uv = { 0, 0 };
            vert2.uv = { texSize, texSize };
            vert3.uv = { texSize, 0 };
            vert0.normal = { 0, 1, 0 };
            vert1.normal = { 0, 1, 0 };
            vert2.normal = { 0, 1, 0 };
            vert3.normal = { 0, 1, 0 };
            std::vector<Vertex> vertices;
            vertices.push_back(vert0);
            vertices.push_back(vert1);
            vertices.push_back(vert2);
            vertices.push_back(vert3);
            //std::vector<uint32_t> indices = { 2, 1, 0, 3, 2, 0 };
            std::vector<uint32_t> indices = { 0, 1, 2, 0, 2, 3 };
            Util::SetTangentsFromVertices(vertices, indices);
            ModelOLD model;
            model.m_meshIndices.push_back(CreateMeshOLD(vertices, indices));
            model.m_filename = "Floor";
            _models["floor"] = model;

            Model& model2 = AssetManager::CreateModel("floor");
            model2.AddMeshIndex(AssetManager::CreateMesh("floor_mesh", vertices, indices));
            model2.SetLoadingState(LoadingState::Value::LOADING_COMPLETE);
        }

        {
            // bathroom floor 
            float xMin = -2.715f;
            float xMax = -0.76f;
            float zMin = 1.9f;
            float zMax = 3.8f;

            Vertex vert0, vert1, vert2, vert3;
            float yPos = 0.0f;
            float size = 10;
            float texScale = 1.5f;
            vert0.position = { xMin, yPos, zMin };
            vert1.position = { xMin, yPos, zMax };
            vert2.position = { xMax, yPos, zMax };
            vert3.position = { xMax, yPos, zMin };
            vert0.uv = { xMin / texScale, zMax / texScale };
            vert1.uv = { xMin / texScale, zMin / texScale };
            vert2.uv = { xMax / texScale, zMin / texScale };
            vert3.uv = { xMax / texScale, zMax / texScale };
            vert0.normal = { 0, 1, 0 };
            vert1.normal = { 0, 1, 0 };
            vert2.normal = { 0, 1, 0 };
            vert3.normal = { 0, 1, 0 };
            std::vector<Vertex> vertices;
            vertices.push_back(vert0);
            vertices.push_back(vert1);
            vertices.push_back(vert2);
            vertices.push_back(vert3);
            //std::vector<uint32_t> indices = { 2, 1, 0, 3, 2, 0 };
            std::vector<uint32_t> indices = { 0, 1, 2, 0, 2, 3 };
            Util::SetTangentsFromVertices(vertices, indices);
            ModelOLD model;
            model.m_meshIndices.push_back(CreateMeshOLD(vertices, indices));
            model.m_filename = "bathroom_floor";
            _models["bathroom_floor"] = model;

            Model& model2 = AssetManager::CreateModel("bathroom_floor");
            model2.AddMeshIndex(AssetManager::CreateMesh("bathroom_floor_mesh", vertices, indices));
            model2.SetLoadingState(LoadingState::Value::LOADING_COMPLETE);
        }

        {
            // bathroom ceiling 
            float xMin = -2.715f;
            float xMax = -0.76f;
            float zMin = 1.9f;
            float zMax = 3.8f;

            Vertex vert0, vert1, vert2, vert3;
            float yPos = CEILING_HEIGHT;
            float size = 10;
            float texScale = 1.5f;
            vert0.position = { xMin, yPos, zMin };
            vert1.position = { xMin, yPos, zMax };
            vert2.position = { xMax, yPos, zMax };
            vert3.position = { xMax, yPos, zMin };
            vert0.uv = { xMin / texScale, zMax / texScale };
            vert1.uv = { xMin / texScale, zMin / texScale };
            vert2.uv = { xMax / texScale, zMin / texScale };
            vert3.uv = { xMax / texScale, zMax / texScale };
            vert0.normal = { 0, -1, 0 };
            vert1.normal = { 0, -1, 0 };
            vert2.normal = { 0, -1, 0 };
            vert3.normal = { 0, -1, 0 };
            std::vector<Vertex> vertices;
            vertices.push_back(vert0);
            vertices.push_back(vert1);
            vertices.push_back(vert2);
            vertices.push_back(vert3);
            //std::vector<uint32_t> indices = { 0, 1, 2, 0, 2, 3 };
            std::vector<uint32_t> indices = { 2, 1, 0, 3, 2, 0 };
            Util::SetTangentsFromVertices(vertices, indices);
            ModelOLD model;
            model.m_meshIndices.push_back(CreateMeshOLD(vertices, indices));
            model.m_filename = "bathroom_ceiling";
            _models["bathroom_ceiling"] = model;

            Model& model2 = AssetManager::CreateModel("bathroom_ceiling");
            model2.AddMeshIndex(AssetManager::CreateMesh("bathroom_ceiling_mesh", vertices, indices));
            model2.SetLoadingState(LoadingState::Value::LOADING_COMPLETE);
        }


        {
            float Zmin = -1.8f;
            float Zmax = 1.8f;
            float Xmin = -2.75f;
            float Xmax = 1.6f;
            // bedroom ceiling 
            Vertex vert0, vert1, vert2, vert3;
            float size = 10;
            float texScale = 3;
            vert0.position = { Xmin, CEILING_HEIGHT, Zmin };
            vert1.position = { Xmin, CEILING_HEIGHT, Zmax };
            vert2.position = { Xmax, CEILING_HEIGHT, Zmax };
            vert3.position = { Xmax, CEILING_HEIGHT, Zmin };
            vert0.uv = { 0, fmod(2.0, 1.0) };
            vert1.uv = { 0, 0 };
            vert2.uv = { fmod(2.0, 1.0), fmod(2.0, 1.0) };
            vert3.uv = { fmod(2.0, 1.0), 0 };
            vert0.uv = { Xmin / texScale, Zmax / texScale };
            vert1.uv = { Xmin / texScale, Zmin / texScale };
            vert2.uv = { Xmax / texScale, Zmin / texScale };
            vert3.uv = { Xmax / texScale, Zmax / texScale };
            vert0.normal = { 0, 1, 0 };
            vert1.normal = { 0, 1, 0 };
            vert2.normal = { 0, 1, 0 };
            vert3.normal = { 0, 1, 0 };
            std::vector<Vertex> vertices;
            vertices.push_back(vert0);
            vertices.push_back(vert1);
            vertices.push_back(vert2);
            vertices.push_back(vert3);
            std::vector<uint32_t> indices = { 2, 1, 0, 3, 2, 0 };
            Util::SetTangentsFromVertices(vertices, indices);
            ModelOLD model;
            model.m_meshIndices.push_back(CreateMeshOLD(vertices, indices));
            _models["ceiling"] = model;

            Model& model2 = AssetManager::CreateModel("ceiling");
            model2.AddMeshIndex(AssetManager::CreateMesh("ceiling_mesh", vertices, indices));
            model2.SetLoadingState(LoadingState::Value::LOADING_COMPLETE);
        }

        // Walls
        // Scene::CreateWalls();

    }

}