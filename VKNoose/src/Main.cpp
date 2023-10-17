#include "vk_engine.h"
#include <iostream>
#define NOMINMAX
#include "Windows.h"
#include "Game/AssetManager.h"
#include "Engine.h"

int main()
{
	Engine::Init();
	Engine::Run();
	Engine::Cleanup();

	return 0;
}