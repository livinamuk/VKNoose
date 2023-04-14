#pragma once
#include <unordered_map>
#include "fmod.hpp"
#include <fmod_errors.h>

struct AudioEffectInfo {
	std::string filename = "";
	float volume = 0.0f;
};

/*struct AudioFile {
	const char* name;
	//SoLoud::Wav* audio; SoLoud::Soloud Audio::gSoloud;
	FMOD::Sound* sound = nullptr;
};*/

struct AudioHandle {
	FMOD::Sound* sound = nullptr;
	FMOD::Channel* channel = nullptr;
};

constexpr int AUDIO_CHANNEL_COUNT = 512;

class Audio
{
public: // functions
	static void Init();
	static void LoadAudio(const char* name); 
	static void Update();
	static FMOD::Sound* PlayAudio(const char* name, float volume = 1.0f);
	static FMOD::Sound* PlayAudio(AudioEffectInfo info);
	static AudioHandle LoopAudio(const char* name, float volume = 1.0f);

	//static FMOD::Sound* sound;// = nullptr;
	static FMOD::System* system;	
	static FMOD_RESULT result;

	static void Terminate();

	static void PlayGlockFlesh();

public: // variables
	//static SoLoud::Soloud gSoloud;
	static std::unordered_map<std::string, FMOD::Sound*> s_loadedAudio;
	static int s_currentChannel;
};
