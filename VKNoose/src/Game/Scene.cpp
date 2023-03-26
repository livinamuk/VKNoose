#include "Scene.h"
#include "AssetManager.h"
#include "../Audio/Audio.h"
#include "House/wall.h"

namespace Scene {
	std::vector<GameObject> _gameObjects;
	std::vector<Wall> _walls;
}

AudioHandle _ropeAudioHandle;



void Scene::Init()
{
	GameObject& chair = _gameObjects.emplace_back(GameObject());
	chair.SetModel("chair");
	chair._worldTransform.position = glm::vec3(0.8f, 0, 1.1f);
	chair._worldTransform.rotation.y = 3.7f;
	chair.SetMeshMaterial(0, "FallenChair");

	GameObject& floor = _gameObjects.emplace_back(GameObject());
	floor.SetModel("floor");
	floor.SetMeshMaterial(0, "FloorBoards");

	GameObject& skull = _gameObjects.emplace_back(GameObject());
	skull.SetModel("skull");
	skull._worldTransform.rotation.y = 3.7f;
	skull._worldTransform.position = glm::vec3(-4, 0, -4);
	skull.SetMeshMaterial(0, "BlackSkull");
	skull.SetMeshMaterial(1, "BlackSkull");
	skull._worldTransform.scale = glm::vec3(2);

	GameObject& drawers = _gameObjects.emplace_back(GameObject());
	drawers.SetModel("chest_of_drawers");
	drawers._worldTransform.position = glm::vec3(1.6, 0, 0);
	drawers._worldTransform.rotation.y = -NOOSE_PI / 2;
	drawers.SetMeshMaterial(0, "Drawers");

	GameObject& skull2 = _gameObjects.emplace_back(GameObject());
	skull2.SetModel("skull");
	skull2._worldTransform.rotation.y = drawers._worldTransform.rotation.y + 1.6f;
	skull2._worldTransform.position = drawers._worldTransform.position + glm::vec3(-0.35f, 1.15f, -1.675f + 2.05f);
	skull2._worldTransform.position.z -= 0.8f;
	skull2.SetMeshMaterial(0, "BlackSkull");
	skull2.SetMeshMaterial(1, "BlackSkull");
	skull2._worldTransform.scale = glm::vec3(0.1f);

	GameObject& ceiling = _gameObjects.emplace_back(GameObject());
	ceiling.SetModel("ceiling");
	ceiling.SetMeshMaterial(0, "Ceiling");
	//ceiling.SetMeshMaterial(0, "NumGrid");

	GameObject& wife = _gameObjects.emplace_back(GameObject());
	wife.SetModel("wife");
	//wife._worldTransform.position.x = 0.8f - 0.6f;
	wife._worldTransform.position.x = 0.6f;
	//wife._worldTransform.position.y = 2.5f;
	wife._worldTransform.position.y = 2.6f;
	wife._worldTransform.position.z = 0.9f;// -0.15;
	wife._worldTransform.rotation.y = -1.75;
	wife._worldTransform.scale = glm::vec3(0.01f);
	wife.SetMeshMaterial(0, "WifeHair");
	wife.SetMeshMaterial(1, "WifeSkin");
	wife.SetMeshMaterial(2, "WifeSkin");
	wife.SetMeshMaterial(3, "WifeBrows");
	wife.SetMeshMaterial(4, "WifeDress");
	wife.SetMeshMaterial(5, "WifeEye");
	wife.SetMeshMaterial(6, "WifeTeeth");
	wife.SetMeshMaterial(7, "Noose");
	wife.SetMeshMaterial(8, "Noose");

	float DOOR_WIDTH = 0.8f;

	float roomZmin = -1.8f;
	float roomZmax = 1.8f;
	float roomXmin = -2.75f;
	float roomXmax = 1.6f;
	float bathroomDoorX = -1.6f;
	float doorWidth = 0.9;
	float hallDoorX = 0.5f;

	GameObject& door0 = _gameObjects.emplace_back(GameObject());
	door0.SetModel("door");
	door0.SetMeshMaterial(0, "Door");
	door0._worldTransform.rotation.y = NOOSE_HALF_PI;
	door0._worldTransform.position.x = hallDoorX + 0.39550f;
	door0._worldTransform.position.z = -2.05 + 0.2f + 0.058520 - 0.1;

	GameObject& doorFrame0 = _gameObjects.emplace_back(GameObject());
	doorFrame0.SetModel("door_frame");
	doorFrame0.SetMeshMaterial(0, "Door");
	doorFrame0._worldTransform.rotation.y = NOOSE_HALF_PI;
	doorFrame0._worldTransform.position.x = hallDoorX;
	doorFrame0._worldTransform.position.z = -2.05 + 0.2f;

	GameObject& lightSwitch = _gameObjects.emplace_back(GameObject());
	lightSwitch.SetModel("light_switch");
	lightSwitch.SetMeshMaterial(0, "LightSwitch");
	lightSwitch._worldTransform.rotation.y = -NOOSE_HALF_PI;
	lightSwitch._worldTransform.scale = glm::vec3(1.05f);
	lightSwitch._worldTransform.position = glm::vec3(-0.12f, 1.1f, -1.8f);

	GameObject& vase = _gameObjects.emplace_back(GameObject());
	vase.SetModel("vase");
	vase._worldTransform.rotation.y = -0.6f - NOOSE_HALF_PI;
	vase._worldTransform.position = glm::vec3(1.6f -0.4f, 1.49f, 0.395f);
	vase.SetMeshMaterial(0, "Vase");

	GameObject& flowers = _gameObjects.emplace_back(GameObject());
	flowers.SetModel("flowers");
	flowers._worldTransform.rotation.y = -0.6f - NOOSE_HALF_PI;
	flowers._worldTransform.position = glm::vec3(1.6f - 0.4f, 1.49f, 0.395f);
	flowers.SetMeshMaterial(0, "Flowers");

	/*
	float d = 1.25f;


	GameObject& door3 = _gameObjects.emplace_back(GameObject());
	door3.SetModel("door");
	door3.SetMeshMaterial(0, "Door");
	door3._worldTransform.position = glm::vec3(-0.5, 0, d);
	door3._worldTransform.rotation = glm::vec3(0, 3.14 * 1.5, 0);*/
	
	/*
	GameObject& door2 = _gameObjects.emplace_back(GameObject());
	door2.SetModel("door");
	door2.SetMeshMaterial(0, "Door");
	door2._worldTransform.position = glm::vec3(0.5, 0, -d);
	door2._worldTransform.rotation = glm::vec3(0, 3.14 * 0.5, 0);
	*/

	/*GameObject& door1 = _gameObjects.emplace_back(GameObject());
	door1.SetModel("door");
	door1.SetMeshMaterial(0, "Door");
	door1._worldTransform.position = glm::vec3(d, 0, -0.5);
	door1._worldTransform.rotation = glm::vec3(0, -3.14, 0);
	*/

	/*GameObject& door0 = _gameObjects.emplace_back(GameObject());
	door0.SetModel("door");
	door0.SetMeshMaterial(0, "Door");
	door0._worldTransform.position = glm::vec3(-d, 0, -0.5);
	door0._worldTransform.rotation = glm::vec3(0, 3.14, 0);

	
	*/



	/*        
					  C  
	 	X		 ___________
		|		|	    Wife|	
		|		|			|
		|		|			|
		|	  B |			| A
		|		|			|
		|		|			|
		|		|___________|
	    |    
		|			  D 
        |
	    (0,0)----------------------- Z

	*/

	Wall& wallA = _walls.emplace_back(glm::vec3(roomXmax, 0, roomZmax), glm::vec3(roomXmin, 0, roomZmax));
	Wall& wallB = _walls.emplace_back(glm::vec3(roomXmin, 0, roomZmin), glm::vec3(hallDoorX - doorWidth / 2, 0, -roomZmax));
	Wall& wallB2 = _walls.emplace_back(glm::vec3(hallDoorX + doorWidth / 2, 0, roomZmin), glm::vec3(roomXmax, 0, roomZmin));
	Wall& wallBC = _walls.emplace_back(glm::vec3(hallDoorX - doorWidth / 2, 2, -roomZmax), glm::vec3(hallDoorX + doorWidth / 2, 2, roomZmin));
	Wall& wallC = _walls.emplace_back(glm::vec3(roomXmax, 0, roomZmin), glm::vec3(roomXmax, 0, roomZmax));
	Wall& wallD = _walls.emplace_back(glm::vec3(roomXmin, 0, roomZmax), glm::vec3(roomXmin, 0, roomZmin));

	for (auto& wall : _walls) {
		GameObject& trim = _gameObjects.emplace_back(GameObject());
		trim._worldTransform.position = wall._begin * glm::vec3(1, 0, 1);
		trim._worldTransform.position.y += CEILING_HEIGHT - 2.4f;
		trim._worldTransform.rotation.y = Util::YRotationBetweenTwoPoints(wall._begin, wall._end);
		trim._worldTransform.scale.x = glm::distance(wall._end, wall._begin);
		trim.SetModel("trims_ceiling");
		trim.SetMeshMaterial(0, "Trims");
	}
}

