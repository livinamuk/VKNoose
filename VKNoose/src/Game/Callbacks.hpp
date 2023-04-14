#pragma once
#include "../Common.h"
#include "GameObject.h"
#include "Scene.h"

namespace Callbacks {

	inline void UnlockSmallDrawer() {
		Scene::GetGameObjectByName("LockedSmallDrawer")->SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
		Scene::GetGameObjectByName("LockedSmallDrawer")->SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
		Scene::GetGameObjectByName("LockedSmallDrawer")->SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
		Scene::GetGameObjectByName("LockedSmallDrawer")->SetInteract(InteractType::NONE, "", nullptr);
		Scene::GetGameObjectByName("LockedSmallDrawer")->SetOpenAxis(OpenAxis::TRANSLATE_Z);
		Audio::PlayAudio("Unlock1.wav");
	}

	inline void TakeSmallKey() {
		Scene::GetGameObjectByName("Small Key")->PickUp();
		Scene::GetGameObjectByName("LockedSmallDrawer")->SetInteract(InteractType::QUESTION, "Use the [g]SMALL KEY[w]?", Callbacks::UnlockSmallDrawer);
	}

	inline void ReturnFlowers() {
		GameData::RemoveInventoryItemByName("Flowers");
		Scene::GetGameObjectByName("Flowers")->SetCollectedState(false);
		Scene::GetGameObjectByName("Vase")->SetInteract(InteractType::NONE, "", nullptr);
		if (Scene::GetGameObjectByName("Small Key")->IsCollected()) {
			Scene::GetGameObjectByName("Flowers")->SetInteract(InteractType::TEXT, "Did she leave these here?", nullptr);
		}
	}

	inline void FlowersWereTaken() {
		Scene::GetGameObjectByName("Vase")->SetInteract(InteractType::QUESTION, "Return the [g]FLOWERS[w]?", Callbacks::ReturnFlowers);
		Scene::GetGameObjectByName("Small Key")->SetInteract(InteractType::QUESTION, "Take the [g]SMALL KEY[w]?", Callbacks::TakeSmallKey);
	}

	inline void OpenCabinet() {
		Scene::GetGameObjectByName("Cabinet Door")->Interact();
	}
}