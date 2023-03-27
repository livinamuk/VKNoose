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
	chair.SetPosition(glm::vec3(0.8f, 0, 1.1f));
	chair.SetRotationY(3.7f);
	chair.SetMeshMaterial("FallenChair");

	GameObject& floor = _gameObjects.emplace_back(GameObject());
	floor.SetModel("floor");
	floor.SetMeshMaterial("FloorBoards");

	GameObject& skull = _gameObjects.emplace_back(GameObject());
	skull.SetModel("skull");
	skull.SetRotationY(3.7f);
	skull.SetPosition(glm::vec3(-4, 0, -4));
	skull.SetScale(2);
	skull.SetMeshMaterial("BlackSkull");


	{ // Drawers	
		GameObject& drawers = _gameObjects.emplace_back(GameObject());
		drawers.SetModel("DrawerFrame");
		drawers.SetPosition(glm::vec3(1.6, 0, 0));
		drawers.SetRotationY(-NOOSE_PI / 2);
		drawers.SetMeshMaterial("Drawers");
		drawers.SetName("ChestOfDrawers");

		GameObject& topLeftDrawer = _gameObjects.emplace_back(GameObject());
		topLeftDrawer.SetModel("DrawerTopLeft");
		topLeftDrawer.SetMeshMaterial("Drawers");
		topLeftDrawer.SetScriptName("OpenableDrawer");
		topLeftDrawer.SetParentName("ChestOfDrawers");
		topLeftDrawer.SetName("TopLeftDrawer");
		topLeftDrawer.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);

		GameObject& topRightDrawer = _gameObjects.emplace_back(GameObject());
		topRightDrawer.SetModel("DrawerTopRight");
		topRightDrawer.SetMeshMaterial("Drawers");
		topRightDrawer.SetScriptName("OpenableDrawer");
		topRightDrawer.SetParentName("ChestOfDrawers");
		topRightDrawer.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);

		GameObject& drawSecond = _gameObjects.emplace_back(GameObject());
		drawSecond.SetModel("DrawerSecond");
		drawSecond.SetMeshMaterial("Drawers");
		drawSecond.SetScriptName("OpenableDrawer");
		drawSecond.SetParentName("ChestOfDrawers");
		drawSecond.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);

		GameObject& drawThird = _gameObjects.emplace_back(GameObject());
		drawThird.SetModel("DrawerThird");
		drawThird.SetMeshMaterial("Drawers");
		drawThird.SetScriptName("OpenableDrawer");
		drawThird.SetParentName("ChestOfDrawers");
		drawThird.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);

		GameObject& drawerFourth = _gameObjects.emplace_back(GameObject());
		drawerFourth.SetModel("DrawerFourth");
		drawerFourth.SetMeshMaterial("Drawers");
		drawerFourth.SetScriptName("OpenableDrawer");
		drawerFourth.SetParentName("ChestOfDrawers");
		drawerFourth.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);

		GameObject& diary = _gameObjects.emplace_back(GameObject());
		diary.SetParentName("TopLeftDrawer");
		diary.SetModel("Diary");
		diary.SetPosition(-0.3f, 0.95f, 0.4f);
		diary.SetRotationY(0.4f);
		diary.SetScale(0.75f);
		diary.SetMeshMaterial("Diary");
		//diary.m_pickUpName = "Wife's Diary";
		//diary.m_pickUpText = "Take your [r]WIFE'S DIARY[w]?";

	}

	GameObject& skull2 = _gameObjects.emplace_back(GameObject());
	skull2.SetModel("skull");
	skull2.SetParentName("ChestOfDrawers");
	skull2.SetRotationY(1.6f);
	skull2.SetPosition(glm::vec3(-0.4f, 1.15f, -1.675f + 2.0f));
	skull2.SetMeshMaterial("BlackSkull");
	skull2.SetScale(0.1f);
	skull2.SetParentName("ChestOfDrawers");

	GameObject& ceiling = _gameObjects.emplace_back(GameObject());
	ceiling.SetModel("ceiling");
	ceiling.SetMeshMaterial("Ceiling");

	GameObject& wife = _gameObjects.emplace_back(GameObject());
	wife.SetModel("wife");
	wife.SetName("Wife");
	wife.SetPosition(0.6f, 2.6f, 0.9f);
	wife.SetRotationY(-1.75f);
	wife.SetScale(0.01f);
	wife.SetMeshMaterial("WifeHair", 0);
	wife.SetMeshMaterial("WifeSkin", 1);
	wife.SetMeshMaterial("WifeSkin", 2);
	wife.SetMeshMaterial("WifeBrows", 3);
	wife.SetMeshMaterial("WifeDress", 4);
	wife.SetMeshMaterial("WifeEye", 5);
	wife.SetMeshMaterial("WifeTeeth", 6);
	wife.SetMeshMaterial("Noose", 7);
	wife.SetMeshMaterial("Noose", 8);

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
	door0.SetMeshMaterial("Door");
	door0.SetRotationY(NOOSE_HALF_PI);
	door0.SetPosition(hallDoorX + 0.39550f, 0, -2.05 + 0.2f + 0.058520 - 0.1);
	door0.SetScriptName("OpenableDoor");
	door0.SetOpenState(OpenState::CLOSED, 5.208f, NOOSE_HALF_PI, NOOSE_HALF_PI - 1.9f);

	GameObject& doorFrame0 = _gameObjects.emplace_back(GameObject());
	doorFrame0.SetModel("door_frame");
	doorFrame0.SetMeshMaterial("Door");
	doorFrame0.SetRotationY(NOOSE_HALF_PI);
	doorFrame0.SetPosition(hallDoorX, 0, -2.05 + 0.2f);

	GameObject& lightSwitch = _gameObjects.emplace_back(GameObject());
	lightSwitch.SetModel("light_switch");
	lightSwitch.SetMeshMaterial("LightSwitch");
	lightSwitch.SetRotationY(-NOOSE_HALF_PI);
	lightSwitch.SetScale(1.05f);
	lightSwitch.SetPosition(-0.12f, 1.1f, -1.8f);

	GameObject& vase = _gameObjects.emplace_back(GameObject());
	vase.SetModel("vase");
	vase.SetRotationY(-0.6f - NOOSE_HALF_PI);
	vase.SetPosition(1.6f -0.4f, 1.49f, 0.395f);
	vase.SetMeshMaterial("Vase");

	GameObject& flowers = _gameObjects.emplace_back(GameObject());
	flowers.SetModel("flowers");
	flowers.SetRotationY(-0.6f - NOOSE_HALF_PI);
	flowers.SetPosition(1.6f - 0.4f, 1.49f, 0.395f);
	flowers.SetMeshMaterial("Flowers");

	/*	X		   C  
	 	|	  ___________
		|	 |	     Wife|	
		|	 |			 |
		|	 |			 |
		|  B |			 | A
		|	 |			 |
		|	 |			 |
		|	 |___________|
	    |    
		|		   D 
	    (0,0)----------------- Z
	*/
	Wall& wallA = _walls.emplace_back(glm::vec3(roomXmax, 0, roomZmax), glm::vec3(roomXmin, 0, roomZmax));
	Wall& wallB = _walls.emplace_back(glm::vec3(roomXmin, 0, roomZmin), glm::vec3(hallDoorX - doorWidth / 2, 0, -roomZmax));
	Wall& wallB2 = _walls.emplace_back(glm::vec3(hallDoorX + doorWidth / 2, 0, roomZmin), glm::vec3(roomXmax, 0, roomZmin));
	Wall& wallBC = _walls.emplace_back(glm::vec3(hallDoorX - doorWidth / 2, 2, -roomZmax), glm::vec3(hallDoorX + doorWidth / 2, 2, roomZmin));
	Wall& wallC = _walls.emplace_back(glm::vec3(roomXmax, 0, roomZmin), glm::vec3(roomXmax, 0, roomZmax));
	Wall& wallD = _walls.emplace_back(glm::vec3(roomXmin, 0, roomZmax), glm::vec3(roomXmin, 0, roomZmin));

	for (auto& wall : _walls) {
		GameObject& trim = _gameObjects.emplace_back(GameObject());
		trim.SetPosition(wall._begin.x, CEILING_HEIGHT - 2.4f, wall._begin.z);
		trim.SetRotationY(Util::YRotationBetweenTwoPoints(wall._begin, wall._end));
		trim.SetScaleX(glm::distance(wall._end, wall._begin));
		trim.SetModel("trims_ceiling");
		trim.SetMeshMaterial("Trims");
	}
}

