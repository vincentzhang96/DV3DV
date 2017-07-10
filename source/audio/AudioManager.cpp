#include "../stdafx.h"
#include "AudioManager.h"
#include <minwinbase.h>

AudioManager::AudioManager(resman::ResourceManager* resManager) : _sounds(4096)
{
	_resManager = resManager;
	_system = nullptr;
	_llSystem = nullptr;
	_audioInitOk = false;
}


AudioManager::~AudioManager()
{
	if (_audioInitOk)
	{
		_system->release();
	}
	CoUninitialize();
}

void AudioManager::Init()
{
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	FMOD_RESULT result;
	
	result = FMOD::Studio::System::create(&_system);
	if (result != FMOD_OK)
	{
		CLOG(WARNING, FMODLOGGER) << "Failed to create FMOD: Error " << result << " " << FMOD_ErrorString(result);
		_audioInitOk = false;
		return;
	}

	result = _system->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, 0);
	if (result != FMOD_OK)
	{
		CLOG(WARNING, FMODLOGGER) << "Failed to init FMOD: Error " << result << " " << FMOD_ErrorString(result);
		_audioInitOk = false;
		return;
	}
	
	_system->getLowLevelSystem(&_llSystem);

	_system->setCallback(FModCallback, 
		FMOD_SYSTEM_CALLBACK_MEMORYALLOCATIONFAILED |
		FMOD_SYSTEM_CALLBACK_ERROR);

	CLOG(INFO, FMODLOGGER) << "FMOD studio initialized";
	_audioInitOk = true;
}

void AudioManager::OnTick()
{
	if (_audioInitOk)
	{
		_system->update();
	}
}

SOUNDHANDLE AudioManager::Load(const resman::ResourceRequest& request)
{
	if (!_audioInitOk)
	{
		return INVALID_SOUNDHANDLE;
	}

	auto data = _resManager->GetResource(request);
	if (data)
	{
		FMOD::Sound* sound;
		FMOD_CREATESOUNDEXINFO soundInfo;
		ZeroMemory(&soundInfo, sizeof(FMOD_CREATESOUNDEXINFO));
		soundInfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
		soundInfo.length = data->size();

		//	Determine type if we can
		FMOD_SOUND_TYPE soundType = FMOD_SOUND_TYPE_UNKNOWN;
		if (request.type == resman::REQ_TPUID)
		{
			switch (request.resTpuid.t)
			{
			case 0x0300:
				soundType = FMOD_SOUND_TYPE_MPEG;
				break;
			case 0x0302:
				soundType = FMOD_SOUND_TYPE_OGGVORBIS;
				break;
			case 0x0303:
				soundType = FMOD_SOUND_TYPE_WAV;
				break;
			case 0x0304:
				soundType = FMOD_SOUND_TYPE_AUDIOQUEUE;
				break;
			}
		}
		else if (request.type == resman::REQ_PAKPATH)
		{
			std::string path = request.resPakPath;
			//	TODO
		}
		
		soundInfo.suggestedsoundtype = soundType;

		auto result = _llSystem->createSound(
			reinterpret_cast<const char*>(data->data()), 
			FMOD_CREATESAMPLE | FMOD_OPENMEMORY, 
			&soundInfo, 
			&sound);
		if (result != FMOD_OK)
		{
			CLOG(WARNING, FMODLOGGER) << "Failed to load sound: Error " << result << " " << FMOD_ErrorString(result);
			return INVALID_SOUNDHANDLE;
		}

		auto handle = _sounds.insert(sound);

		return handle;
	} 
	else
	{
		return INVALID_SOUNDHANDLE;
	}
}

void AudioManager::Play(const SOUNDHANDLE hSound)
{
	if (hSound == INVALID_SOUNDHANDLE || !_audioInitOk)
	{
		return;
	}

	if (!_sounds.contains(hSound))
	{
		CLOG(WARNING, FMODLOGGER) << "No sound with handle " << hSound;
		return;
	}
	auto sound = _sounds[hSound];
	auto result = _llSystem->playSound(sound, nullptr, false, nullptr);
	if (result != FMOD_OK)
	{
		CLOG(WARNING, FMODLOGGER) << "Failed to play sound: Error " << result << " " << FMOD_ErrorString(result);
		return;
	}
}

FMOD_RESULT AudioManager::FModCallback(FMOD_STUDIO_SYSTEM *system, FMOD_STUDIO_SYSTEM_CALLBACK_TYPE type, void *commanddata, void *userdata)
{
	auto sys = reinterpret_cast<FMOD::System *>(system);

	switch (type)
	{
	case FMOD_SYSTEM_CALLBACK_DEVICELISTCHANGED:
	{
		int numdrivers;
		sys->getNumDrivers(&numdrivers);
		CLOG(TRACE, FMODLOGGER) << "Device list changed, seeing " << numdrivers;
		break;
	}
	case FMOD_SYSTEM_CALLBACK_MEMORYALLOCATIONFAILED:
	{
		CLOG(WARNING, FMODLOGGER) << "Failed to allocate memory";
		break;
	}
	case FMOD_SYSTEM_CALLBACK_THREADCREATED:
	{
		CLOG(TRACE, FMODLOGGER) << "Thread created";
		break;
	}
	case FMOD_SYSTEM_CALLBACK_THREADDESTROYED:
	{
		CLOG(TRACE, FMODLOGGER) << "Thread destroyed";
		break;
	}
	case FMOD_SYSTEM_CALLBACK_BADDSPCONNECTION:
	{
		CLOG(WARNING, FMODLOGGER) << "Bad DSP connection";
		break;
	}
	case FMOD_SYSTEM_CALLBACK_PREMIX:
	case FMOD_SYSTEM_CALLBACK_MIDMIX:
	case FMOD_SYSTEM_CALLBACK_POSTMIX:
	{
		break;
	}
	case FMOD_SYSTEM_CALLBACK_ERROR:
	{
		auto info = static_cast<FMOD_ERRORCALLBACK_INFO*>(commanddata);
		CLOG(WARNING, FMODLOGGER) << "System callback error " << info->result << " from " << info->functionname << "(" << info->functionparams << " instance " << info->instance << " (type " << info->instancetype << ")";
		break;
	}
	default:
	{
		CLOG(WARNING, FMODLOGGER) << "Unknown callback type " << type;
	}
	}

	return FMOD_OK;
}
