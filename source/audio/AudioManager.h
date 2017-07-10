#pragma once
#include "fmod_studio.hpp"
#include "fmod_errors.h"
#include "../ResourceManager.h"

#define FMODLOGGER "fmod"
#define INIT_FMOD_LOGGER el::Logger* fmodLogger = el::Loggers::getLogger(FMODLOGGER);

typedef uint32_t SOUNDHANDLE;
#define INVALID_SOUNDHANDLE 0

class AudioManager
{
	bool _audioInitOk;
	packed_freelist<FMOD::Sound*> _sounds;
	resman::ResourceManager* _resManager;
public:
	FMOD::Studio::System* _system;
	FMOD::System* _llSystem;

	explicit AudioManager(resman::ResourceManager* resManager);
	~AudioManager();

	void Init();

	void OnTick();

	SOUNDHANDLE Load(const resman::ResourceRequest &request);

	void Play(const SOUNDHANDLE hSound);

	static FMOD_RESULT F_CALLBACK FModCallback(FMOD_STUDIO_SYSTEM *system, FMOD_STUDIO_SYSTEM_CALLBACK_TYPE type, void *commanddata, void *userdata);
};

