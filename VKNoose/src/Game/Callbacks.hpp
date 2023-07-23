#pragma once
#include "../Common.h"
#include "GameObject.h"
#include "Scene.h"

namespace Callbacks {

	inline void TurnBathroomLightOff();
	inline void TurnBedroomLightOff();

	inline void UseLaptop() {

		Player& player = GameData::GetPlayer();
		Camera& camera = player.m_camera;
		camera._state = Camera::State::USING_LAPTOP;
		camera.ResetHeadBob();
		Scene::GetGameObjectByName("MacbookScreen")->SetOpenState(OpenState::OPENING, 12.0f, -(NOOSE_PI / 2), 0.43f);
	}

	inline void UnlockSmallDrawer() {
		Scene::GetGameObjectByName("LockedSmallDrawer")->SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
		Scene::GetGameObjectByName("LockedSmallDrawer")->SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
		Scene::GetGameObjectByName("LockedSmallDrawer")->SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
		Scene::GetGameObjectByName("LockedSmallDrawer")->SetInteract(InteractType::NONE, "", nullptr);
		Scene::GetGameObjectByName("LockedSmallDrawer")->SetOpenAxis(OpenAxis::TRANSLATE_Z);
		GameData::RemoveInventoryItemByName("Small Key");
		Audio::PlayAudio("Unlock1.wav");
	}

	inline void LockedDrawer() {
		if (GameData::PlayerHasItem("Small Key")) {
			TextBlitter::AskQuestion("Use the [g]SMALL KEY[w]?", Callbacks::UnlockSmallDrawer, nullptr);
		}
		else {
			TextBlitter::Type("It's locked.");
			Audio::PlayAudio("Locked1.wav", 0.25f);
		}
	}

	inline void TakeSmallKey() {
		Scene::GetGameObjectByName("Small Key")->PickUp();
		//Scene::GetGameObjectByName("LockedSmallDrawer")->SetInteract(InteractType::QUESTION, "Use the [g]SMALL KEY[w]?", Callbacks::UnlockSmallDrawer);
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

	inline void TurnBedroomLightOn() {
		Audio::PlayAudio("LightSwitchOn.wav");
		Scene::SetLightState(0, Light::State::ON);
		Scene::GetGameObjectByName("LightswitchBedroom")->_model = AssetManager::GetModel("LightSwitchOn");
		Scene::GetGameObjectByName("LightswitchBedroom")->SetInteract(InteractType::CALLBACK_ONLY, "", Callbacks::TurnBedroomLightOff);

	}
	inline void TurnBedroomLightOff() {
		Audio::PlayAudio("LightSwitchOff.wav");
		Scene::SetLightState(0, Light::State::OFF);
		Scene::GetGameObjectByName("LightswitchBedroom")->_model = AssetManager::GetModel("LightSwitchOff");
		Scene::GetGameObjectByName("LightswitchBedroom")->SetInteract(InteractType::CALLBACK_ONLY, "", Callbacks::TurnBedroomLightOn);
	}
	inline void TurnBathroomLightOn() {
		Audio::PlayAudio("LightSwitchOn.wav");
		Scene::SetLightState(1, Light::State::ON);
		Scene::GetGameObjectByName("LightswitchBathroom")->_model = AssetManager::GetModel("LightSwitchOn");
		Scene::GetGameObjectByName("LightswitchBathroom")->SetInteract(InteractType::CALLBACK_ONLY, "", Callbacks::TurnBathroomLightOff);

	}
	inline void TurnBathroomLightOff() {
		Audio::PlayAudio("LightSwitchOff.wav");
		Scene::SetLightState(1, Light::State::OFF);
		Scene::GetGameObjectByName("LightswitchBathroom")->_model = AssetManager::GetModel("LightSwitchOff");
		Scene::GetGameObjectByName("LightswitchBathroom")->SetInteract(InteractType::CALLBACK_ONLY, "", Callbacks::TurnBathroomLightOn);
	}
}