#include "Scene.h"
#include "AssetManager.h"
#include "../Audio/Audio.h"
#include "House/wall.h"
#include "Callbacks.hpp"

namespace Scene {
	std::vector<GameObject> _gameObjects;
	std::vector<GameObject> _inventoryGameObjects;
	std::vector<Wall> _walls;
	std::vector<Wall> _inventoryWalls;
	std::vector<Light> _lights;
	std::vector<Light> _lightsInventory;
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
	_lights.clear();

	Light& light = _lights.emplace_back(Light());
	light.position = { -0.6, 2.1, -0 };
	light.color = { 1, 0.95, 0.8 };
	//light.color += glm::vec3(0.1);
	light.state = Light::State::ON;
	light.currentBrightness = 1.0f;

	Light& light2 = _lights.emplace_back(Light());
	light2.position = glm::vec3(-1.7376, 2, 2.85);
	light2.color = glm::vec3(1, 0.95, 0.8) * glm::vec3(1, 0.95, 0.8) * glm::vec3(0.45);
	light2.state = Light::State::ON;
	light2.currentBrightness = 1.0f;

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
	//floor.SetScale(glm::vec3(1, -1, 1));

	GameObject& bathroomfloor = _gameObjects.emplace_back(GameObject());
	bathroomfloor.SetModel("bathroom_floor");
	bathroomfloor.SetMeshMaterial("BathroomFloor");
	//bathroomfloor.SetScale(glm::vec3(1, -1, 1));

	GameObject& ceiling = _gameObjects.emplace_back(GameObject());
	ceiling.SetModel("ceiling");
	ceiling.SetMeshMaterial("Ceiling");

	GameObject& bathroom_ceiling = _gameObjects.emplace_back(GameObject());
	bathroom_ceiling.SetModel("bathroom_ceiling");
	bathroom_ceiling.SetMeshMaterial("Ceiling");
	//bathroom_ceiling.SetScale(glm::vec3(1, -1, 1));

	GameObject& skull = _gameObjects.emplace_back(GameObject());
	skull.SetModel("BlackSkull");
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

		float drawerVolume = 1.0f;
		GameObject& topLeftDrawer = _gameObjects.emplace_back(GameObject());
		topLeftDrawer.SetModel("DrawerTopLeft");
		topLeftDrawer.SetMeshMaterial("Drawers");
		topLeftDrawer.SetScriptName("OpenableDrawer");
		topLeftDrawer.SetParentName("ChestOfDrawers");
		topLeftDrawer.SetName("TopLeftDrawer");
		topLeftDrawer.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
		topLeftDrawer.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
		topLeftDrawer.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
		topLeftDrawer.SetOpenAxis(OpenAxis::TRANSLATE_Z);

		GameObject& topRightDrawer = _gameObjects.emplace_back(GameObject());
		topRightDrawer.SetModel("DrawerTopRight");
		topRightDrawer.SetMeshMaterial("Drawers");
		topRightDrawer.SetScriptName("OpenableDrawer");
		topRightDrawer.SetParentName("ChestOfDrawers");
		topRightDrawer.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
		topRightDrawer.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
		topRightDrawer.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
		topRightDrawer.SetOpenAxis(OpenAxis::TRANSLATE_Z);

		GameObject& drawSecond = _gameObjects.emplace_back(GameObject());
		drawSecond.SetModel("DrawerSecond");
		drawSecond.SetMeshMaterial("Drawers");
		drawSecond.SetScriptName("OpenableDrawer");
		drawSecond.SetParentName("ChestOfDrawers");
		drawSecond.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
		drawSecond.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
		drawSecond.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
		drawSecond.SetOpenAxis(OpenAxis::TRANSLATE_Z);

		GameObject& drawThird = _gameObjects.emplace_back(GameObject());
		drawThird.SetModel("DrawerThird");
		drawThird.SetMeshMaterial("Drawers");
		drawThird.SetScriptName("OpenableDrawer");
		drawThird.SetParentName("ChestOfDrawers");
		drawThird.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
		drawThird.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
		drawThird.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
		drawThird.SetOpenAxis(OpenAxis::TRANSLATE_Z);

