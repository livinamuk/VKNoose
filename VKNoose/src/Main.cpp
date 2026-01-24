#include "API/Vulkan/vk_backend.h"
#include "BackEnd/BackEnd.h"
#include <iostream>
#define NOMINMAX
#include "Windows.h"
#include "AssetManagement/AssetManager.h"

void LazyKeyPresses();

int main() {
    // Init
    BackEnd::Init(WindowedMode::WINDOWED);
    AssetManager::Init();
    GameData::Init();

    // Main loop
    while (BackEnd::WindowHasNotBeenForceClosed()) {
        BackEnd::Update();

        LazyKeyPresses();

        if (!AssetManager::LoadingComplete()) {
            AssetManager::UpdateLoading();
            VulkanBackEnd::RenderLoadingFrame();
        }
        //else {
        //    continue;
        //}

        //if (!VulkanBackEnd::_loaded && !AssetManager::LoadingComplete()) {
        else if (!VulkanBackEnd::_loaded) {
            VulkanBackEnd::LoadNextItem();
            VulkanBackEnd::RenderLoadingFrame();
        }

        else {
            GameData::Update();
            Audio::Update();
            VulkanBackEnd::RenderGameFrame();
        }
    }

    // Cleanup
    VulkanBackEnd::Cleanup();
	return 0;
}



#include "Game/GameData.h"
#include "Game/Scene.h"
#include "Game/Laptop.h"
#include "UI/TextBlitter.h"

void LazyKeyPresses() {

    if (Input::KeyPressed(HELL_KEY_BACKSPACE)) {
        VulkanBackEnd::_forceCloseWindow = true;
    }
    if (Input::KeyPressed(HELL_KEY_C)) {
        GameData::GetPlayer().m_camera._disableHeadBob = !GameData::GetPlayer().m_camera._disableHeadBob;
        Audio::PlayAudio("RE_bleep.wav", 0.5f);
    }
    if (Input::KeyPressed(HELL_KEY_M)) {
        VulkanBackEnd::_collisionEnabled = !VulkanBackEnd::_collisionEnabled;
        Audio::PlayAudio("RE_bleep.wav", 0.5f);
    }
    if (Input::KeyPressed(HELL_KEY_H)) {
        VulkanBackEnd::hotload_shaders();
    }
    if (Input::KeyPressed(HELL_KEY_Y)) {
        VulkanBackEnd::_debugScene = !VulkanBackEnd::_debugScene;
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
        VulkanBackEnd::_renderGBuffer = !VulkanBackEnd::_renderGBuffer;
        Audio::PlayAudio("RE_bleep.wav", 0.5f);
    }
    if (Input::KeyPressed(HELL_KEY_T)) {
        VulkanBackEnd::_usePathRayTracer = !VulkanBackEnd::_usePathRayTracer;
        Audio::PlayAudio("RE_bleep.wav", 0.5f);
    }

    if (Input::KeyPressed(HELL_KEY_B)) {
        Audio::PlayAudio("RE_bleep.wav", 0.5f);
        int debugModeIndex = (int)VulkanBackEnd::_debugMode;
        debugModeIndex++;
        if (debugModeIndex == (int)DebugMode::DEBUG_MODE_COUNT) {
            debugModeIndex = 0;
        }
        debugModeIndex;
        VulkanBackEnd::_debugMode = (DebugMode)debugModeIndex;
    }
    if (Input::KeyPressed(HELL_KEY_R)) {
        TextBlitter::ResetBlitter();
        Audio::PlayAudio("RE_bleep.wav", 0.9f);
        GameData::CleanInventory();
        Scene::Init();
        Laptop::Init();
    }
    if (Input::KeyPressed(HELL_KEY_F)) {
        VulkanBackEnd::ToggleFullscreen();
    }
}