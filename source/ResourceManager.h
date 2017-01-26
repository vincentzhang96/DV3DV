#pragma once
#include "ppac/PhoenixPAC.h"
#include "dn/pak/DnPak.h"

namespace resman
{
	enum ResourceRequestType;
	union ResourceRequestPayload;
	struct ResourceRequest;
	class ResourceManager;

	enum ResourceRequestType
	{
		REQ_NONE,
		REQ_TPUID,
		REQ_PAKPATH
	};

	typedef struct ResourceRequest
	{
		ResourceRequestType type;
		union
		{
			ppac::TPUID resTpuid;
			std::string resPakPath;
		};

		ResourceRequest();
		ResourceRequest(const ppac::TPUID& tpuid);
		ResourceRequest(const std::string& pakPath);
		~ResourceRequest();
	} ResourceRequest;

	class ResourceManager
	{
	private:
		ppac::cPPACManager _ppacManager;
		dn::cPakManager _dnPakManager;
		

	public:


		std::vector<uint8_t> GetResource(ResourceRequest request);
	};


}