void Scene::Update() {

	static bool firstRun = true;

	if (firstRun) {
		_ropeAudioHandle = Audio::LoopAudio("Rope.wav", 0.5);
		firstRun = false;
	}

	// Sway wife
	static float sway = 0;
	sway += 0.0625f;
	float offset = sin(sway) * 0.03;
	auto& wife = _gameObjects[6];
	wife._worldTransform.rotation.y = -1.75 + offset;
	wife._worldTransform.rotation.x = 0.075f;
	wife._worldTransform.rotation.x += -offset * 0.045f;
	wife._worldTransform.rotation.z = -offset * 0.045f;

	// Distance to wife audio
	/*glm::vec3 playerPos = GameData::player.m_position;
	glm::vec3 wifePos = GameData::entities[0].m_transform.position * glm::vec3(1, 0, 1);
	float distance = glm::distance(playerPos, wifePos);
	float falloff = 1.0;
	float ropeVolume = 1.0f;
	if (distance > falloff) {
		distance -= falloff;
		distance *= 0.25;
		ropeVolume = (1 - distance);
	}
	_ropeAudioHandle.channel->setVolume(ropeVolume * 0.5f);*/
}

std::vector<GameObject>& Scene::GetGameObjects() {
	return _gameObjects;
}

std::vector<MeshRenderInfo> Scene::GetMeshRenderInfos() {

	std::vector<MeshRenderInfo> infos;

	int parentIndex = 0;
	for (GameObject& gameObject : _gameObjects) {
		for (int i=0; i < gameObject._model->_meshIndices.size(); i++) {
			int meshIndex = gameObject._model->_meshIndices[i];
			Mesh* mesh = AssetManager::GetMesh(meshIndex);

			MeshRenderInfo info;
			info._vkTransform = gameObject.GetVkTransformMatrixKHR();
			info._deviceAddress = mesh->_accelerationStructure.deviceAddress;
			info._modelMatrix = gameObject._worldTransform.to_mat4();
			info._basecolor = gameObject._meshMaterials[i]->_basecolor;
			info._normal = gameObject._meshMaterials[i]->_normal;
			info._rma = gameObject._meshMaterials[i]->_rma;
			info._vertexOffset = mesh->_vertexOffset;
			info._indexOffset = mesh->_indexOffset;
			info._parentIndex = parentIndex;
			info._parentType = "GameObject";
			infos.push_back(info);
			parentIndex++;
		}
	}
	parentIndex = 0;
	for (Wall& wall: _walls) {		
		Mesh* mesh = AssetManager::GetMesh(wall._meshIndex);
		Material* material = AssetManager::GetMaterial("WallPaper");
		MeshRenderInfo info;
		info._vkTransform = Util::GetIdentiyVkTransformMatrixKHR();
		info._deviceAddress = mesh->_accelerationStructure.deviceAddress;
		info._modelMatrix = glm::mat4(1);
		info._basecolor = material->_basecolor;
		info._normal = material->_normal;
		info._rma = material->_rma;
		info._vertexOffset = mesh->_vertexOffset;
		info._indexOffset = mesh->_indexOffset;
		info._parentIndex = parentIndex;
		info._parentType = "Wall";
		infos.push_back(info);
		parentIndex++;
	}
	return infos;
}

