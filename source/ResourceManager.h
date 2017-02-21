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

	bool operator==(const ResourceRequest& lhs, const ResourceRequest& rhs);
	bool operator!=(const ResourceRequest& lhs, const ResourceRequest& rhs);
//	bool operator<(const ResourceRequest& lhs, const ResourceRequest& rhs);
//	bool operator>(const ResourceRequest& lhs, const ResourceRequest& rhs);
//	bool operator>=(const ResourceRequest& lhs, const ResourceRequest& rhs);
//	bool operator<=(const ResourceRequest& lhs, const ResourceRequest& rhs);

	struct RESOURCEREQUESTHASH
	{
		size_t operator()(const ResourceRequest& req) const
		{
			switch (req.type)
			{
			case REQ_PAKPATH:
				return std::hash<std::string>()(req.resPakPath);
			case REQ_TPUID:
				return ppac::PPACTPUIDHASH()(req.resTpuid);
			default:
				break;
			}
			return 0;
		}
	};

	typedef struct ResourceResponse
	{
		bool _present;
		std::vector<uint8_t> _data;

		ResourceResponse();
		explicit ResourceResponse(std::vector<uint8_t> vdata);

		operator bool() const
		{
			return _present;
		}

		std::vector<uint8_t>* operator ->()
		{
			return &_data;
		}
	} ResourceResponse;

	class ResourceManager
	{
	private:
	public:
		ppac::cPPACManager _ppacManager;
		dn::cPakManager _dnPakManager;

		ResourceManager();
		~ResourceManager();

		ResourceResponse GetResource(const ResourceRequest &request);
	};


}