GameObject* Scene::GetGameObjectByName(std::string name) {
	if (name == "undefined")
		return nullptr;
	for (GameObject& gameObject : _gameObjects) {
		if (gameObject.GetName() == name) {
			return &gameObject;
		}
	}
	return nullptr;
}

void Scene::Update(float deltaTime) {

	// Player pressed E to interact?
	if (_hoveredGameObject && _hoveredGameObject->IsInteractable() && Input::KeyPressed(HELL_KEY_E)) {
		_hoveredGameObject->Interact();
	}

	static bool firstRun = true;

	if (firstRun) {
		_ropeAudioHandle = Audio::LoopAudio("Rope.wav", 0.5);
		firstRun = false;
	}

	// Sway wife
	static float sway = 0;
	sway += (3.89845f * deltaTime);
	float offset = sin(sway) * 0.03;
	GameObject* wife = GetGameObjectByName("Wife");
	if (wife) {
		wife->SetRotationX(0.075f - offset * 0.045f);
		wife->SetRotationY(-1.75 + offset);
		wife->SetRotationZ(-offset * 0.045f);
	}

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


	for (GameObject& gameObject : _gameObjects) {
		gameObject.Update(deltaTime);
	}
}

std::vector<GameObject>& Scene::GetGameObjects() {
	return _gameObjects;
}