		GameObject& drawerFourth = _gameObjects.emplace_back(GameObject());
		drawerFourth.SetModel("DrawerFourth");
		drawerFourth.SetMeshMaterial("Drawers");
		drawerFourth.SetScriptName("OpenableDrawer");
		drawerFourth.SetParentName("ChestOfDrawers");
		drawerFourth.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
		drawerFourth.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
		drawerFourth.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
		drawerFourth.SetOpenAxis(OpenAxis::TRANSLATE_Z);
	}

	GameObject& skull2 = _gameObjects.emplace_back(GameObject());
	skull2.SetModel("BlackSkull");
	skull2.SetParentName("ChestOfDrawers");
	skull2.SetRotationY(1.6f);
	skull2.SetPosition(glm::vec3(-0.4f, 1.15f, -1.675f + 2.0f));
	skull2.SetMeshMaterial("BlackSkull");
	skull2.SetScale(0.1f);
	skull2.SetParentName("ChestOfDrawers");
	skull2.SetName("Black Skull");
	skull2.SetInteract(InteractType::PICKUP, "Take the [g]BLACK SKULL[w]?", nullptr);
	

	GameObject& wife = _gameObjects.emplace_back(GameObject());
	wife.SetModel("Wife");
	wife.SetName("Wife");
	wife.SetPosition(0.6f, 2.6f, 0.9f);
	wife.SetRotationY(-1.75f);
	wife.SetScale(0.01f);
	wife.SetMeshMaterialByMeshName("Side_part_wavy", "WifeHair");
	wife.SetMeshMaterialByMeshName("CC_Base_Eye", "WifeEye");
	wife.SetMeshMaterialByMeshName("CC_Base_Teeth", "WifeTeeth");
	wife.SetMeshMaterialByMeshName("CC_Game_Body", "WifeSkin");
	wife.SetMeshMaterialByMeshName("CC_Game_Tongue", "WifeSkin");
	wife.SetMeshMaterialByMeshName("Dress", "WifeDress");
	wife.SetMeshMaterialByMeshName("F_BS_wDress_L", "WifeDress2");
	wife.SetMeshMaterialByMeshName("WifeDress_low", "WifeDress2");
	wife.SetMeshMaterialByMeshName("WifeDress_low_0", "WifeDress2");
	wife.SetMeshMaterialByMeshName("Noose_pivot", "Noose");
	wife.SetMeshMaterialByMeshName("Noose", "Noose");
	wife.SetMeshMaterialByMeshName("Object001", "Noose");
	wife.SetMeshMaterialByMeshName("Rope_Low", "Noose");

	/*wife.SetMeshMaterial("WifeHair", 0);
	wife.SetMeshMaterial("WifeSkin", 1);
	wife.SetMeshMaterial("WifeSkin", 2);
	wife.SetMeshMaterial("WifeBrows", 3);
	wife.SetMeshMaterial("WifeDress", 4);
	wife.SetMeshMaterial("WifeEye", 5);
	wife.SetMeshMaterial("WifeTeeth", 6);
	wife.SetMeshMaterial("Noose", 7);
	wife.SetMeshMaterial("Noose", 8);*/
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
	door0.SetModel("Door");
	door0.SetName("Door");
	door0.SetMeshMaterial("Door");
	door0.SetRotationY(NOOSE_HALF_PI);
	door0.SetPosition(hallDoorX + 0.39550f, 0, -1.85 - 0.058520);
	door0.SetScriptName("OpenableDoor");
	//door0.SetOpenState(OpenState::CLOSED, 5.208f, NOOSE_HALF_PI, -NOOSE_HALF_PI - 1.9f);
	door0.SetOpenState(OpenState::CLOSED, 5.208f, NOOSE_HALF_PI, NOOSE_HALF_PI - 1.9f);
	door0.SetAudioOnOpen("Door_Open.wav", DOOR_VOLUME);
	door0.SetAudioOnClose("Door_Open.wav", DOOR_VOLUME);
	door0.SetOpenAxis(OpenAxis::ROTATION_NEG_Y);

	GameObject& doorFrame0 = _gameObjects.emplace_back(GameObject());
	doorFrame0.SetModel("DoorFrame"); 
	doorFrame0.SetName("DoorFrame");
	doorFrame0.SetMeshMaterial("Door");
	doorFrame0.SetRotationY(NOOSE_HALF_PI);
	doorFrame0.SetPosition(hallDoorX, 0, -2.05 + 0.2f);

	GameObject& door1 = _gameObjects.emplace_back(GameObject());
	door1.SetModel("Door");
	door1.SetName("Door");
	door1.SetMeshMaterial("Door");
	door1.SetRotationY(NOOSE_HALF_PI);
	door1.SetPosition(bathroomDoorX - 0.39550f, 0, bathroomZmin - 0.05f + 0.058520);
	door1.SetScriptName("OpenableDoor");
	door1.SetOpenState(OpenState::CLOSED, 5.208f, -NOOSE_HALF_PI, -1.9f - NOOSE_HALF_PI);
	door1.SetAudioOnOpen("Door_Open.wav", DOOR_VOLUME);
	door1.SetAudioOnClose("Door_Open.wav", DOOR_VOLUME);
	door1.SetOpenAxis(OpenAxis::ROTATION_NEG_Y);


	GameObject& doorFrame1 = _gameObjects.emplace_back(GameObject());
	doorFrame1.SetModel("DoorFrame");
	doorFrame1.SetName("DoorFrame");
	doorFrame1.SetMeshMaterial("Door");
	doorFrame1.SetRotationY(-NOOSE_HALF_PI);
	doorFrame1.SetPosition(bathroomDoorX, 0, bathroomZmin - 0.05f);


	GameObject& lightSwitch = _gameObjects.emplace_back(GameObject());
	lightSwitch.SetModel("LightSwitchOn");
	lightSwitch.SetMeshMaterial("LightSwitch");
	lightSwitch.SetName("LightswitchBedroom");
	lightSwitch.SetRotationY(-NOOSE_HALF_PI);
	lightSwitch.SetScale(1.05f);
	lightSwitch.SetPosition(-0.12f, 1.1f, -1.8f);
	lightSwitch.SetInteract(InteractType::CALLBACK_ONLY, "", Callbacks::TurnBedroomLightOff);

	GameObject& vase = _gameObjects.emplace_back(GameObject());
	vase.SetModel("Vase");
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
	flowers.SetModel("Flowers");
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
	toiletLid.SetName("ToiletLid");
	toiletLid.SetMeshMaterial("Toilet");
	toiletLid.SetPosition(0, 0.40727, -0.2014);
	toiletLid.SetParentName("Toilet");
	toiletLid.SetOpenState(OpenState::OPENING, 12.0f, 0, -(NOOSE_PI / 2) - 0.12);
	toiletLid.SetAudioOnOpen("ToiletLidUp.wav", 0.75f);
	toiletLid.SetAudioOnClose("ToiletLidDown.wav", 0.5f);
	toiletLid.SetOpenAxis(OpenAxis::ROTATION_NEG_X);

	GameObject& toiletSeat = _gameObjects.emplace_back(GameObject());
	toiletSeat.SetModel("ToiletSeat");
	toiletSeat.SetName("ToiletSeat");
	toiletSeat.SetMeshMaterial("Toilet");
	toiletSeat.SetPosition(0, 0.40727, -0.2014);
	toiletSeat.SetAudioOnOpen("ToiletSeatUp.wav", 0.75f);
	toiletSeat.SetAudioOnClose("ToiletSeatDown.wav", 0.5f);
	toiletSeat.SetParentName("Toilet");
	toiletSeat.SetOpenState(OpenState::CLOSED, 12.0f, 0, (NOOSE_PI / 2) + 0.12);
	toiletSeat.SetOpenAxis(OpenAxis::ROTATION_POS_X);

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
	cabinetDoor.SetAudioOnOpen("CabinetOpen.wav", CABINET_VOLUME);
	cabinetDoor.SetAudioOnClose("CabinetClose.wav", CABINET_VOLUME);
	cabinetDoor.SetOpenState(OpenState::CLOSED, 9, 0, NOOSE_HALF_PI);
	cabinetDoor.SetOpenAxis(OpenAxis::ROTATION_POS_Y);

	GameObject& cabinetMirror = _gameObjects.emplace_back(GameObject());
	cabinetMirror.SetModel("CabinetMirror");
	cabinetMirror.SetMeshMaterial("NumGrid");
	cabinetMirror.SetMeshMaterial("Cabinet");
	cabinetMirror.SetParentName("Cabinet Door");
	cabinetMirror.SetScriptName("OpenCabinetDoor");
	cabinetMirror.SetInteract(InteractType::TEXT, "", Callbacks::OpenCabinet);
	cabinetMirror.SetMaterialType(MaterialType::MIRROR);

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
	smallChestOfDrawer_1.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
	smallChestOfDrawer_1.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
	smallChestOfDrawer_1.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
	smallChestOfDrawer_1.SetOpenAxis(OpenAxis::TRANSLATE_Z);

	GameObject& smallChestOfDrawer_2 = _gameObjects.emplace_back(GameObject());
	smallChestOfDrawer_2.SetModel("SmallDrawerSecond");
	smallChestOfDrawer_2.SetMeshMaterial("Drawers");
	smallChestOfDrawer_2.SetParentName("SmallDrawersHis");
	smallChestOfDrawer_2.SetScriptName("OpenableDrawer");
	smallChestOfDrawer_2.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
	smallChestOfDrawer_2.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
	smallChestOfDrawer_2.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
	smallChestOfDrawer_2.SetOpenAxis(OpenAxis::TRANSLATE_Z);

	GameObject& smallChestOfDrawer_3 = _gameObjects.emplace_back(GameObject());
	smallChestOfDrawer_3.SetModel("SmallDrawerThird");
	smallChestOfDrawer_3.SetMeshMaterial("Drawers");
	smallChestOfDrawer_3.SetParentName("SmallDrawersHis");
	smallChestOfDrawer_3.SetScriptName("OpenableDrawer");
	smallChestOfDrawer_3.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
	smallChestOfDrawer_3.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
	smallChestOfDrawer_3.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
	smallChestOfDrawer_3.SetOpenAxis(OpenAxis::TRANSLATE_Z);

	GameObject& smallChestOfDrawer_4 = _gameObjects.emplace_back(GameObject());
	smallChestOfDrawer_4.SetModel("SmallDrawerFourth");
	smallChestOfDrawer_4.SetMeshMaterial("Drawers");
	smallChestOfDrawer_4.SetParentName("SmallDrawersHis");
	smallChestOfDrawer_4.SetScriptName("OpenableDrawer");
	smallChestOfDrawer_4.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
	smallChestOfDrawer_4.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
	smallChestOfDrawer_4.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
	smallChestOfDrawer_4.SetOpenAxis(OpenAxis::TRANSLATE_Z);


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
	//smallChestOfDrawer_1B.SetInteract(InteractType::TEXT, "It's locked.", nullptr);
	smallChestOfDrawer_1B.SetInteract(InteractType::CALLBACK_ONLY, "", Callbacks::LockedDrawer);
	//smallChestOfDrawer_1B.SetAudioOnInteract("Locked1.wav", 0.25f);

	GameObject& smallChestOfDrawer_2B = _gameObjects.emplace_back(GameObject());
	smallChestOfDrawer_2B.SetModel("SmallDrawerSecond");
	smallChestOfDrawer_2B.SetMeshMaterial("Drawers");
	smallChestOfDrawer_2B.SetParentName("SmallDrawersHers");
	smallChestOfDrawer_2B.SetScriptName("OpenableDrawer");
	smallChestOfDrawer_2B.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
	smallChestOfDrawer_2B.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
	smallChestOfDrawer_2B.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
	smallChestOfDrawer_2B.SetOpenAxis(OpenAxis::TRANSLATE_Z);

	GameObject& smallChestOfDrawer_3B = _gameObjects.emplace_back(GameObject());
	smallChestOfDrawer_3B.SetModel("SmallDrawerThird");
	smallChestOfDrawer_3B.SetMeshMaterial("Drawers");
	smallChestOfDrawer_3B.SetParentName("SmallDrawersHers");
	smallChestOfDrawer_3B.SetScriptName("OpenableDrawer");
	smallChestOfDrawer_3B.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
	smallChestOfDrawer_3B.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
	smallChestOfDrawer_3B.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
	smallChestOfDrawer_3B.SetOpenAxis(OpenAxis::TRANSLATE_Z);

	GameObject& smallChestOfDrawer_4B = _gameObjects.emplace_back(GameObject());
	smallChestOfDrawer_4B.SetModel("SmallDrawerFourth");
	smallChestOfDrawer_4B.SetMeshMaterial("Drawers");
	smallChestOfDrawer_4B.SetParentName("SmallDrawersHers");
	smallChestOfDrawer_4B.SetScriptName("OpenableDrawer");
	smallChestOfDrawer_4B.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
	smallChestOfDrawer_4B.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
	smallChestOfDrawer_4B.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
	smallChestOfDrawer_4B.SetOpenAxis(OpenAxis::TRANSLATE_Z);

	GameObject& diary = _gameObjects.emplace_back(GameObject());
	diary.SetParentName("LockedSmallDrawer");
	diary.SetModel("Diary");
	//diary.SetPosition(-0.3f, 0.95f, 0.4f);
	diary.SetPosition(0.0f, 0.65f, 0.3f);
	diary.SetRotationY(0.4f);
	diary.SetScale(0.75f);
	diary.SetMeshMaterial("Diary");
	diary.SetName("Wife's Diary");
	diary.SetInteract(InteractType::PICKUP, "Take your [g]WIFE'S DIARY[w]?", nullptr);

	GameObject& yourPhone = _gameObjects.emplace_back(GameObject());
	yourPhone.SetModel("YourPhone");
	yourPhone.SetName("Water Damaged Phone");
	yourPhone.SetMeshMaterial("Phone");
	yourPhone.SetInteract(InteractType::PICKUP, "Take the [g]WATER DAMAGED PHONE[w]?", nullptr);
	yourPhone.SetPosition(-1.39f, 0.20f, 3.29f);
	yourPhone.SetRotationX(-NOOSE_HALF_PI + 0.5);
	yourPhone.SetRotationY(0.3);
	yourPhone.SetScale(0.95f);


	GameObject& bed = _gameObjects.emplace_back(GameObject());
	bed.SetModel("BedNoPillows");
	bed.SetMeshMaterial("BedFabrics");
	bed.SetPosition(-2.75, 0.0, 0);
	bed.SetRotationY(NOOSE_PI / 2);
	bed.SetName("Bed");
	bed.EnableCollision();
	bed.SetBoundingBoxFromMesh(0);

	GameObject& pillow = _gameObjects.emplace_back(GameObject());
	pillow.SetParentName("Bed");
	pillow.SetPosition(0, 0.3, 0);
	pillow.SetModel("PillowHers");
	pillow.SetMeshMaterial("BedFabrics");

	GameObject& pillow2 = _gameObjects.emplace_back(GameObject());
	pillow2.SetModel("PillowHis");
	pillow2.SetPosition(0, 0.3, 0);
	pillow2.SetParentName("Bed");
	pillow2.SetMeshMaterial("BedFabrics");


	GameObject& cube = _gameObjects.emplace_back(GameObject());
	cube.SetModel("Cube");
	cube.SetMeshMaterial("White");
	cube.SetScale(glm::vec3(0.5, 0.95, 1.8));
	cube.SetPosition(-0.4, 0, 0.6);
	cube.SetRotationY(-1.1);
	cube.SetName("Cube");
	cube.SetMaterialType(MaterialType::MIRROR);
	//cube.EnableCollision();
	//cube.SetBoundingBoxFromMesh(0);

	GameObject& cube2 = _gameObjects.emplace_back(GameObject());
	cube2.SetModel("Cube");
	cube2.SetMeshMaterial("White");
	cube2.SetScale(glm::vec3(0.4, 1.2, 1.8));
	cube2.SetPosition(-0.9, 0, -0.6);
	cube2.SetRotationY(0.2);
	cube2.SetName("Cube2");
	cube2.SetMaterialType(MaterialType::GLASS);

	/*GameObject& cube3 = _gameObjects.emplace_back(GameObject());
	cube3.SetModel("Cube");
	cube3.SetMeshMaterial("Red");
	cube3.SetScale(glm::vec3(0.5, 0.174, 0.5));
	cube3.SetPosition(0.2, 1.17, 0.2);
	cube3.SetRotationY(0.2);
	cube3.SetParentName("Cube2");
	cube3.SetName("Cube3");
	cube3.SetMaterialType(MaterialType::TRANSLUCENT);*/
	//cube2.EnableCollision();
	//cube2.SetBoundingBoxFromMesh(0);

	/*GameObject& wineglass = _gameObjects.emplace_back(GameObject());
	wineglass.SetModel("WineGlass");
	wineglass.SetRotationY(1.6f);
	wineglass.SetPosition(glm::vec3(1.26, 1.15f, -1.675f + 2.0f - 0.3f));
	wineglass.SetScale(0.04f);
	wineglass.SetMaterialType(MaterialType::GLASS);
	wineglass.SetMeshMaterial("Ceiling");

	GameObject& wineglass2 = _gameObjects.emplace_back(GameObject());
	wineglass2.SetModel("WineGlass");
	wineglass2.SetRotationY(1.6f);
	wineglass2.SetPosition(glm::vec3(1.30, 1.15f, -1.675f + 2.0f - 0.15f));
	wineglass2.SetScale(0.04f);
	wineglass2.SetMaterialType(MaterialType::GLASS);
	wineglass2.SetMeshMaterial("White");*/

	GameObject& lightSwitch2 = _gameObjects.emplace_back(GameObject());
	lightSwitch2.SetModel("LightSwitchOn");
	lightSwitch2.SetMeshMaterial("LightSwitch");
	lightSwitch2.SetName("LightswitchBathroom");
	lightSwitch2.SetRotationY(-NOOSE_HALF_PI);
	lightSwitch2.SetScale(1.05f);
	lightSwitch2.SetPosition(-0.95f, 1.1f, 1.9f);
	lightSwitch2.SetInteract(InteractType::CALLBACK_ONLY, "", Callbacks::TurnBathroomLightOff);

	
	/*GameObject& macbookClosed = _gameObjects.emplace_back(GameObject());
	macbookClosed.SetModel("MacbookClosed");
	macbookClosed.SetPosition(glm::vec3(0, 1, 0));
	macbookClosed.SetScale(0.01f);
	macbookClosed.SetMeshMaterial("Macbook");*/

	GameObject& macbookBody = _gameObjects.emplace_back(GameObject());
	macbookBody.SetModel("MacbookBody");
	macbookBody.SetName("MacbookBody");
	macbookBody.SetPosition(-2.45, 0.88, 1.17);
	macbookBody.SetScale(0.01f);
	macbookBody.SetRotationY(-1.42);
	macbookBody.SetMeshMaterial("Macbook");
	macbookBody.SetInteract(InteractType::QUESTION, "Use [g]WIFE'S MACBOOK[w]?", Callbacks::UseLaptop);

	GameObject& macbookScreen = _gameObjects.emplace_back(GameObject());
	macbookScreen.SetModel("MacbookScreen");
	macbookScreen.SetName("MacbookScreen");
	macbookScreen.SetParentName("MacbookBody");
	macbookScreen.SetRotationX(-NOOSE_PI / 2);
	macbookScreen.SetOpenState(OpenState::CLOSED, 12.0f, -(NOOSE_PI / 2), 0.43f);
	macbookScreen.SetAudioOnOpen("LaptopOpen.wav", 0.25f);
	macbookScreen.SetAudioOnClose("LaptopClose.wav", 0.25f);
	macbookScreen.SetOpenAxis(OpenAxis::ROTATION_POS_X);
	macbookScreen.SetMeshMaterial("Macbook");

	GameObject& macbookScreenDisplay = _gameObjects.emplace_back(GameObject());
	macbookScreenDisplay.SetModel("MacbookScreenDisplay");
	macbookScreenDisplay.SetName("MacbookScreenDisplay");
	macbookScreenDisplay.SetParentName("MacbookScreen");
	macbookScreenDisplay.SetMeshMaterial("LaptopDisplay");
	macbookScreenDisplay.SetInteractToAffectAnotherObject("MacbookScreen");
	macbookScreenDisplay.SetMaterialType(MaterialType::LAPTOP_DISPLAY);

	GameObject& lampHers = _gameObjects.emplace_back(GameObject());
	lampHers.SetModel("Lamp");
	lampHers.SetMeshMaterial("Lamp");
	lampHers.SetPosition(-2.55f, 0.88, 1.43);
	lampHers.SetRotationY(NOOSE_HALF_PI);
	lampHers.SetScale(1.0);

	GameObject& lampHis = _gameObjects.emplace_back(GameObject());
	lampHis.SetModel("Lamp");
	lampHis.SetMeshMaterial("Lamp");
	lampHis.SetPosition(-2.55f, 0.88, -1.43);
	lampHis.SetRotationY(NOOSE_HALF_PI);
	lampHis.SetScale(1.0);

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

	{
		float h = -1.2;
		Wall& wallA = _inventoryWalls.emplace_back(glm::vec3(+1.1, h, +1.1), glm::vec3(-1.1, h, +1.1), "WallPaper");
		Wall& wallB = _inventoryWalls.emplace_back(glm::vec3(-1.1, h, -1.1), glm::vec3(+1.1, h, -1.1), "WallPaper");
		Wall& wallC = _inventoryWalls.emplace_back(glm::vec3(+1.1, h, -1.1), glm::vec3(+1.1, h, +1.1), "WallPaper");
		Wall& wallD = _inventoryWalls.emplace_back(glm::vec3(-1.1, h, +1.1), glm::vec3(-1.1, h, -1.1), "WallPaper");
	}

	static bool runOnce = true;
	if (runOnce) {
		Wall& wallA = _walls.emplace_back(glm::vec3(roomXmax, 0, roomZmax), glm::vec3(bathroomDoorX + doorWidth / 2, 0, roomZmax), "WallPaper");
		wallA._material = AssetManager::GetMaterial("Green");
		Wall& wallH = _walls.emplace_back(glm::vec3(bathroomDoorX - doorWidth / 2, 0, roomZmax), glm::vec3(roomXmin, 0, roomZmax), "WallPaper");
		wallH._material = AssetManager::GetMaterial("Green");
		Wall& wallAboveBathroomDoor = _walls.emplace_back(glm::vec3(bathroomDoorX + doorWidth / 2, 2, roomZmax), glm::vec3(bathroomDoorX - doorWidth / 2, 2, roomZmax), "WallPaper");
		wallAboveBathroomDoor._material = AssetManager::GetMaterial("Green");
		Wall& wallB = _walls.emplace_back(glm::vec3(roomXmin, 0, roomZmin), glm::vec3(hallDoorX - doorWidth / 2, 0, -roomZmax), "WallPaper");
		wallB._material = AssetManager::GetMaterial("Red");
		Wall& wallB2 = _walls.emplace_back(glm::vec3(hallDoorX + doorWidth / 2, 0, roomZmin), glm::vec3(roomXmax, 0, roomZmin), "WallPaper");
		wallB2._material = AssetManager::GetMaterial("Red");
		Wall& wallBC = _walls.emplace_back(glm::vec3(hallDoorX - doorWidth / 2, 2, -roomZmax), glm::vec3(hallDoorX + doorWidth / 2, 2, roomZmin), "WallPaper");
		wallBC._material = AssetManager::GetMaterial("Red");
		Wall& wallC = _walls.emplace_back(glm::vec3(roomXmax, 0, roomZmin), glm::vec3(roomXmax, 0, roomZmax), "WallPaper");
		wallC._material = AssetManager::GetMaterial("White");
		Wall& wallD = _walls.emplace_back(glm::vec3(roomXmin, 0, roomZmax), glm::vec3(roomXmin, 0, roomZmin), "WallPaper");
		wallD._material = AssetManager::GetMaterial("White");




		//wallAboveBathroomDoor._material = AssetManager::GetMaterial("Green");

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
		trim.SetModel("TrimCeiling");
		trim.SetMeshMaterial("Trims");
	}
}

