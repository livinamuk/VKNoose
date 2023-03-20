#include "Scene.h"
#include "AssetManager.h"
#include "../Audio/Audio.h"

namespace Scene {
	std::vector<GameObject> _gameObjects;
}

AudioHandle _ropeAudioHandle;



void Scene::Init()
{
	GameObject& chair = _gameObjects.emplace_back(GameObject());
	chair.SetModel("chair");
	chair._worldTransform.rotation.y = 3.7f;
	chair.SetMeshMaterial(0, "FallenChair");

	GameObject& floor = _gameObjects.emplace_back(GameObject());
	floor.SetModel("floor");
	floor.SetMeshMaterial(0, "FloorBoards");

	GameObject& wife = _gameObjects.emplace_back(GameObject());
	wife.SetModel("wife");
	//wife._worldTransform.position.x = 0.8f - 0.6f;
	wife._worldTransform.position.x = 0.8f - 0.6f;
	//wife._worldTransform.position.y = 2.5f;
	wife._worldTransform.position.y = 2.7f;
	wife._worldTransform.position.z = 1.1f - 0.9f - 0.15;
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

	GameObject& skull = _gameObjects.emplace_back(GameObject());
	skull.SetModel("skull");
	skull._worldTransform.rotation.y = 3.7f;
	skull._worldTransform.position = glm::vec3(-6, 0, -6);
	skull.SetMeshMaterial(0, "BlackSkull");
	skull.SetMeshMaterial(1, "BlackSkull");
	skull._worldTransform.scale = glm::vec3(2);

	GameObject& door0 = _gameObjects.emplace_back(GameObject());
	door0.SetModel("door");
	door0.SetMeshMaterial(0, "Door");
	float d = 1.25f;
	door0._worldTransform.position = glm::vec3(-d, 0, -0.5);
	door0._worldTransform.rotation = glm::vec3(0, 3.14, 0);

	GameObject& door1 = _gameObjects.emplace_back(GameObject());
	door1.SetModel("door");
	door1.SetMeshMaterial(0, "Door");
	door1._worldTransform.position = glm::vec3(d, 0, -0.5);
	door1._worldTransform.rotation = glm::vec3(0, -3.14, 0);

	GameObject& door2 = _gameObjects.emplace_back(GameObject());
	door2.SetModel("door");
	door2.SetMeshMaterial(0, "Door");
	door2._worldTransform.position = glm::vec3(0.5, 0, -d);
	door2._worldTransform.rotation = glm::vec3(0, 3.14 * 0.5, 0);
	
	GameObject& door3 = _gameObjects.emplace_back(GameObject());
	door3.SetModel("door");
	door3.SetMeshMaterial(0, "Door");
	door3._worldTransform.position = glm::vec3(-0.5, 0, d);
	door3._worldTransform.rotation = glm::vec3(0, 3.14 * 1.5, 0);
	
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
	auto& wife = _gameObjects[2];
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