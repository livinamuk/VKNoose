#include "vk_engine.h"
#include <iostream>
#define NOMINMAX
#include "Windows.h"
#include "Game/AssetManager.h"
#include "BackEnd.h"

void LazyKeyPresses();

int main() {
    // Init
    Vulkan::InitMinimum();
    GameData::Init();

    // Main loop
    while (BackEnd::WindowHasNotBeenForceClosed()) {
        BackEnd::Update();

        LazyKeyPresses();

        if (!Vulkan::_loaded) {
            Vulkan::LoadNextItem();
            Vulkan::RenderLoadingFrame();
        }
        else {
            GameData::Update();
            Audio::Update();
            Vulkan::RenderGameFrame();
        }
    }

    // Cleanup
    Vulkan::Cleanup();
	return 0;
}





#include "Game/GameData.h"
#include "Game/Scene.h"
#include "Game/Laptop.h"
#include "UI/TextBlitter.h"

void LazyKeyPresses() {

    if (Input::KeyPressed(HELL_KEY_BACKSPACE)) {
        Vulkan::_forceCloseWindow = true;
    }
    if (Input::KeyPressed(HELL_KEY_C)) {
        GameData::GetPlayer().m_camera._disableHeadBob = !GameData::GetPlayer().m_camera._disableHeadBob;
        Audio::PlayAudio("RE_bleep.wav", 0.5f);
    }
    if (Input::KeyPressed(HELL_KEY_M)) {
        Vulkan::_collisionEnabled = !Vulkan::_collisionEnabled;
        Audio::PlayAudio("RE_bleep.wav", 0.5f);
    }
    if (Input::KeyPressed(HELL_KEY_H)) {
        Vulkan::hotload_shaders();
    }
    if (Input::KeyPressed(HELL_KEY_Y)) {
        Vulkan::_debugScene = !Vulkan::_debugScene;
        Audio::PlayAudio("RE_bleep.wav", 0.5f);
    }
    if (Input::KeyPressed(HELL_KEY_G)) {
        GameData::CleanInventory();
        for (auto& itemData : GameData::_inventoryItemDataContainer)
            GameData::AddInventoryItem(itemData.name);
        Audio::PlayAudio("RE_bleep.wav", 0.5f);
        TextBlitter::Type("GIVEN ALL ITEMS.");
    }
    if (Input::KeyPressed(HELL_KEY_U)) {
        Vulkan::_renderGBuffer = !Vulkan::_renderGBuffer;
        Audio::PlayAudio("RE_bleep.wav", 0.5f);
    }
    if (Input::KeyPressed(HELL_KEY_T)) {
        Vulkan::_usePathRayTracer = !Vulkan::_usePathRayTracer;
        Audio::PlayAudio("RE_bleep.wav", 0.5f);
    }

    if (Input::KeyPressed(HELL_KEY_B)) {
        Audio::PlayAudio("RE_bleep.wav", 0.5f);
        int debugModeIndex = (int)Vulkan::_debugMode;
        debugModeIndex++;
        if (debugModeIndex == (int)DebugMode::DEBUG_MODE_COUNT) {
            debugModeIndex = 0;
        }
        debugModeIndex;
        Vulkan::_debugMode = (DebugMode)debugModeIndex;
    }
    if (Input::KeyPressed(HELL_KEY_R)) {
        TextBlitter::ResetBlitter();
        Audio::PlayAudio("RE_bleep.wav", 0.9f);
        GameData::CleanInventory();
        Scene::Init();
        Laptop::Init();
    }
    if (Input::KeyPressed(HELL_KEY_F)) {
        Vulkan::ToggleFullscreen();
    }
}