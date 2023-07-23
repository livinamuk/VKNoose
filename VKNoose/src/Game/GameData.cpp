#include "GameData.h"
#include "../Util.h"
#include "AssetManager.h"

namespace GameData {
	std::vector<std::string> _inventory;
	Player _player; 
	float _cameraZoom = 1.0f;
}

void GameData::AddInventoryItem(std::string itemName) {
	_inventory.push_back(itemName);
}

void GameData::RemoveInventoryItem(int index) {
	if (index >= 0 && index < _inventory.size()) {
		_inventory.erase(_inventory.begin() + index);
	}
}

void GameData::CleanInventory() {
	_inventory.clear();
}

Player& GameData::GetPlayer() {
	return _player;
}

int GameData::GetInventoryItemCount() {
	return _inventory.size();
}

std::string GameData::GetInventoryItemNameByIndex(int index, bool upperCase) {
	if (index >= 0 && index < _inventory.size()) {
		if (upperCase) {
			return Util::UpperCase(_inventory[index]);
		}
		else {
			return _inventory[index];
		}
	}
	else return "";
}


bool GameData::PlayerHasItem(std::string itemName) {
	for (std::string& item : _inventory) {
		if (item == itemName)
			return true;
	}
	return false;
}


void GameData::RemoveInventoryItemByName(std::string name) {
	// Removes first matching occurance
	for (int i = 0; i < _inventory.size(); i++) {
		if (_inventory[i] == name) {
			_inventory.erase(_inventory.begin() + i);
			return;
		}
	}
}

glm::mat4 GameData::GetProjectionMatrix() {
	
	float zoom = _cameraZoom;

	/*if (_player.m_camera.test) {
		zoom = _player.m_camera.zoom;

		//_player.m_camera.m_transform.rotation.y = -1.42 + NOOSE_PI;

		glm::vec3 pos = { -2.45, 0.88, 1.17 };
		glm::vec3 rot = { 0,-1.42 ,0 };
		//MacbookBody
	}*/
		
	glm::mat4 projection = glm::perspective(zoom, 1700.f / 900.f, 0.01f, 100.0f);
	projection[1][1] *= -1;
	return projection;
}

glm::mat4 GameData::GetViewMatrix() {
	return _player.m_camera.GetViewMatrix();
}

glm::vec3 GameData::GetCameraPosition() {
	return _player.m_camera.m_viewPos;
}

void GameData::InitInventoryItemData()
{
	InventoryItemData phone;
	phone.name = "Water Damaged Phone";
	phone.menu.push_back("Use");
	phone.menu.push_back("Examine");
	//phone.menu.push_back("Discard");
	phone.material = AssetManager::GetMaterial("Phone");
	phone.model = AssetManager::GetModel("YourPhone");
	phone.transform.rotation.x = -NOOSE_HALF_PI;
	phone.transform.rotation.y = NOOSE_PI;
	phone.transform.scale = glm::vec3(3.1);
	phone.transform.rotation.x -= 0.1f;

	InventoryItemData diary;
	diary.name = "Wife's Diary";
	diary.menu.push_back("Read");
	diary.menu.push_back("Examine");
	diary.transform.scale = glm::vec3(1.9);
	diary.material = AssetManager::GetMaterial("Diary");
	diary.model = AssetManager::GetModel("Diary");
	diary.transform.rotation.x = 1.41f;
	diary.transform.rotation.y = -0.53f;
	diary.transform.rotation.x += 0.05f;
	_inventoryItemDataContainer.push_back(diary);
	_inventoryItemDataContainer.push_back(phone);

	InventoryItemData blackSkull;
	blackSkull.name = "Black Skull";
	blackSkull.menu.push_back("Use");
	blackSkull.menu.push_back("Examine");
	blackSkull.transform.scale = glm::vec3(0.25);
	blackSkull.material = AssetManager::GetMaterial("BlackSkull");
	blackSkull.model = AssetManager::GetModel("skull2");
	blackSkull.transform.rotation.x = 0;
	blackSkull.transform.rotation.y = 0;
	blackSkull.transform.rotation.x += 0.25f;
	_inventoryItemDataContainer.push_back(blackSkull);

	InventoryItemData flowers;
	flowers.name = "Flowers";
	flowers.menu.push_back("Use");
	flowers.menu.push_back("Examine");
	flowers.transform.scale = glm::vec3(1.7);
	flowers.material = AssetManager::GetMaterial("Flowers");
	flowers.model = AssetManager::GetModel("flowers");
	flowers.transform.rotation.x = 0.40;
	flowers.transform.rotation.y = 0.45;
	flowers.transform.rotation.x += 0.15f;
	_inventoryItemDataContainer.push_back(flowers);

	InventoryItemData smallKey;
	smallKey.name = "Small Key";
	smallKey.menu.push_back("Use");
	smallKey.menu.push_back("Examine");
	smallKey.transform.scale = glm::vec3(1.0);
	smallKey.material = AssetManager::GetMaterial("SmallKey");
	smallKey.model = AssetManager::GetModel("SmallKey");
	smallKey.transform.rotation.z = -0.64;
	smallKey.transform.rotation.y = 3.0;
	smallKey.transform.rotation.x += 0.15f;
	smallKey.transform.position.y -= 0.02f;
	_inventoryItemDataContainer.push_back(smallKey);
}


int GameData::GetInventorySize() {
	return (int)_inventory.size();
}

std::string GameData::GetInventoryItemNameByIndex(int index) {	
	if (index < 0 || index >= _inventory.size()) {
		return "";
	}
	return _inventory[index];
}