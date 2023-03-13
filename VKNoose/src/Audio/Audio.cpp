#include "Audio.h"
//#include "Helpers/Util.h"
#include <iostream>

std::unordered_map<std::string, FMOD::Sound*> Audio::s_loadedAudio;
//SoLoud::Soloud Audio::gSoloud;

FMOD::System* Audio::system = nullptr;
FMOD_RESULT Audio::result;

int Audio::s_currentChannel = 0;

bool succeededOrWarn(const std::string& message, FMOD_RESULT result)
{
	if (result != FMOD_OK) {
		std::cout << message << ": " << result << " " << FMOD_ErrorString(result) << std::endl;
		return false;
	}
	return true;
}

void Audio::Init()
{


	// Create the main system object.
	result = FMOD::System_Create(&system);
	if (!succeededOrWarn("FMOD: Failed to create system object", result))
		return ;

	// Initialize FMOD.
	result = system->init(AUDIO_CHANNEL_COUNT, FMOD_INIT_NORMAL, nullptr);
	if (!succeededOrWarn("FMOD: Failed to initialise system object", result))
		return ;

	// Create the channel group.
	FMOD::ChannelGroup* channelGroup = nullptr;
	result = system->createChannelGroup("inGameSoundEffects", &channelGroup);
	if (!succeededOrWarn("FMOD: Failed to create in-game sound effects channel group", result))
		return ;

	// Create the sound.
	//system->createSound("res/audio/Shotgun_Fire_01.wav", FMOD_DEFAULT, nullptr, &sound);

	// Play the sound.
	/*FMOD::Channel* channel = nullptr;
	result = system->playSound(sound, nullptr, false, &channel);
	if (!succeededOrWarn("FMOD: Failed to play sound", result))
		return ;


	 */

	Audio::LoadAudio("player_step_1.wav");
	Audio::LoadAudio("player_step_2.wav");
	Audio::LoadAudio("player_step_3.wav");
	Audio::LoadAudio("player_step_4.wav");

	Audio::LoadAudio("pl_dirt1.wav");
	Audio::LoadAudio("pl_dirt2.wav");
	Audio::LoadAudio("pl_dirt3.wav");
	Audio::LoadAudio("pl_dirt4.wav"); 

	Audio::LoadAudio("UI_Select.wav");
	//gSoloud.init();
	/*
	Audio::LoadAudio("UI_Select2.wav");
	Audio::LoadAudio("UI_Select3.wav");
	Audio::LoadAudio("UI_Select4.wav");
	Audio::LoadAudio("UI_Select5.wav");
	Audio::LoadAudio("DrawerOpen.wav");
	Audio::LoadAudio("DrawerClose.wav");
	Audio::LoadAudio("ItemPickUp.wav");
	Audio::LoadAudio("Rope.wav");
	Audio::LoadAudio("Music2.wav");
	Audio::LoadAudio("Door_Open2.wav");
	Audio::LoadAudio("CabinetOpen.wav");
	Audio::LoadAudio("CabinetClose.wav");
	Audio::LoadAudio("ToiletLidUp.wav");
	Audio::LoadAudio("ToiletLidDown.wav");
	Audio::LoadAudio("ToiletSeatUp.wav");
	Audio::LoadAudio("ToiletSeatDown.wav");
	Audio::LoadAudio("OpenMenu.wav");
	Audio::LoadAudio("MenuScroll.wav");
	Audio::LoadAudio("LightSwitchOn.wav");
	Audio::LoadAudio("LightSwitchOff.wav");
	Audio::LoadAudio("Unlock1.wav");
	Audio::LoadAudio("Vase.wav");*/
}

void Audio::Update()
{
	system->update();
}

void Audio::LoadAudio(const char* name)
{
	std::string fullpath = "res/audio/";
	fullpath += name;

	FMOD_MODE eMode = FMOD_DEFAULT;
	if (fullpath == "res/audio/Music.wav")
		eMode = FMOD_LOOP_NORMAL;

	// Create the sound.
	FMOD::Sound* sound = nullptr; 
	system->createSound(fullpath.c_str(), eMode, nullptr, &sound);

	// Map pointer to name
	s_loadedAudio[name] = sound;
}

FMOD::Sound* Audio::PlayAudio(const char* name, float volume)
{
	FMOD::Sound* sound = s_loadedAudio[name];
	FMOD::Channel* channel = nullptr;
	system->playSound(sound, nullptr, false, &channel);
	channel->setVolume(volume);
	return sound;
}

AudioHandle Audio::LoopAudio(const char* name, float volume)
{
	AudioHandle handle;
	handle.sound = s_loadedAudio[name];
	system->playSound(handle.sound, nullptr, false, &handle.channel);
	handle.channel->setVolume(volume);
	handle.channel->setMode(FMOD_LOOP_NORMAL);
	handle.sound->setMode(FMOD_LOOP_NORMAL);
	handle.sound->setLoopCount(-1);
	return handle;
}


void Audio::Terminate()
{
	//gSoloud.deinit();
}

void Audio::PlayGlockFlesh()
{
	int RandomAudio = rand() % 8;
	if (RandomAudio == 0)
		Audio::PlayAudio("FLY_Bullet_Impact_Flesh_01.wav");
	if (RandomAudio == 1)
		Audio::PlayAudio("FLY_Bullet_Impact_Flesh_02.wav");
	if (RandomAudio == 2)
		Audio::PlayAudio("FLY_Bullet_Impact_Flesh_03.wav");
	if (RandomAudio == 3)
		Audio::PlayAudio("FLY_Bullet_Impact_Flesh_04.wav");
	if (RandomAudio == 4)
		Audio::PlayAudio("FLY_Bullet_Impact_Flesh_05.wav");
	if (RandomAudio == 5)
		Audio::PlayAudio("FLY_Bullet_Impact_Flesh_06.wav");
	if (RandomAudio == 6)
		Audio::PlayAudio("FLY_Bullet_Impact_Flesh_07.wav");
	if (RandomAudio == 7)
		Audio::PlayAudio("FLY_Bullet_Impact_Flesh_08.wav");
}