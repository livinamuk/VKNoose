#include "GameData.h"
#include "../Util.h"

namespace GameData {
	std::vector<std::string> _inventory;
	Player _player;
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