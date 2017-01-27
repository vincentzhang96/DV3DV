#include "stdafx.h"
#include "ResourceManager.h"

resman::ResourceRequest::ResourceRequest()
{
	type = REQ_NONE;
	resTpuid = 0;
}

resman::ResourceRequest::ResourceRequest(const ppac::TPUID& tpuid)
{
	type = REQ_TPUID;
	resTpuid = tpuid;
}

resman::ResourceRequest::ResourceRequest(const std::string& pakPath)
{
	type = REQ_PAKPATH;
	resPakPath = pakPath;
}

resman::ResourceRequest::~ResourceRequest()
{
	switch(type)
	{
	case REQ_TPUID:
	case REQ_NONE:
		(&resTpuid)->~TPUID();
		break;
	case REQ_PAKPATH:
		(&resPakPath)->std::string::~string();
		break;
	}
}

resman::ResourceResponse::ResourceResponse()
{
	_present = false;
}

resman::ResourceResponse::ResourceResponse(std::vector<uint8_t> vdata)
{
	_present = true;
	_data = vdata;
}

resman::ResourceManager::ResourceManager() : _ppacManager(), _dnPakManager()
{
}

resman::ResourceManager::~ResourceManager()
{
}

resman::ResourceResponse resman::ResourceManager::GetResource(ResourceRequest request)
{
	switch (request.type)
	{
	case REQ_PAKPATH:
	{
		auto ret = _dnPakManager.GetFileData(request.resPakPath);
		if (ret)
		{
			return ResourceResponse(ret->_data);
		}
		//	Return empty vector
		return ResourceResponse();
	}
	case REQ_TPUID:
	{
		auto ret = _ppacManager.GetFileData(request.resTpuid);
		if (ret)
		{
			return ResourceResponse(ret->_data);
		}
		//	Return empty vector
		return ResourceResponse();
	}
	case REQ_NONE:
	default:
		//	This is a programming error
		LOG(WARNING) << "Invalid request: Got request type " << request.type;
		throw "Invalid request";
	}
}


