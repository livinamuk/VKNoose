#pragma once
#include "../Common.h"
#include "Player.h"

namespace GameData {	

	void Init();
	void Update();

	void AddInventoryItem(std::string name);
	void RemoveInventoryItem(int index);
	void RemoveInventoryItemByName(std::string name);
	void CleanInventory();
	int GetInventoryItemCount();
	std::string GetInventoryItemNameByIndex(int index, bool upperCase);
	Player& GetPlayer();
	bool PlayerHasItem(std::string itemName);
	int GetInventorySize();
	std::string  GetInventoryItemNameByIndex(int index);
	
	glm::mat4 GetProjectionMatrix();
	glm::mat4 GetViewMatrix();
	glm::vec3 GetCameraPosition();
	void InitInventoryItemData(); 

	float GetDeltaTime();

	extern float _cameraZoom;

	// Inventory
	inline glm::mat4 _inventoryProjectionMatrix = glm::mat4(1);
	inline float _inventoryZoom = 1.0f;
	inline InventoryViewMode _inventoryViewMode = InventoryViewMode::SCROLL;
	inline glm::mat4 _inventoryExamineMatrix = glm::mat4(1);
	inline bool inventoryOpen = false;
	inline std::vector<InventoryItemData> _inventoryItemDataContainer;
	inline int _selectedInventoryMenuOption = 0;

	inline InventoryItemData* GetInventoryItemDataByName(std::string name)
	{
		for (InventoryItemData& itemData : GameData::_inventoryItemDataContainer)
			if (name == itemData.name)
				return &itemData;

		std::cout << "GetInventoryItemDataByName() failed: " << name << " not found\n";
		return nullptr;
	}
};