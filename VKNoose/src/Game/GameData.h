#include "../Common.h"
#include "Player.h"

namespace GameData {	
	void AddInventoryItem(std::string name);
	void RemoveInventoryItem(int index);
	void RemoveInventoryItemByName(std::string name);
	void CleanInventory();
	int GetInventoryItemCount();
	std::string GetInventoryItemNameByIndex(int index, bool upperCase);
	Player& GetPlayer();
	bool PlayerHasItem(std::string itemName);
};