void Scene::StoreMousePickResult(int instanceIndex, int primitiveIndex)
{
	_instanceIndex = instanceIndex;
	_primitiveIndex = primitiveIndex;
	_worldPosOfHitObject = glm::vec3(0, 0, 0);
	_hitModelName = "undefined";

	std::vector<MeshRenderInfo> infos = GetMeshRenderInfos();

	if (instanceIndex >= 0 && instanceIndex < infos.size()) {
		MeshRenderInfo info = infos[instanceIndex];

		void* parent = info._parent;

		if (info._parentType == "GameObject") {
		
			if (info._parentIndex >= 0 && info._parentIndex < _gameObjects.size()) {
				GameObject& gameObject = _gameObjects[info._parentIndex];
				_worldPosOfHitObject = gameObject._worldTransform.position;

				if (gameObject._model)
					_hitModelName = gameObject._model->_filename;
			}
		}
	}
}

/*
std::vector<Mesh*> Scene::GetMeshList() {
	std::vector<Mesh*> meshes;
	for (GameObject& gameObject : _gameObjects) {
		for (int& index : gameObject._model->_meshIndices) {
			meshes.push_back(AssetManager::GetMesh(index));
		}
	}
	return meshes;
}

int Scene::GetMeshCount() {
	return GetMeshList().size() + _walls.size();
}*/