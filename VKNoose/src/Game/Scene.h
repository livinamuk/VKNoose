#pragma once
#include "GameObject.h"

namespace Scene {
	void Init();
	void Update();
	std::vector<GameObject>& GetGameObjects();	
}
