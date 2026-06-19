#include "AssetManager.h"
#include "Hell/Logging.h"
#include "ResourceManagement/ResourceManager.h"
#include "Util/Util.h"
#include <future>

//#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace AssetManager {
    static std::vector<std::future<void>> g_modelFutures;
    ModelData ImportModel(const std::string& path); // In File.h in HellEngine

    void LoadPendingModelsAsync() {
        for (Model& model : GetModels()) {
            if (model.GetLoadingState() == LoadingState::Value::AWAITING_LOADING_FROM_DISK) {
                model.SetLoadingState(LoadingState::Value::LOADING_FROM_DISK);
                AddItemToLoadLog("     " + model.GetFileInfo().path);
				LoadModel(&model); // non async for now
                //g_modelFutures.emplace_back(std::async(std::launch::async, LoadModel, &model));
                break;
            }
        }

        // Pump any completed futures
        for (size_t i = 0; i < g_modelFutures.size();) {
            if (g_modelFutures[i].wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                try {
                    g_modelFutures[i].get();
                }
                catch (...) {
                    std::cout << "Some async model loading error occured\n";
                }
                g_modelFutures.erase(g_modelFutures.begin() + i);
            }
            else {
                ++i;
            }
        }
    }

    void LoadModel(Model* model) {
        const FileInfo& fileInfo = model->GetFileInfo();
        std::string modelPath = "res/models/" + fileInfo.name + "." + fileInfo.ext;
        model->m_modelData = ImportModel(modelPath);
        model->SetLoadingState(LoadingState::Value::LOADING_COMPLETE);
    }

    ModelData ImportModel(const std::string& path) {
		ModelData modelData;

		// AABB
        modelData.aabbMin = glm::vec3(99999.9f);
        modelData.aabbMax = glm::vec3(-99999.9f);

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn;
		std::string err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
			std::cout << "Crashed loading model: " << path << "\n";
			return modelData;
		}

		std::unordered_map<Vertex, uint32_t> uniqueVertices = {};


		for (const auto& shape : shapes) {
			std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            glm::vec3 aabbMin = glm::vec3(99999.9f);
            glm::vec3 aabbMax = glm::vec3(-99999.9f);

			for (int i = 0; i < shape.mesh.indices.size(); i++) {

				Vertex vertex = {};
				const auto& index = shape.mesh.indices[i];
				vertex.position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				// Check if `normal_index` is zero or positive. negative = no normal data
				if (index.normal_index >= 0) {
					vertex.normal.x = attrib.normals[3 * size_t(index.normal_index) + 0];
					vertex.normal.y = attrib.normals[3 * size_t(index.normal_index) + 1];
					vertex.normal.z = attrib.normals[3 * size_t(index.normal_index) + 2];
				}

				if (attrib.texcoords.size() && index.texcoord_index != -1) { // should only be 1 or 2, some bug in debug where there were over 1000 on the spherelines model...
					vertex.uv = { attrib.texcoords[2 * index.texcoord_index + 0],	1.0f - attrib.texcoords[2 * index.texcoord_index + 1] };
				}

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}

                aabbMin = glm::min(aabbMin, vertex.position);
                aabbMax = glm::max(aabbMax, vertex.position);

				indices.push_back(uniqueVertices[vertex]);
			}

			Util::SetTangentsFromVertices(vertices, indices);

			// Hack to not render her brows
			if (shape.name == "Camila_Brow") {
				indices = { 0,0,0 };
			}

			MeshData& meshData = modelData.meshes.emplace_back();
            meshData.name = shape.name;
			meshData.vertices = vertices;
			meshData.indices = indices;
			meshData.vertexCount = vertices.size();
            meshData.indexCount = indices.size();
            meshData.aabbMin = aabbMin;
            meshData.aabbMax = aabbMax;

            modelData.aabbMin = glm::min(modelData.aabbMin, meshData.aabbMin);
			modelData.aabbMax = glm::max(modelData.aabbMax, meshData.aabbMax);
		}

        modelData.meshCount = modelData.meshes.size();
        modelData.name = Util::GetFileInfo(path).filename;

		return modelData;
    }

	Model& CreateModel(const std::string& name) {
		Model& model = GetModels().emplace_back();
		model.SetName(name);
		return model;
	}

	Model* GetModelByName(const std::string& name) {
		for (Model& model : GetModels()) {
			if (model.GetName() == name) {
				return &model;
			}
		}

		Logging::Error() << "AssetManager::GetModelByName(..) failed '" << name << "' not found\n";
		return nullptr;
	}

	void BakeModels() {
		// Preallocate the vertex/index count
		size_t vertexCount = 0;
		size_t indexCount = 0;

		for (const Model& model : GetModels()) {
			for (const MeshData& mesh : model.m_modelData.meshes) {
				vertexCount += mesh.vertexCount;
				indexCount += mesh.indexCount;
			}
		}

		MeshBuffer& staticGeometry = ResourceManager::GetMeshBuffer(ResourceManager::STATIC_GEOMETRY_MESH_BUFFER_NAME);
		staticGeometry.PreAllocate(vertexCount + 1024, indexCount + 4096);

		// Copy the vertices/indices into the asset manager
		for (Model& model : GetModels()) {
			model.SetAABB(model.m_modelData.aabbMin, model.m_modelData.aabbMax);

			for (MeshData& meshData : model.m_modelData.meshes) {
				uint64_t meshId = CreateMesh(meshData.name, meshData.vertices, meshData.indices, meshData.aabbMin, meshData.aabbMax, meshData.parentIndex, meshData.localTransform, meshData.inverseBindTransform);
				model.AddMeshId(meshId);
			}
		}

		std::cout << "\n";
		Logging::Init() << "AssetManager::BakeModels()\n";
	}

}