std::vector<MeshRenderInfo> Scene::GetMeshRenderInfos() {

	std::vector<MeshRenderInfo> infos;

	for (GameObject& gameObject : _gameObjects) {
		for (int i=0; i < gameObject._model->_meshIndices.size(); i++) {
			int meshIndex = gameObject._model->_meshIndices[i];
			Mesh* mesh = AssetManager::GetMesh(meshIndex);

			MeshRenderInfo info;
			info._vkTransform = gameObject.GetVkTransformMatrixKHR();
			info._deviceAddress = mesh->_accelerationStructure.deviceAddress;
			info._modelMatrix = gameObject.GetModelMatrix();
			info._basecolor = gameObject.GetMaterial(i)->_basecolor;
			info._normal = gameObject.GetMaterial(i)->_normal;
			info._rma = gameObject.GetMaterial(i)->_rma;
			info._vertexOffset = mesh->_vertexOffset;
			info._indexOffset = mesh->_indexOffset;
			info._parent = &gameObject;
			info._mesh = mesh;
			infos.push_back(info);
		}
	}
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
		info._parent = &wall;
		info._mesh = mesh;
		infos.push_back(info);
	}
	return infos;
}

void Scene::StoreMousePickResult(int instanceIndex, int primitiveIndex)
{
	_instanceIndex = instanceIndex;
	_primitiveIndex = primitiveIndex;
	_hitModelName = "undefined";
	_hitTriangleVertices.clear();
	_hoveredGameObject = nullptr;

	std::vector<MeshRenderInfo> infos = GetMeshRenderInfos();

	if (instanceIndex < 0 || instanceIndex >= infos.size())
		return;

	MeshRenderInfo hitInfo = infos[instanceIndex];

	if (!hitInfo._parent)
		return;
	
	for (GameObject& gameObject : _gameObjects) {
		if (&gameObject == hitInfo._parent) {
			_hitModelName = gameObject._model->_filename;
			int indexOffset = hitInfo._mesh->_indexOffset;
			int vertexOffset = hitInfo._mesh->_vertexOffset;
			int index0 = AssetManager::GetIndex(3 * primitiveIndex + 0 + indexOffset);
			int index1 = AssetManager::GetIndex(3 * primitiveIndex + 1 + indexOffset);
			int index2 = AssetManager::GetIndex(3 * primitiveIndex + 2 + indexOffset);
			Vertex v0 = AssetManager::GetVertex(index0 + vertexOffset);
			Vertex v1 = AssetManager::GetVertex(index1 + vertexOffset);
			Vertex v2 = AssetManager::GetVertex(index2 + vertexOffset);
			v0.position = gameObject.GetModelMatrix() * glm::vec4(v0.position, 1.0);
			v1.position = gameObject.GetModelMatrix() * glm::vec4(v1.position, 1.0);
			v2.position = gameObject.GetModelMatrix() * glm::vec4(v2.position, 1.0);
			_hitTriangleVertices.push_back(v0);
			_hitTriangleVertices.push_back(v1);
			_hitTriangleVertices.push_back(v2);
			_hoveredGameObject = &gameObject;
		}
	}

	for (Wall& wall : _walls) {
		if (&wall == hitInfo._parent) {
			_hitModelName = "Wall";
			Mesh* mesh = AssetManager::GetMesh(wall._meshIndex);
			int indexOffset = hitInfo._mesh->_indexOffset;
			int vertexOffset = hitInfo._mesh->_vertexOffset;
			int index0 = AssetManager::GetIndex(3 * primitiveIndex + 0 + indexOffset);
			int index1 = AssetManager::GetIndex(3 * primitiveIndex + 1 + indexOffset);
			int index2 = AssetManager::GetIndex(3 * primitiveIndex + 2 + indexOffset);
			Vertex v0 = AssetManager::GetVertex(index0 + vertexOffset);
			Vertex v1 = AssetManager::GetVertex(index1 + vertexOffset);
			Vertex v2 = AssetManager::GetVertex(index2 + vertexOffset);
			_hitTriangleVertices.push_back(v0);
			_hitTriangleVertices.push_back(v1);
			_hitTriangleVertices.push_back(v2);
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