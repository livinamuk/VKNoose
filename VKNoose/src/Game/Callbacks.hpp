#pragma once
#include "../Common.h"
#include "GameObject.h"
#include "Scene.h"

namespace Callbacks {

	inline void ReturnFlowers() {
		Scene::GetGameObjectByName("Flowers")->ResetToInitialState();
		Scene::GetGameObjectByName("Vase")->SetQuestion("", nullptr);
	}

	inline void TakeFlowers() {
		Scene::GetGameObjectByName("Flowers")->PickUp();
		Scene::GetGameObjectByName("Vase")->SetQuestion("Return the [g]FLOWERS[w]?", Callbacks::ReturnFlowers);
	}
}