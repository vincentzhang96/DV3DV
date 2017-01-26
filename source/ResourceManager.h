#pragma once
#include "ppac/PhoenixPAC.h"

namespace resman
{
	enum ResourceRequestType;
	union ResourceRequestPayload;
	struct ResourceRequest;
	class ResourceManager;

	enum ResourceRequestType
	{
		REQ_TPUID,
		REQ_PAKPATH
	};

	union ResourceRequestPayload
	{
		ppac::TPUID tpuid;
		std::string pakPath;
	};

	typedef struct ResourceRequest
	{
		ResourceRequestType type;
		ResourceRequestPayload payload;

		ResourceRequest();
		ResourceRequest(ppac::TPUID tpuid);
		ResourceRequest(std::string pakPath);
	} ResourceRequest;

	class ResourceManager
	{
	private:
		ppac::cPPACManager _pacManager;
		

	public:


		std::vector<uint8_t> GetResource(ResourceRequest request);
	};


}
