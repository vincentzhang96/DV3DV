#pragma once
#include "fmod_studio.hpp"
#include "fmod_errors.h"

#define FMODLOGGER "fmod"
#define INIT_FMOD_LOGGER el::Logger* fmodLogger = el::Loggers::getLogger(FMODLOGGER);

class AudioManager
{
	FMOD::Studio::System* _system;
	bool _audioInitOk;
public:
	AudioManager();
	~AudioManager();

	void Init();

	void OnTick();

	static FMOD_RESULT F_CALLBACK FModCallback(FMOD_STUDIO_SYSTEM *system, FMOD_STUDIO_SYSTEM_CALLBACK_TYPE type, void *commanddata, void *userdata);
};