void Scene::UpdateInventoryScene(float deltaTime) {

	static int _selectedInventoryItemIndex = 0;
	static float camXOffset = 0.0f;
	static float zoomValue = 0.0f;
	static float zoomTarget = 0.0f;
	int _renderWidth = 512;
	int _renderHeight = 288;
	
	// Load inventory data if needed
	if (GameData::_inventoryItemDataContainer.size() == 0) {
		GameData::InitInventoryItemData();
	}

	//Pathtrace world
	_inventoryGameObjects.clear();
	GameObject& floor = _inventoryGameObjects.emplace_back(GameObject());
	floor.SetModel("floor");
	floor.SetPositionY(-1.2);
	floor.SetMeshMaterial("FloorBoards");
	GameObject& ceiling = _inventoryGameObjects.emplace_back(GameObject());
	ceiling.SetModel("ceiling");
	ceiling.SetPositionY(-1.2);
	ceiling.SetMeshMaterial("Ceiling");

	// Prevent out of range
	_selectedInventoryItemIndex = std::min(_selectedInventoryItemIndex, GameData::GetInventorySize() - 1);
	_selectedInventoryItemIndex = std::max(_selectedInventoryItemIndex, 0);

	// If inventory is closed, snap the camera to the next item, so that nothing is scrolling when you re-open the inventory
	if (!GameData::inventoryOpen) {
		camXOffset = (float)_selectedInventoryItemIndex;
		zoomTarget = 0.0f;
		zoomValue = 0.0f;
		return;
	}

	// Lerp the zooming and scrolling values
	zoomValue = Util::FInterpTo(zoomValue, zoomTarget, deltaTime, 40);
	camXOffset = Util::FInterpTo(camXOffset, _selectedInventoryItemIndex, deltaTime, 20.0f);
		
	// No items?
	if (GameData::GetInventorySize() == 0) {
		TextBlitter::BlitAtPosition("NO ITEMS", _renderWidth / 2, _renderHeight / 2 - 9, true);
		return;
	}

	Transform translateYPos;
	translateYPos.position.y = 0.05f;
	translateYPos.position.z = zoomValue;
	
	////////////
	// SCROLL //

	if (GameData::_inventoryViewMode == InventoryViewMode::SCROLL) {
		
		zoomTarget = 0;

		// Toggle left and right, main view
		if (GameData::inventoryOpen) {
			if (Input::KeyPressed(HELL_KEY_D) && _selectedInventoryItemIndex < GameData::GetInventorySize() - 1) {
				_selectedInventoryItemIndex++;
				Audio::PlayAudio("RE_type.wav", 1.0f);
			}
			if (Input::KeyPressed(HELL_KEY_A) && _selectedInventoryItemIndex > 0) {
				_selectedInventoryItemIndex--;
				Audio::PlayAudio("RE_type.wav", 1.0f);
			}
		}

		std::string selectedItemName = GameData::GetInventoryItemNameByIndex(_selectedInventoryItemIndex);
		InventoryItemData* selectedItemData = GameData::GetInventoryItemDataByName(selectedItemName);

		// Menu (up and down)
		if (selectedItemData) {
			GameData::_selectedInventoryMenuOption = std::min(GameData::_selectedInventoryMenuOption, (int)selectedItemData->menu.size() - 1);

			if (selectedItemData) {
				if (Input::KeyPressed(HELL_KEY_S) && GameData::_selectedInventoryMenuOption < selectedItemData->menu.size() - 1) {
					Audio::PlayAudio("RE_type.wav", 1.0f);
					GameData::_selectedInventoryMenuOption += 1;
				}
				if (Input::KeyPressed(HELL_KEY_W) && GameData::_selectedInventoryMenuOption > 0) {
					Audio::PlayAudio("RE_type.wav", 1.0f);
					GameData::_selectedInventoryMenuOption -= 1;
				}
			}
		}

		// Build the game objects vector
		for (int i = 0; i < GameData::GetInventorySize(); i++) {
			for (int j = 0; j < GameData::_inventoryItemDataContainer.size(); j++) {
				if (GameData::GetInventoryItemNameByIndex(i) == GameData::_inventoryItemDataContainer[j].name) {
					auto itemData = GameData::_inventoryItemDataContainer[j];
					GameObject& item = _inventoryGameObjects.emplace_back(GameObject());
					item._model = itemData.model;

					Transform trans = itemData.transform;
					trans.position.x += i - camXOffset;
					trans.position.y += 0.0;

					//item.SetModelMatrix(translateYPos.to_mat4() * GameData::_inventoryExamineMatrix * trans.to_mat4());
					item.SetModelMatrixTransformOverride(translateYPos.to_mat4() * trans.to_mat4());

					item._meshMaterialIndices.resize(item._model->_meshIndices.size());
					item._meshMaterialTypes.resize(item._model->_meshIndices.size());
					item.SetMeshMaterial(itemData.material->_name.c_str());
				}
			}
		}

		// Text
		int gap = 4;
		int yBegin = 57;
		int lineHeight = 15;

		std::string arrowLeft = "<";
		std::string arrowRight = ">";

		if (_selectedInventoryItemIndex == 0)
			arrowLeft = " ";
		if (_selectedInventoryItemIndex == GameData::GetInventorySize() - 1)
			arrowRight = " ";

		std::string item = arrowLeft + "  [g]" + selectedItemName + "[w]  " + arrowRight;

		TextBlitter::BlitAtPosition(item, _renderWidth / 2, yBegin, true);

		if (selectedItemData) {
			for (int i = 0; i < selectedItemData->menu.size(); i++) {
				if (GameData::_selectedInventoryMenuOption == i)
					TextBlitter::BlitAtPosition(">" + selectedItemData->menu[i] + " ", _renderWidth / 2, yBegin - lineHeight - (lineHeight * i) - gap, true);
				else
					TextBlitter::BlitAtPosition(" " + selectedItemData->menu[i] + " ", _renderWidth / 2, yBegin - lineHeight - (lineHeight * i) - gap, true);
			}
		}

		if (Input::KeyPressed(HELL_KEY_E) && GameData::_selectedInventoryMenuOption == 1) {
			GameData::_inventoryViewMode = InventoryViewMode::EXAMINE;
			Audio::PlayAudio("OpenMenu.wav", 0.5f);
			zoomTarget = 0.25;
		}
	}

	/////////////
	// EXAMINE //

	else if (GameData::_inventoryViewMode == InventoryViewMode::EXAMINE) {

		if (Input::MouseWheelUp()) {
			zoomTarget += 0.1f;
		}
		if (Input::MouseWheelDown()) {
			zoomTarget -= 0.1f;
		}

		zoomTarget = std::min(0.40f, zoomTarget);
		zoomTarget = std::max(0.25f, zoomTarget);

		// This CAN happen, if the user resets the scene while in this menu
		if (GameData::GetInventorySize() == 0)
			return;

		InventoryItemData* inventoryItemData = GameData::GetInventoryItemDataByName(GameData::GetInventoryItemNameByIndex(_selectedInventoryItemIndex));

		if (inventoryItemData) {

			if (Input::LeftMouseDown()) {
				float mouseSensitivity = 0.005;
				Transform transform;
				transform.rotation.y = Input::GetMouseOffsetX() * mouseSensitivity;
				transform.rotation.x = Input::GetMouseOffsetY() * mouseSensitivity;
				GameData::_inventoryExamineMatrix = transform.to_mat4() * GameData::_inventoryExamineMatrix;
			}

			// Build the game objects vector						
			GameObject& item = _inventoryGameObjects.emplace_back(GameObject());
			item._model = inventoryItemData->model;

			Transform trans = inventoryItemData->transform;
			trans.position.x += _selectedInventoryItemIndex - camXOffset;
			trans.position.y +=  0;
			item.SetModelMatrixTransformOverride(translateYPos.to_mat4() * GameData::_inventoryExamineMatrix * trans.to_mat4());

			item._meshMaterialIndices.resize(item._model->_meshIndices.size());
			item._meshMaterialTypes.resize(item._model->_meshIndices.size());
			item.SetMeshMaterial(inventoryItemData->material->_name.c_str());

			
			//}
			//_inventoryItemShader.use();
			//_inventoryItemShader.setMat4("model", GameData::_inventoryExamineMatrix * inventoryItemData->transform.to_mat4());
			//inventoryItemData->material->Bind();
			//inventoryItemData->model->Draw();

			std::string text = "ROTATE: L Mouse    ZOOM: Wheel    BACK: R Mouse";
			TextBlitter::BlitAtPosition(text, _renderWidth / 2, 20, true);
		}

		// Leave
		if (Input::RightMousePressed()) {
			GameData::_inventoryViewMode = InventoryViewMode::SCROLL;
			GameData::_inventoryExamineMatrix = glm::mat4(1);
			GameData::_selectedInventoryMenuOption = 0;
			Audio::PlayAudio("OpenMenu.wav", 0.5f);
		}
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

std::vector<LightRenderInfo> Scene::GetLightRenderInfo()
{
	std::vector<LightRenderInfo> data;
	for (int i = 0; i < _lights.size(); i++) {
		data.push_back(LightRenderInfo());
		data[i].position = glm::vec4(_lights[i].position, 0);
		data[i].color = glm::vec4(_lights[i].color * _lights[i].currentBrightness, 0);
	}
	return data;
}

std::vector<LightRenderInfo> Scene::GetLightRenderInfoInventory()
{
	std::vector<LightRenderInfo> data;

	LightRenderInfo light0;
	light0.position = glm::vec4(0.5, 0.2, 1, 0);
	light0.color = glm::vec4(1, 0.95, 0.8, 0);
	data.push_back(light0);

	LightRenderInfo light1;
	light1.position = glm::vec4(-0.5, 0.2, 1, 0);
	light1.color = glm::vec4(1, 0.95, 0.8, 0);
	data.push_back(light1);

	return data;
}

int Scene::GetGameObjectCount() {
	return _gameObjects.size();
}


void Light::Update(float delatTime) {

	if (state == Light::State::OFF) {
		currentBrightness = 0;
	}
	else {
		currentBrightness += delatTime * 4;
		currentBrightness = std::min(1.0f, currentBrightness);
	}
}

void Scene::Update(float deltaTime) {

	Camera& camera = GameData::GetPlayer().m_camera;
	Player& player = GameData::GetPlayer();

	static bool exittingLaptop = false;
	static float exitTimer = 0;

	static bool wifeNew = true;

	if (Input::KeyPressed(HELL_KEY_L)) {
		wifeNew = !wifeNew;

		if (wifeNew)
			Scene::GetGameObjectByName("Wife")->SetModel("wife");
		else
			Scene::GetGameObjectByName("Wife")->SetModel("WifeOld");
	}

	if (camera._state == Camera::State::USING_LAPTOP) {

		//TextBlitter::BlitAtPosition("ESC: Exit", 20, 16, false);
		TextBlitter::BlitAtPosition("ESC: Exit", 512 - 85, 16, false);
		//TextBlitter::BlitAtPosition(TextBlitter::_debugTextToBilt, 16, 288 - 16, false);
		//TextBlitter::BlitAtPosition("shit", 16, 288 - 16, false);
		//TextBlitter::BlitAtPosition("fuck\nfuck\nfuck\nfuck\nfuck\n", 16, 196, false);

		// LEave laptop
		if (Input::KeyPressed(HELL_KEY_ESCAPE)) {
			camera._state = Camera::State::PLAYER_CONROL;
			player._viewHeight = GameData::GetPlayer().m_camera.m_transform.position.y;
			Audio::PlayAudio("RE_type.wav");
			exittingLaptop = true;
			exitTimer = 0.1f;
			player.m_position = glm::vec3(-2.05f, 1.65f, 1.11f);
		}
	}

	if (exittingLaptop) {
		camera.m_transform.rotation.x = Util::FInterpTo(camera.m_transform.rotation.x, -1.0f, deltaTime, 30);
		exitTimer -= deltaTime;
		if (exitTimer <= 0)
			exittingLaptop = false;
	}



	static float rotation = 0;
	static float rotation2 = 0;
	//rotation += deltaTime;
	//rotation = 0.3f;


	/*
	Transform t;
	t.position = camera->m_viewPos + (camera->m_front * glm::vec3(z));
	t.position.y += y;
	//Scene::GetGameObjectByName("MacbookBody")->SetModelMatrixTransformOverride()

	*/
	//glm::vec3 pos = GameData::GetCameraPosition();

	//pos += camera->m_front * glm::vec3(z);

	static float x = 0;
	static float y = 0;
	static float z = 0;

	if (Input::KeyPressed(HELL_KEY_UP)) {
		z += 0.01f;
	}
	if (Input::KeyPressed(HELL_KEY_DOWN)) {
		z -= 0.01f;
	}
	if (Input::KeyPressed(HELL_KEY_LEFT)) {
		y += 0.01f;
	}
	if (Input::KeyPressed(HELL_KEY_RIGHT)) {
		y -= 0.01f;
	}
	if (Input::KeyPressed(HELL_KEY_K)) {
		x += 0.01f;
	}
	if (Input::KeyPressed(HELL_KEY_L)) {
		x -= 0.01f;
	}
	if (Input::KeyPressed(HELL_KEY_O)) {
		rotation += 0.01f;
	}
	if (Input::KeyPressed(HELL_KEY_P)) {
		rotation -= 0.01f;
	}
	if (Input::KeyPressed(HELL_KEY_8)) {
		rotation2 += 0.01f;
	}
	if (Input::KeyPressed(HELL_KEY_9)) {
		rotation2 -= 0.01f;
	}

	glm::vec3 pos = glm::vec3(x, y, z);

	/*std::cout << "Position:  " << Util::Vec3ToString(pos) << "\n";
	std::cout << "Rotation:  " << rotation << "\n";
	std::cout << "Rotation:  " << rotation2 << "\n";


	Scene::GetGameObjectByName("MacbookScreen")->SetRotationX(rotation);
	Scene::GetGameObjectByName("MacbookBody")->SetRotationY(rotation2);

	Scene::GetGameObjectByName("MacbookBody")->SetPosition(pos);*/
	
	if (Input::KeyPressed(HELL_KEY_TAB) && !TextBlitter::QuestionIsOpen() && GameData::GetPlayer().m_camera._state != Camera::State::USING_LAPTOP) {
		Audio::PlayAudio("OpenMenu.wav", 0.5f);
		GameData::_inventoryViewMode = InventoryViewMode::SCROLL;
		GameData::inventoryOpen = !GameData::inventoryOpen;
		GameData::_selectedInventoryMenuOption = 0;
		GameData::_inventoryExamineMatrix = glm::mat4(1);
	}

	/*if (Input::KeyPressed(HELL_KEY_K)) {
		auto itemData = GameData::_inventoryItemDataContainer[1];
		GameObject& skull2 = _gameObjects.emplace_back(GameObject());
		skull2.SetModel("skull2");
		skull2.SetMeshMaterial("BlackSkull");
		skull2.SetTransform(itemData.transform);
	}*/

	// Player pressed E to interact?
	if (!GameData::inventoryOpen && _hoveredGameObject && Input::KeyPressed(HELL_KEY_E) && !GameData::GetPlayer().m_interactDisabled) {

		// Interact with the object
		if (_hoveredGameObject->IsInteractable()) {
			_hoveredGameObject->Interact(); 
		}
		// and also call interact on its partner if it has one
		if (_hoveredGameObject->_interactAffectsThisObjectInstead != "") {
			GetGameObjectByName(_hoveredGameObject->_interactAffectsThisObjectInstead)->Interact();
		}
	}

	static bool firstRun = true;

	if (firstRun) {
		_ropeAudioHandle = Audio::LoopAudio("Rope.wav", 0.5);
		firstRun = false;
	}

	// Sway wife
	static float sway = 0;
	float offset = 0;
	static bool forceDisable = false;
	if (Input::KeyPressed(HELL_KEY_P)) {
		forceDisable = !forceDisable;
	}
	if (!forceDisable) {
		sway += (3.89845f * deltaTime);
		offset = sin(sway) * 0.03f;
	}
	GameObject* wife = GetGameObjectByName("Wife");
	if (wife) {
		wife->SetRotationX(0.075f - offset * 0.045f);
		wife->SetRotationY(-1.75f + offset);
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

	for (Light& light : _lights) {
		light.Update(deltaTime);
	}

	UpdateInventoryScene(deltaTime);
}

std::vector<GameObject>& Scene::GetGameObjects() {
	return _gameObjects;
}

std::vector<MeshInstance> Scene::GetSceneMeshInstances(bool debugScene)
{
	std::vector<MeshInstance> instances;

	for (GameObject& gameObject : _gameObjects) {
		for (int i = 0; i < gameObject._model->_meshIndices.size(); i++) {
			int meshIndex = gameObject._model->_meshIndices[i];
			Mesh* mesh = AssetManager::GetMesh(meshIndex);
			MeshInstance instance;
			instance.worldMatrix = gameObject.GetModelMatrix();
			instance.basecolorIndex = gameObject.GetMaterial(i)->_basecolor;
			instance.normalIndex = gameObject.GetMaterial(i)->_normal;
			instance.rmaIndex = gameObject.GetMaterial(i)->_rma;
			instance.vertexOffset = mesh->_vertexOffset;
			instance.indexOffset = mesh->_indexOffset;
			instance.materialType = (int)gameObject._meshMaterialTypes[i];
			instances.push_back(instance);
		}
	}
	for (Wall& wall : _walls) {
		Mesh* mesh = AssetManager::GetMesh(wall._meshIndex);
		Material* material = wall._material;
		if (!debugScene && material != AssetManager::GetMaterial("BathroomWall")) {
			material = AssetManager::GetMaterial("WallPaper");
		}
		MeshInstance instance;
		instance.worldMatrix = glm::mat4(1);
		instance.basecolorIndex = material->_basecolor;
		instance.normalIndex = material->_normal;
		instance.rmaIndex = material->_rma;
		instance.vertexOffset = mesh->_vertexOffset;
		instance.indexOffset = mesh->_indexOffset;
		instance.materialType = (int)MaterialType::DEFAULT;
		instances.push_back(instance);
	}
	return instances;
}


std::vector<MeshInstance> Scene::GetInventoryMeshInstances(bool debugScene)
{
	std::vector<MeshInstance> instances;

	for (GameObject& gameObject : _inventoryGameObjects) {
		for (int i = 0; i < gameObject._model->_meshIndices.size(); i++) {
			int meshIndex = gameObject._model->_meshIndices[i];
			Mesh* mesh = AssetManager::GetMesh(meshIndex);
			MeshInstance instance;
			instance.worldMatrix = gameObject.GetModelMatrix();
			instance.basecolorIndex = gameObject.GetMaterial(i)->_basecolor;
			instance.normalIndex = gameObject.GetMaterial(i)->_normal;
			instance.rmaIndex = gameObject.GetMaterial(i)->_rma;
			instance.vertexOffset = mesh->_vertexOffset;
			instance.indexOffset = mesh->_indexOffset;
			instance.materialType = (int)gameObject._meshMaterialTypes[i];
			instances.push_back(instance);
		}
	}
	for (Wall& wall : _inventoryWalls) {
		Mesh* mesh = AssetManager::GetMesh(wall._meshIndex);
		Material* material = wall._material;
		if (!debugScene && material != AssetManager::GetMaterial("BathroomWall")) {
			material = AssetManager::GetMaterial("WallPaper");
		}
		MeshInstance instance;
		instance.worldMatrix = glm::mat4(1);
		instance.basecolorIndex = material->_basecolor;
		instance.normalIndex = material->_normal;
		instance.rmaIndex = material->_rma;
		instance.vertexOffset = mesh->_vertexOffset;
		instance.indexOffset = mesh->_indexOffset;
		instance.materialType = (int)MaterialType::DEFAULT;
		instances.push_back(instance);
	}

	return instances;
}

std::vector<VkAccelerationStructureInstanceKHR> Scene::GetMeshInstancesForInventoryAccelerationStructure()
{
	int instanceCustomIndex = 0;
	std::vector<VkAccelerationStructureInstanceKHR> instances;

	for (GameObject& gameObject : _inventoryGameObjects) {
		for (auto meshIndex : gameObject._model->_meshIndices) {
			Mesh* mesh = AssetManager::GetMesh(meshIndex);
			VkAccelerationStructureInstanceKHR& instance = instances.emplace_back(VkAccelerationStructureInstanceKHR());
			instance.transform = gameObject.GetVkTransformMatrixKHR();
			instance.instanceCustomIndex = instanceCustomIndex++;
			instance.mask = 0xFF;
			instance.instanceShaderBindingTableRecordOffset = 0;
			instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR;
			instance.accelerationStructureReference = mesh->_accelerationStructure.deviceAddress;
		}
	}
	for (Wall& wall : _inventoryWalls) {
		Mesh* mesh = AssetManager::GetMesh(wall._meshIndex);
		VkAccelerationStructureInstanceKHR& instance = instances.emplace_back(VkAccelerationStructureInstanceKHR());
		instance.transform = Util::GetIdentiyVkTransformMatrixKHR();
		instance.instanceCustomIndex = instanceCustomIndex++;
		instance.mask = 0xFF;
		instance.instanceShaderBindingTableRecordOffset = 0;
		instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR;
		instance.accelerationStructureReference = mesh->_accelerationStructure.deviceAddress;
	}
	return instances;
}

std::vector<VkAccelerationStructureInstanceKHR> Scene::GetMeshInstancesForSceneAccelerationStructure() {
	int instanceCustomIndex = 0;
	std::vector<VkAccelerationStructureInstanceKHR> instances;

	for (GameObject& gameObject : _gameObjects) {
		for (auto meshIndex : gameObject._model->_meshIndices) {
			Mesh* mesh = AssetManager::GetMesh(meshIndex);
			VkAccelerationStructureInstanceKHR& instance = instances.emplace_back(VkAccelerationStructureInstanceKHR());
			instance.transform = gameObject.GetVkTransformMatrixKHR();
			instance.instanceCustomIndex = instanceCustomIndex++;
			instance.mask = 0xFF;
			instance.instanceShaderBindingTableRecordOffset = 0;
			instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR;
			instance.accelerationStructureReference = mesh->_accelerationStructure.deviceAddress;
		}
	}
	for (Wall& wall : _walls) {
		Mesh* mesh = AssetManager::GetMesh(wall._meshIndex);
		VkAccelerationStructureInstanceKHR& instance = instances.emplace_back(VkAccelerationStructureInstanceKHR());
		instance.transform = Util::GetIdentiyVkTransformMatrixKHR();
		instance.instanceCustomIndex = instanceCustomIndex++;
		instance.mask = 0xFF;
		instance.instanceShaderBindingTableRecordOffset = 0;
		instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR;
		instance.accelerationStructureReference = mesh->_accelerationStructure.deviceAddress;
	}
	return instances;
}

std::vector<Mesh*> Scene::GetSceneMeshes(bool debugScene)
{
	std::vector<Mesh*> meshes;
	for (GameObject& gameObject : _gameObjects) {
		for (auto meshIndex : gameObject._model->_meshIndices) {
			meshes.push_back(AssetManager::GetMesh(meshIndex));
		}
	}
	for (Wall& wall : _walls) {
		meshes.push_back(AssetManager::GetMesh(wall._meshIndex));
	}
	return meshes;
}

void Scene::StoreMousePickResult(int instanceIndex, int primitiveIndex)
{
	if (GameData::inventoryOpen)
		return;

	_instanceIndex = instanceIndex;
	_primitiveIndex = primitiveIndex;
	_hitModelName = "undefined";
	_hitTriangleVertices.clear();
	_hoveredGameObject = nullptr;

	// First populate a vector with the info you need
	struct MeshHitInfo {
		Model* model = nullptr;
		Mesh* mesh = nullptr;
		void* parent = nullptr;
	};
	std::vector<MeshHitInfo> infos;

	for (GameObject& gameObject : _gameObjects) {
		for (auto meshIndex : gameObject._model->_meshIndices) {
			MeshHitInfo& info = infos.emplace_back(MeshHitInfo());
			info.model = gameObject._model;
			info.mesh = AssetManager::GetMesh(meshIndex);
			info.parent = &gameObject;
		}
	}
	for (Wall& wall : _walls) {
		MeshHitInfo& info = infos.emplace_back(MeshHitInfo());
		info.model = nullptr;
		info.mesh = AssetManager::GetMesh(wall._meshIndex);
		info.parent = &wall;
	}

	// Retrieve the hit
	if (instanceIndex < 0 || instanceIndex >= infos.size())
		return;

	MeshHitInfo hitInfo = infos[instanceIndex];

	if (!hitInfo.parent)
		return;
	
	// Find the vertices
	for (GameObject& gameObject : _gameObjects) {
		if (&gameObject == hitInfo.parent) {
			_hitModelName = gameObject._model->_filename;
			int indexOffset = hitInfo.mesh->_indexOffset;
			int vertexOffset = hitInfo.mesh->_vertexOffset;
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
		if (&wall == hitInfo.parent) {
			_hitModelName = "Wall";
			Mesh* mesh = AssetManager::GetMesh(wall._meshIndex);
			int indexOffset = hitInfo.mesh->_indexOffset;
			int vertexOffset = hitInfo.mesh->_vertexOffset;
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

void Scene::SetLightState(int index, Light::State state)
{
	if (index < 0 || index >= _lights.size())
		return;
	_lights[index].state = state;
}