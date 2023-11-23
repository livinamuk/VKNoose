#include "vk_engine.h"
#include <iostream>
#define NOMINMAX
#include "Windows.h"
#include "Game/AssetManager.h"
#include "Engine.h"
#include "Core/Lighting.h"

int main()
{
	for (int y = 0; y < PROPOGATION_HEIGHT; y++) {
		for (int z = 0; z < PROPOGATION_DEPTH; z++) {
			//std::cout << y << ", " << z << "\n";
		}
	}

	Engine::Init();
	Engine::Run();
	Engine::Cleanup();

	return 0;
}