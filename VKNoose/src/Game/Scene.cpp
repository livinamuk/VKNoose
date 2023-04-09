#include "Scene.h"
#include "AssetManager.h"
#include "../Audio/Audio.h"
#include "House/wall.h"
#include "Callbacks.hpp"

namespace Scene {
	std::vector<GameObject> _gameObjects;
	std::vector<Wall> _walls;
}

AudioHandle _ropeAudioHandle;

struct CollsionLine {
	glm::vec3 begin;
	glm::vec3 end;
};

void Scene::Init()
{
	_gameObjects.clear();
	_gameObjects.reserve(1000);

	GameObject& chair = _gameObjects.emplace_back(GameObject());
	chair.SetModel("FallenChairBottom");
	chair.SetPosition(glm::vec3(0.8f, 0, 1.1f));
	chair.SetRotationY(3.7f);
	chair.SetMeshMaterial("FallenChair");
	chair.EnableCollision();
	chair.SetBoundingBoxFromMesh(0);

	GameObject& chair1 = _gameObjects.emplace_back(GameObject());
	chair1.SetModel("FallenChairTop");
	chair1.SetPosition(glm::vec3(0.8f, 0, 1.1f));
	chair1.SetRotationY(3.7f);
	chair1.SetMeshMaterial("FallenChair");

	GameObject& floor = _gameObjects.emplace_back(GameObject());
	floor.SetModel("floor");
	floor.SetMeshMaterial("FloorBoards");

	GameObject& bathroomfloor = _gameObjects.emplace_back(GameObject());
	bathroomfloor.SetModel("bathroom_floor");
	bathroomfloor.SetMeshMaterial("BathroomFloor");

	GameObject& ceiling = _gameObjects.emplace_back(GameObject());
	ceiling.SetModel("ceiling");
	ceiling.SetMeshMaterial("Ceiling");

	GameObject& bathroom_ceiling = _gameObjects.emplace_back(GameObject());
	bathroom_ceiling.SetModel("bathroom_ceiling");
	bathroom_ceiling.SetMeshMaterial("Ceiling");

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
		drawers.SetBoundingBoxFromMesh(0); 
		drawers.EnableCollision();

		GameObject& topLeftDrawer = _gameObjects.emplace_back(GameObject());
		topLeftDrawer.SetModel("DrawerTopLeft");
		topLeftDrawer.SetMeshMaterial("Drawers");
		topLeftDrawer.SetScriptName("OpenableDrawer");
		topLeftDrawer.SetParentName("ChestOfDrawers");
		topLeftDrawer.SetName("TopLeftDrawer");
		topLeftDrawer.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f, "DrawerOpen.wav", "DrawerOpen.wav");

		GameObject& topRightDrawer = _gameObjects.emplace_back(GameObject());
		topRightDrawer.SetModel("DrawerTopRight");
		topRightDrawer.SetMeshMaterial("Drawers");
		topRightDrawer.SetScriptName("OpenableDrawer");
		topRightDrawer.SetParentName("ChestOfDrawers");
		topRightDrawer.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f, "DrawerOpen.wav", "DrawerOpen.wav");

		GameObject& drawSecond = _gameObjects.emplace_back(GameObject());
		drawSecond.SetModel("DrawerSecond");
		drawSecond.SetMeshMaterial("Drawers");
		drawSecond.SetScriptName("OpenableDrawer");
		drawSecond.SetParentName("ChestOfDrawers");
		drawSecond.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f, "DrawerOpen.wav", "DrawerOpen.wav");

		GameObject& drawThird = _gameObjects.emplace_back(GameObject());
		drawThird.SetModel("DrawerThird");
		drawThird.SetMeshMaterial("Drawers");
		drawThird.SetScriptName("OpenableDrawer");
		drawThird.SetParentName("ChestOfDrawers");
		drawThird.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f, "DrawerOpen.wav", "DrawerOpen.wav");

		GameObject& drawerFourth = _gameObjects.emplace_back(GameObject());
		drawerFourth.SetModel("DrawerFourth");
		drawerFourth.SetMeshMaterial("Drawers");
		drawerFourth.SetScriptName("OpenableDrawer");
		drawerFourth.SetParentName("ChestOfDrawers");
		drawerFourth.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f, "DrawerOpen.wav", "DrawerOpen.wav");

		GameObject& diary = _gameObjects.emplace_back(GameObject());
		diary.SetParentName("TopLeftDrawer");
		diary.SetModel("Diary");
		diary.SetPosition(-0.3f, 0.95f, 0.4f);
		diary.SetRotationY(0.4f);
		diary.SetScale(0.75f);
		diary.SetMeshMaterial("Diary");
		diary.SetName("Wife's Diary");
		diary.SetInteract(InteractType::PICKUP, "Take your [g]WIFE'S DIARY[w]?", nullptr);
	}

	GameObject& skull2 = _gameObjects.emplace_back(GameObject());
	skull2.SetModel("skull");
	skull2.SetParentName("ChestOfDrawers");
	skull2.SetRotationY(1.6f);
	skull2.SetPosition(glm::vec3(-0.4f, 1.15f, -1.675f + 2.0f));
	skull2.SetMeshMaterial("BlackSkull");
	skull2.SetScale(0.1f);
	skull2.SetParentName("ChestOfDrawers");
	skull2.SetName("Black Skull");
	skull2.SetInteract(InteractType::PICKUP, "Take the [g]BLACK SKULL[w]?", nullptr);
	

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
	wife.SetInteract(InteractType::TEXT, "This can't be fucking happening.", nullptr);


	float DOOR_WIDTH = 0.8f;

	float roomZmin = -1.8f;
	float roomZmax = 1.8f;
	float roomXmin = -2.75f;
	float roomXmax = 1.6f;

	float bathroomXmin = -2.715f;
	float bathroomXmax = -0.76f;
	float bathroomZmin = 1.9f;
	float bathroomZmax = 3.8f;

	float bathroomDoorX = -1.6f;
	float doorWidth = 0.9;
	float hallDoorX = 0.5f;

	GameObject& door0 = _gameObjects.emplace_back(GameObject());
	door0.SetModel("door");
	door0.SetName("Door");
	door0.SetMeshMaterial("Door");
	door0.SetRotationY(NOOSE_HALF_PI);
	door0.SetPosition(hallDoorX + 0.39550f, 0, -1.85 - 0.058520);
	door0.SetScriptName("OpenableDoor");
	door0.SetOpenState(OpenState::CLOSED, 5.208f, NOOSE_HALF_PI, NOOSE_HALF_PI - 1.9f, "Door_Open.wav", "Door_Open.wav");

	GameObject& doorFrame0 = _gameObjects.emplace_back(GameObject());
	doorFrame0.SetModel("door_frame"); 
	doorFrame0.SetName("DoorFrame");
	doorFrame0.SetMeshMaterial("Door");
	doorFrame0.SetRotationY(NOOSE_HALF_PI);
	doorFrame0.SetPosition(hallDoorX, 0, -2.05 + 0.2f);

	GameObject& door1 = _gameObjects.emplace_back(GameObject());
	door1.SetModel("door");
	door1.SetName("Door");
	door1.SetMeshMaterial("Door");
	door1.SetRotationY(NOOSE_HALF_PI);
	door1.SetPosition(bathroomDoorX - 0.39550f, 0, bathroomZmin - 0.05f + 0.058520);
	door1.SetScriptName("OpenableDoor");
	door1.SetOpenState(OpenState::CLOSED, 5.208f, -NOOSE_HALF_PI, -1.9f -NOOSE_HALF_PI, "Door_Open.wav", "Door_Open.wav");


	GameObject& doorFrame1 = _gameObjects.emplace_back(GameObject());
	doorFrame1.SetModel("door_frame");
	doorFrame1.SetName("DoorFrame");
	doorFrame1.SetMeshMaterial("Door");
	doorFrame1.SetRotationY(-NOOSE_HALF_PI);
	doorFrame1.SetPosition(bathroomDoorX, 0, bathroomZmin - 0.05f);


	GameObject& lightSwitch = _gameObjects.emplace_back(GameObject());
	lightSwitch.SetModel("light_switch");
	lightSwitch.SetMeshMaterial("LightSwitch");
	lightSwitch.SetRotationY(-NOOSE_HALF_PI);
	lightSwitch.SetScale(1.05f);
	lightSwitch.SetPosition(-0.12f, 1.1f, -1.8f);

	GameObject& vase = _gameObjects.emplace_back(GameObject());
	vase.SetModel("vase");
	vase.SetRotationY(-0.6f - NOOSE_HALF_PI);
	vase.SetPosition(1.6f - 0.4f, 1.49f, 0.395f);
	vase.SetMeshMaterial("Vase");
	vase.SetName("Vase");
	
	GameObject& keyInVase = _gameObjects.emplace_back(GameObject());
	keyInVase.SetModel("KeyInVase");
	keyInVase.SetRotationY(-0.6f - NOOSE_HALF_PI);
	keyInVase.SetPosition(1.6f - 0.4f, 1.49f, 0.395f);
	keyInVase.SetMeshMaterial("SmallKey");
	keyInVase.SetName("Small Key");
	
	GameObject& flowers = _gameObjects.emplace_back(GameObject());
	flowers.SetModel("flowers");
	flowers.SetRotationY(-0.6f - NOOSE_HALF_PI);
	flowers.SetPosition(1.6f - 0.4f, 1.49f, 0.395f);
	flowers.SetMeshMaterial("Flowers");
	flowers.SetName("Flowers");
	flowers.SetInteract(InteractType::PICKUP, "Take the [g]FLOWERS[w]?", Callbacks::FlowersWereTaken);

	GameObject& _toilet = _gameObjects.emplace_back(GameObject());
	_toilet.SetModel("Toilet");
	_toilet.SetMeshMaterial("Toilet");
	_toilet.SetName("Toilet");
	_toilet.SetPosition(-1.4, 0.0f, 3.8f);

	GameObject& toiletLid = _gameObjects.emplace_back(GameObject());
	toiletLid.SetModel("ToiletLid");
	toiletLid.SetMeshMaterial("Toilet");
	toiletLid.SetPosition(0, 0.40727, -0.2014);
	toiletLid.SetScriptName("OpenableToiletLid");
	toiletLid.SetParentName("Toilet");
	//toiletLid.SetOpenState("OpenableToiletLid");
	//toiletLid.m_openState = OpenState::OPENING;

	GameObject& toiletSeat = _gameObjects.emplace_back(GameObject());
	toiletSeat.SetModel("ToiletSeat");
	toiletSeat.SetMeshMaterial("Toilet");
	toiletSeat.SetPosition(0, 0.40727, -0.2014);
	toiletSeat.SetScriptName("OpenableToiletSeat");
	toiletSeat.SetParentName("Toilet");

	GameObject& bathroomHeater = _gameObjects.emplace_back(GameObject());
	bathroomHeater.SetModel("Heater");
	bathroomHeater.SetMeshMaterial("Heater");
	bathroomHeater.SetPosition(-0.76f, 0.0f, 3.3f);

	GameObject& toiletPaper = _gameObjects.emplace_back(GameObject());
	toiletPaper.SetModel("ToiletPaper");
	toiletPaper.SetMeshMaterial("Toilet");
	toiletPaper.SetPosition(-1.8, 0.7f, 3.8f);

	GameObject& basin = _gameObjects.emplace_back(GameObject());
	basin.SetModel("Basin");
	basin.SetMeshMaterial("Basin");
	basin.SetPosition(-0.76f, 0.0f, 2.55f);

	GameObject& towel = _gameObjects.emplace_back(GameObject());
	towel.SetModel("Towel");
	towel.SetMeshMaterial("Basin");
	towel.SetPosition(-0.76f, 1.8f, 3.3f);

	GameObject& bin = _gameObjects.emplace_back(GameObject());
	bin.SetModel("BathroomBin");
	bin.SetMeshMaterial("BathroomBin");
	bin.SetName("BathroomBin");
	bin.SetPosition(-0.81, 0.0f, 2.15f);

	GameObject& bathroomBinPedal = _gameObjects.emplace_back(GameObject());
	bathroomBinPedal.SetModel("BathroomBinPedal");
	bathroomBinPedal.SetMeshMaterial("BathroomBin");
	bathroomBinPedal.SetParentName("BathroomBin");

	GameObject& bathroomBinLid = _gameObjects.emplace_back(GameObject());
	bathroomBinLid.SetModel("BathroomBinLid");
	bathroomBinLid.SetMeshMaterial("BathroomBin");
	bathroomBinLid.SetParentName("BathroomBin");
	bathroomBinLid.SetPositionY(0.22726f);

	GameObject& cabinet = _gameObjects.emplace_back(GameObject());
	cabinet.SetModel("CabinetBody");
	cabinet.SetMeshMaterial("Cabinet");
	cabinet.SetName("Cabinet");
	cabinet.SetPosition(-0.76f, 1.25f, 2.55f);

	GameObject& cabinetDoor = _gameObjects.emplace_back(GameObject());
	cabinetDoor.SetModel("CabinetDoor");
	cabinetDoor.SetMeshMaterial("Cabinet");
	cabinetDoor.SetName("Cabinet Door");
	cabinetDoor.SetParentName("Cabinet");
	cabinetDoor.SetPosition(-0.10763f, 0, 0.24941);
	cabinetDoor.SetOpenState(OpenState::CLOSED, 9, 0, NOOSE_HALF_PI, "CabinetOpen.wav", "CabinetClose.wav");

	GameObject& cabinetMirror = _gameObjects.emplace_back(GameObject());
	cabinetMirror.SetModel("CabinetMirror");
	cabinetMirror.SetMeshMaterial("NumGrid");
	cabinetMirror.SetParentName("Cabinet Door");
	cabinetMirror.SetScriptName("OpenCabinetDoor");
	cabinetMirror.SetInteract(InteractType::TEXT, "", Callbacks::OpenCabinet);

	GameObject& smallChestOfDrawers = _gameObjects.emplace_back(GameObject());
	smallChestOfDrawers.SetModel("SmallChestOfDrawersFrame");
	smallChestOfDrawers.SetMeshMaterial("Drawers");
	smallChestOfDrawers.SetName("SmallDrawersHis");
	smallChestOfDrawers.SetPosition(-2.75, 0, -1.3);
	smallChestOfDrawers.SetRotationY(NOOSE_PI / 2);
	smallChestOfDrawers.SetBoundingBoxFromMesh(0);
	smallChestOfDrawers.EnableCollision();

	GameObject& smallChestOfDrawer_1 = _gameObjects.emplace_back(GameObject());
	smallChestOfDrawer_1.SetModel("SmallDrawerTop");
	smallChestOfDrawer_1.SetMeshMaterial("Drawers");
	smallChestOfDrawer_1.SetParentName("SmallDrawersHis");
	smallChestOfDrawer_1.SetScriptName("OpenableDrawer");
	smallChestOfDrawer_1.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f, "DrawerOpen.wav", "DrawerOpen.wav");

	GameObject& smallChestOfDrawer_2 = _gameObjects.emplace_back(GameObject());
	smallChestOfDrawer_2.SetModel("SmallDrawerSecond");
	smallChestOfDrawer_2.SetMeshMaterial("Drawers");
	smallChestOfDrawer_2.SetParentName("SmallDrawersHis");
	smallChestOfDrawer_2.SetScriptName("OpenableDrawer");
	smallChestOfDrawer_2.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f, "DrawerOpen.wav", "DrawerOpen.wav");

	GameObject& smallChestOfDrawer_3 = _gameObjects.emplace_back(GameObject());
	smallChestOfDrawer_3.SetModel("SmallDrawerThird");
	smallChestOfDrawer_3.SetMeshMaterial("Drawers");
	smallChestOfDrawer_3.SetParentName("SmallDrawersHis");
	smallChestOfDrawer_3.SetScriptName("OpenableDrawer");
	smallChestOfDrawer_3.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f, "DrawerOpen.wav", "DrawerOpen.wav");

	GameObject& smallChestOfDrawer_4 = _gameObjects.emplace_back(GameObject());
	smallChestOfDrawer_4.SetModel("SmallDrawerFourth");
	smallChestOfDrawer_4.SetMeshMaterial("Drawers");
	smallChestOfDrawer_4.SetParentName("SmallDrawersHis");
	smallChestOfDrawer_4.SetScriptName("OpenableDrawer");
	smallChestOfDrawer_4.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f, "DrawerOpen.wav", "DrawerOpen.wav");


	GameObject& smallChestOfDrawersB = _gameObjects.emplace_back(GameObject());
	smallChestOfDrawersB.SetModel("SmallChestOfDrawersFrame");
	smallChestOfDrawersB.SetMeshMaterial("Drawers");
	smallChestOfDrawersB.SetName("SmallDrawersHers");
	smallChestOfDrawersB.SetPosition(-2.75, 0, 1.3);
	smallChestOfDrawersB.SetRotationY(NOOSE_PI / 2);
	smallChestOfDrawersB.SetBoundingBoxFromMesh(0);
	smallChestOfDrawersB.EnableCollision();

	GameObject& smallChestOfDrawer_1B = _gameObjects.emplace_back(GameObject());
	smallChestOfDrawer_1B.SetModel("SmallDrawerTop");
	smallChestOfDrawer_1B.SetMeshMaterial("Drawers");
	smallChestOfDrawer_1B.SetParentName("SmallDrawersHers");
	smallChestOfDrawer_1B.SetName("LockedSmallDrawer");
	smallChestOfDrawer_1B.SetInteract(InteractType::TEXT, "It's locked.", nullptr);
	smallChestOfDrawer_1B.SetOnInteractAudio("Locked1.wav");

	GameObject& smallChestOfDrawer_2B = _gameObjects.emplace_back(GameObject());
	smallChestOfDrawer_2B.SetModel("SmallDrawerSecond");
	smallChestOfDrawer_2B.SetMeshMaterial("Drawers");
	smallChestOfDrawer_2B.SetParentName("SmallDrawersHers");
	smallChestOfDrawer_2B.SetScriptName("OpenableDrawer");
	smallChestOfDrawer_2B.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f, "DrawerOpen.wav", "DrawerOpen.wav");

	GameObject& smallChestOfDrawer_3B = _gameObjects.emplace_back(GameObject());
	smallChestOfDrawer_3B.SetModel("SmallDrawerThird");
	smallChestOfDrawer_3B.SetMeshMaterial("Drawers");
	smallChestOfDrawer_3B.SetParentName("SmallDrawersHers");
	smallChestOfDrawer_3B.SetScriptName("OpenableDrawer");
	smallChestOfDrawer_3B.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f, "DrawerOpen.wav", "DrawerOpen.wav");

	GameObject& smallChestOfDrawer_4B = _gameObjects.emplace_back(GameObject());
	smallChestOfDrawer_4B.SetModel("SmallDrawerFourth");
	smallChestOfDrawer_4B.SetMeshMaterial("Drawers");
	smallChestOfDrawer_4B.SetParentName("SmallDrawersHers");
	smallChestOfDrawer_4B.SetScriptName("OpenableDrawer");
	smallChestOfDrawer_4B.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f, "DrawerOpen.wav", "DrawerOpen.wav");

	/*
	EntityStatic& smallChestOfDrawer_1 = smallChestOfDrawers.m_children.emplace_back(EntityStatic());
	smallChestOfDrawer_1.SetModel("SmallDrawerTop.obj");
	smallChestOfDrawer_1.SetMaterial("Drawers");
	smallChestOfDrawer_1.m_scriptName = "OpenableDrawer";
	EntityStatic& smallChestOfDrawer_2 = smallChestOfDrawers.m_children.emplace_back(EntityStatic());
	smallChestOfDrawer_2.SetModel("SmallDrawerSecond.obj");
	smallChestOfDrawer_2.SetMaterial("Drawers");
	smallChestOfDrawer_2.m_scriptName = "OpenableDrawer";
	EntityStatic& smallChestOfDrawer_3 = smallChestOfDrawers.m_children.emplace_back(EntityStatic());
	smallChestOfDrawer_3.SetModel("SmallDrawerThird.obj");
	smallChestOfDrawer_3.SetMaterial("Drawers");
	smallChestOfDrawer_3.m_scriptName = "OpenableDrawer";
	EntityStatic& smallChestOfDrawer_4 = smallChestOfDrawers.m_children.emplace_back(EntityStatic());
	smallChestOfDrawer_4.SetModel("SmallDrawerFourth.obj");
	smallChestOfDrawer_4.SetMaterial("Drawers");
	smallChestOfDrawer_4.m_scriptName = "OpenableDrawer";
	*/

	/*	X		   C  
	 	|	  ___________
		|	 |	     Wife|	
		|	 |			 |A
		|	 |			 |   E
		|  B |			 |______
		|	 |			 |      |
		|	 |			 |______| G  
		|	 |___________|H  I
	    |    
		|		   D        F
	    (0,0)----------------- Z
	*/

	static bool runOnce = true;
	if (runOnce) {
		Wall& wallA = _walls.emplace_back(glm::vec3(roomXmax, 0, roomZmax), glm::vec3(bathroomDoorX + doorWidth / 2, 0, roomZmax), "WallPaper");
		Wall& wallH = _walls.emplace_back(glm::vec3(bathroomDoorX - doorWidth / 2, 0, roomZmax), glm::vec3(roomXmin, 0, roomZmax), "WallPaper");
		Wall& wallAboveBathroomDoor = _walls.emplace_back(glm::vec3(bathroomDoorX + doorWidth / 2, 2, roomZmax), glm::vec3(bathroomDoorX - doorWidth / 2, 2, roomZmax), "WallPaper");
		Wall& wallB = _walls.emplace_back(glm::vec3(roomXmin, 0, roomZmin), glm::vec3(hallDoorX - doorWidth / 2, 0, -roomZmax), "WallPaper");
		Wall& wallB2 = _walls.emplace_back(glm::vec3(hallDoorX + doorWidth / 2, 0, roomZmin), glm::vec3(roomXmax, 0, roomZmin), "WallPaper");
		Wall& wallBC = _walls.emplace_back(glm::vec3(hallDoorX - doorWidth / 2, 2, -roomZmax), glm::vec3(hallDoorX + doorWidth / 2, 2, roomZmin), "WallPaper");
		Wall& wallC = _walls.emplace_back(glm::vec3(roomXmax, 0, roomZmin), glm::vec3(roomXmax, 0, roomZmax), "WallPaper");
		Wall& wallD = _walls.emplace_back(glm::vec3(roomXmin, 0, roomZmax), glm::vec3(roomXmin, 0, roomZmin), "WallPaper");


		Wall& wallE = _walls.emplace_back(glm::vec3(bathroomXmax, 0, bathroomZmin), glm::vec3(bathroomXmax, 0, bathroomZmax), "BathroomWall");
		Wall& wallG = _walls.emplace_back(glm::vec3(bathroomXmax, 0, bathroomZmax), glm::vec3(bathroomXmin, 0, bathroomZmax), "BathroomWall");
		Wall& wallI = _walls.emplace_back(glm::vec3(bathroomXmin, 0, bathroomZmax), glm::vec3(bathroomXmin, 0, bathroomZmin), "BathroomWall");
		Wall& wallHI2DOOR = _walls.emplace_back(glm::vec3(bathroomXmin, 0, bathroomZmin), glm::vec3(bathroomDoorX - doorWidth / 2, 0, bathroomZmin), "BathroomWall");
		Wall& otherGapNextToBathroomDoor = _walls.emplace_back(glm::vec3(bathroomDoorX + doorWidth / 2, 0, bathroomZmin), glm::vec3(bathroomXmax, 0, bathroomZmin), "BathroomWall");
		Wall& aboveBathroomDoor = _walls.emplace_back(glm::vec3(bathroomDoorX - doorWidth / 2, 2, bathroomZmin), glm::vec3(bathroomDoorX + doorWidth / 2, 2, bathroomZmin), "BathroomWall");
		runOnce = false;
	}
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
	std::cout << "Scene::GetGameObjectByName() failed, no object with name \"" << name << "\"\n";
	return nullptr;
}

