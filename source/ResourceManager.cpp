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

resman::ResourceResponse resman::ResourceManager::GetResource(const ResourceRequest &request)
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


bool resman::operator==(const ResourceRequest& lhs, const ResourceRequest& rhs)
{
	if (lhs.type == rhs.type)
	{
		if (lhs.type == REQ_NONE)
		{
			return true;
		}
		if (lhs.type == REQ_PAKPATH)
		{
			return lhs.resPakPath == rhs.resPakPath;
		}
		if (lhs.type == REQ_TPUID)
		{
			return lhs.resTpuid == rhs.resTpuid;
		}
	}
	return false;
}

bool resman::operator!=(const ResourceRequest& lhs, const ResourceRequest& rhs)
{
	//	Checking for inequality is faster than checking for equality so we rewrite == for !=
	if (lhs.type == rhs.type)
	{
		if (lhs.type == REQ_NONE)
		{
			return false;
		}
		if (lhs.type == REQ_PAKPATH)
		{
			return lhs.resPakPath != rhs.resPakPath;
		}
		if (lhs.type == REQ_TPUID)
		{
			return lhs.resTpuid != rhs.resTpuid;
		}
		//	Unknown
		return false;
	}
	else
	{
		return true;
	}
}
