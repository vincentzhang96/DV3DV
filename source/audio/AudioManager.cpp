#include "../stdafx.h"
#include "AudioManager.h"

AudioManager::AudioManager()
{
	_system = nullptr;
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
		LOG(WARNING) << "Failed to create FMOD: Error " << result << " " << FMOD_ErrorString(result);
		_audioInitOk = false;
		return;
	}

	result = _system->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, 0);
	if (result != FMOD_OK)
	{
		LOG(WARNING) << "Failed to init FMOD: Error " << result << " " << FMOD_ErrorString(result);
		_audioInitOk = false;
		return;
	}
	
//	_system->setCallback(FModCallback);

	LOG(INFO) << "FMOD studio initialized";
	_audioInitOk = true;
}

void AudioManager::OnTick()
{
	if (_audioInitOk)
	{
		_system->update();
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
