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