GameObject* Scene::GetGameObjectByIndex(int index) {
	if (index >= 0 && index < _gameObjects.size())
		return &_gameObjects[index];
	else
		return nullptr;
}

int Scene::GetGameObjectCount() {
	return _gameObjects.size();
}

void Scene::Update(float deltaTime) {

	// Player pressed E to interact?
	if (_hoveredGameObject && 
		_hoveredGameObject->IsInteractable() && 
		Input::KeyPressed(HELL_KEY_E) &&
		!GameData::GetPlayer().m_interactDisabled) {

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
		MeshRenderInfo info;
		info._vkTransform = Util::GetIdentiyVkTransformMatrixKHR();
		info._deviceAddress = mesh->_accelerationStructure.deviceAddress;
		info._modelMatrix = glm::mat4(1);
		info._basecolor = wall._material->_basecolor;
		info._normal = wall._material->_normal;
		info._rma = wall._material->_rma;
		info._vertexOffset = mesh->_vertexOffset;
		info._indexOffset = mesh->_indexOffset;
		info._parent = &wall;
		info._mesh = mesh;
		infos.push_back(info);
	}

	static bool runOnce = true;
	if (runOnce) {
		for (int i = 0; i < infos.size(); i++) {
			//std::cout << i << " " << infos[i]._mesh->_name << " " << "\n";
		}
	}
	runOnce = false;

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

/*void Scene::ResetCollectedItems() {

	for (GameObject& gameObject : _gameObjects) {
		gameObject.ResetToInitialState();
	}
}*/

std::vector<Vertex> Scene::GetCollisionLineVertices() {

	std::vector<Vertex> vertices;
	for (Wall& wall : _walls) {
		if (wall._begin.y < 1) {
			vertices.push_back(Vertex(wall._begin));
			vertices.push_back(Vertex(wall._end));
		}
	}
	for (GameObject& gameObject : _gameObjects) {

		if (gameObject.GetName() == "Door") {
			glm::mat4 rotationMatrix = gameObject.GetRotationMatrix();
			const float doorWidth = -0.794f;
			const float doorDepth = -0.0379f;
			// Front
			glm::vec3 pos0 = gameObject.GetPosition();
			glm::vec3 pos1 = gameObject.GetPosition() + Util::Translate(rotationMatrix, glm::vec3(0.0, 0, doorWidth));
			// Back
			glm::vec3 pos2 = gameObject.GetPosition() + Util::Translate(rotationMatrix, glm::vec3(doorDepth, 0, 0));
			glm::vec3 pos3 = gameObject.GetPosition() + Util::Translate(rotationMatrix, glm::vec3(doorDepth, 0, doorWidth));
			vertices.push_back(Vertex(pos0));
			vertices.push_back(Vertex(pos1));
			vertices.push_back(Vertex(pos2));
			vertices.push_back(Vertex(pos3));
			vertices.push_back(Vertex(pos0));
			vertices.push_back(Vertex(pos2));
			vertices.push_back(Vertex(pos1));
			vertices.push_back(Vertex(pos3));
		}

		else if (gameObject.GetName() == "DoorFrame") {
			glm::mat4 rotationMatrix = gameObject.GetRotationMatrix();
			// Front
			glm::vec3 pos0 = gameObject.GetPosition() +Util::Translate(rotationMatrix, glm::vec3(-0.05, 0, -0.45));
			glm::vec3 pos1 = gameObject.GetPosition() + Util::Translate(rotationMatrix, glm::vec3(0.05, 0, -0.45));
			// Back
			glm::vec3 pos2 = gameObject.GetPosition() + Util::Translate(rotationMatrix, glm::vec3(-0.05, 0, +0.45));
			glm::vec3 pos3 = gameObject.GetPosition() + Util::Translate(rotationMatrix, glm::vec3(0.05, 0, +0.45));
			vertices.push_back(Vertex(pos0));
			vertices.push_back(Vertex(pos1));
			vertices.push_back(Vertex(pos2));
			vertices.push_back(Vertex(pos3));
		}

		else if (gameObject.HasCollisionsEnabled()) {
			glm::mat4 rotationMatrix = gameObject.GetRotationMatrix();
			BoundingBox box = gameObject.GetBoundingBox();
			// Front
			glm::vec3 pos0 = gameObject.GetPosition() + Util::Translate(rotationMatrix, glm::vec3(box.xLow, 0, box.zLow));
			glm::vec3 pos1 = gameObject.GetPosition() + Util::Translate(rotationMatrix, glm::vec3(box.xHigh, 0, box.zLow));
			// Back
			glm::vec3 pos2 = gameObject.GetPosition() + Util::Translate(rotationMatrix, glm::vec3(box.xLow, 0, box.zHigh));
			glm::vec3 pos3 = gameObject.GetPosition() + Util::Translate(rotationMatrix, glm::vec3(box.xHigh, 0, box.zHigh));
			vertices.push_back(Vertex(pos0));
			vertices.push_back(Vertex(pos1));
			vertices.push_back(Vertex(pos2));
			vertices.push_back(Vertex(pos3));
			vertices.push_back(Vertex(pos0));
			vertices.push_back(Vertex(pos2));
			vertices.push_back(Vertex(pos1));
			vertices.push_back(Vertex(pos3));
		}
	}
	return vertices;
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