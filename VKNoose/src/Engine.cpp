#include "Engine.h"
#include "vk_engine.h"
#include "Game/GameData.h"

void Engine::Init() {

	//GameData::GetPlayer().m_position = glm::vec3(-2.7f, 0, 0);
	//GameData::GetPlayer().m_camera.m_transform.rotation = glm::vec3(-0.25f, -NOOSE_HALF_PI, 0);
	//GameData::GetPlayer().m_position = glm::vec3(-1.82f, 0, -1.05f);
	//GameData::GetPlayer().m_camera.m_transform.rotation = glm::vec3(-1.12f, -0.66f, 0.0f);
	GameData::GetPlayer().m_position = glm::vec3(-1.87f, 0, 1.11f);
	GameData::GetPlayer().m_camera.m_transform.rotation = glm::vec3(-1.00f, 1.69f, 0.0f);
	
	Vulkan::Init();
}

void Engine::Run() {
	Vulkan::run();
}

void Engine::Cleanup() {
	Vulkan::cleanup();
}