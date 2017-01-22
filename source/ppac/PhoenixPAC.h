#pragma once
#include "../stdafx.h"
#include <mutex>
#include <unordered_map>

namespace ppac
{
	typedef uint8_t uint8;
	typedef uint16_t uint16;
	typedef uint32_t uint32;
	typedef uint64_t uint64;
	typedef uint32_t GUARD;
#ifdef PPAC_OPT_LONG_OFFSET
	typedef uint64 OFFSET;
#else
	typedef uint32 OFFSET;
#endif

	typedef uint32 PPACHANDLE;
#define PPACHANDLE_INVALID 0

#define INIT_PPAC_LOGGER el::Logger* ppacLogger = el::Loggers::getLogger("PPAC");

#define PPAC_HEAD_MAGIC 0x50504143
#define PPAC_HEAD_MAGICSWP 0x43415050
#define PPAC_INDEX_GUARD 0x494E4458
#define PPAC_META_GUARD 0x4D455441
#define PPAC_TRASH_GUARD 0x54525348
#define PPAC_CHK_HEAD_MAGIC(head) (head.hMagic == PPAC_HEAD_MAGIC)
#define PPAC_CHK_INDEX_GUARD(index) (index.iGuard == PPAC_INDEX_GUARD)
#define PPAC_CHK_META_GUARD(meta) (meta.mGuard == PPAC_META_GUARD)
#define PPAC_CHK_TRASH_GUARD(trash) (trash.iGuard == PPAC_TRASH_GUARD)

#define PPACF_USE_LONG_OFFSETS 0x00000001
#define PPACF_JAVA_ARRAY_COMPAT 0x00000002
#define PPACF_CHK_FLAG(flags, flag) ((flags & flag) != 0)

//	PPAC TYPE IDS
//	BASIC
#define PPACT_UNKNOWN			0x0000
#define PPACT_BINARY			0x0001
//	TEXT
#define PPACT_TEXT_PLAIN		0x0100
//	IMAGE
#define PPACT_IMAGE_PNG			0x0200
#define PPACT_IMAGE_JPEG		0x0202
#define PPACT_IMAGE_DDS			0x0206
//	FONT
#define PPACT_FONT_TTF			0x0500
#define PPACT_FONT_OTF			0x0501
//	EXECUTABLE
#define PPACT_EXE_LUA_SCRIPT	0x0703
#define PPACT_EXE_LUA_BYTECODE	0x0704
//	APPLICATION SPECIFIC


#define PPAC_COMPRESSION_SUPPORTED(comp) (comp == 0 || comp == 1)

	//	Forward declarations
	union TPUID;
	struct PPACHEADER;
	struct PPACINDEX;
	struct PPACINDEXENTRY;
	struct PPACMETA;
	struct PPACMETABLOCK;
	struct PPACMETABLOCKENTRY;
	struct PPACTRASHINDEX;
	struct PPACTRASHINDEXENTRY;
	class OPENPACFILEHANDLE;
	class cPPAC;
	class cPPACData;
	class cPPACManager;

	typedef union TPUID
	{
		struct
		{
			uint16 t;
			uint16 p;
			uint32 u;
		};
		uint64 tpuid;
		struct
		{
			uint32 high_tp;
			uint32 low_u;
		};

		TPUID();
		TPUID(uint64 tpu);
		TPUID(const TPUID& tpu);
		TPUID(uint16 type, uint16 purpose, uint32 unique);

		TPUID operator+(const TPUID& rhs) const;
		TPUID operator-(const TPUID& rhs) const;
		TPUID operator^(const TPUID& rhs) const;
		TPUID operator|(const TPUID& rhs) const;
		TPUID operator&(const TPUID& rhs) const;
		TPUID operator~() const;
		TPUID& operator+=(const TPUID& rhs);
		TPUID& operator-=(const TPUID& rhs);
		TPUID& operator&=(const TPUID& rhs);
		TPUID& operator|=(const TPUID& rhs);
		TPUID& operator^=(const TPUID& rhs);
	} TPUID;

	bool operator==(const TPUID& lhs, const TPUID& rhs);
	bool operator!=(const TPUID& lhs, const TPUID& rhs);
	bool operator<(const TPUID& lhs, const TPUID& rhs);
	bool operator>(const TPUID& lhs, const TPUID& rhs);
	bool operator>=(const TPUID& lhs, const TPUID& rhs);
	bool operator<=(const TPUID& lhs, const TPUID& rhs);

	struct PPACTPUIDHASH
	{
		size_t operator()(const TPUID& tpuid) const
		{
			return std::hash<uint64_t>()(tpuid.tpuid);
		}
	};

	static_assert(sizeof(TPUID) == sizeof(uint64), "TPUID isn't the size of a uint64");
	static_assert(offsetof(TPUID, low_u) == offsetof(TPUID, u), "low_u and u should have the same offset");
	static_assert(offsetof(TPUID, high_tp) == offsetof(TPUID, t), "high_tp and t should have the same offset");
	static_assert(offsetof(TPUID, t) == 0, "t offset should be 0");

	typedef struct PPACHEADER
	{
		uint32 hMagic;
		uint16 hMajorVer;
		uint16 hMinorVer;
		uint8 hReserved[16];
		uint32 hFlags;
		OFFSET hIndexOffset;
		OFFSET hMetadataOffset;
		OFFSET hTrashIndexOffset;
	} PPACHEADER;

	const int DISKSZ_PPACHEADER = sizeof(uint32) * 2 +
		sizeof(uint16) * 2 +
		sizeof(uint8) * 16 +
		sizeof(OFFSET) * 3;

	typedef struct PPACINDEX
	{
		typedef std::unordered_map<TPUID, PPACINDEXENTRY, PPACTPUIDHASH> PPACINDEXENTRIES;
		uint32 iCount;
		PPACINDEXENTRIES iEntries;
		GUARD iGuard;
	} PPACINDEX;

	typedef struct PPACINDEXENTRY
	{
		TPUID ieTPUID;
		OFFSET ieDiskOffset;
		uint32 ieDiskSize;
		uint32 ieMemorySize;
		uint8 ieCompressionType;
		uint8 ieReserved[3];
		uint8 ieSHA256[32];
	} PPACINDEXENTRY;

	const size_t DISKSZ_PPACINDEXENTRY = sizeof(uint16) * 2 +
		sizeof(uint32) +
		sizeof(OFFSET) +
		sizeof(uint32) * 2 +
		sizeof(uint8) +
		sizeof(uint8) * 3 +
		sizeof(uint8) * 32;

	typedef struct PPACMETA
	{
		uint32 mSize;
		uint32 mCount;
		std::vector<PPACMETABLOCK> mBlocks;
		GUARD mGuard;
	} PPACMETA;

	typedef struct PPACMETABLOCK
	{
		typedef std::unordered_map<std::string, std::string> PPACMETABLOCKENTRIES;
		TPUID mbTPUID;
		uint16 mbCount;
		uint16 mbSize;
		PPACMETABLOCKENTRIES mbEntries;
	} PPACMETABLOCK;

	typedef struct PPACMETABLOCKENTRY
	{
		uint8 mbeKeySize;
		uint8 mbeValSize;
		std::string mbeKey;
		std::string mbeVal;

		void AddToMap(PPACMETABLOCK::PPACMETABLOCKENTRIES& map);
	} PPACMETABLOCKENTRY;

	typedef struct PPACTRASHINDEX
	{
		uint32 tCount;
		std::vector<PPACTRASHINDEXENTRY> tEntries;
		GUARD tGuard;
	} PPACTRASHINDEX;

	typedef struct PPACTRASHINDEXENTRY
	{
		OFFSET teOffset;
		uint32 teSize;
	} PPACTRASHINDEXENTRY;

	class OPENPACFILEHANDLE
	{
		LPCWSTR _name;
	public:
		explicit OPENPACFILEHANDLE(HANDLE handle, LPCWSTR name);
		~OPENPACFILEHANDLE();
		HANDLE _handle;
	};

	class cPPAC
	{
	private:
		PPACHEADER _header;
		PPACINDEX _index;
		PPACMETA _meta;
		PPACTRASHINDEX _trash;
		std::unique_ptr<OPENPACFILEHANDLE> _handle;
		std::mutex _readMutex;

		bool _ReadHeader(const LPCWSTR file);
		void _ReadIndex(const bool needSwp, const LPCWSTR file);
		void _ReadMetadata(const bool needSwp, const LPCWSTR file);

	public:
		explicit cPPAC(LPCWSTR file);
		~cPPAC() = default;

		std::unique_ptr<cPPACData> GetFileData(const TPUID& tpuid);

		std::vector<PPACINDEXENTRY> GetEntries(const TPUID& filter = 0UL, const TPUID& mask = 0UL);

		const PPACINDEX::PPACINDEXENTRIES & GetEntryMap() const;

		PPACMETABLOCK::PPACMETABLOCKENTRIES GetMetadata(const TPUID& tpuid);
	};

	class cPPACData
	{
	public:
		cPPACData(TPUID tpuid, uint32 size);
		~cPPACData();
		TPUID _tpuid;
		uint32 _size;
		std::vector<uint8> _data;
	};

	class cPPACManager
	{
	private:
		typedef std::unordered_map<TPUID, uint32, PPACTPUIDHASH> TPUIDHandleMap;

		packed_freelist<cPPAC> _loadedPPACs;
		TPUIDHandleMap _tpuidToPPACMap;
	public:
		cPPACManager();
		~cPPACManager();

		PPACHANDLE LoadPPAC(std::wstring pathToPPAC);

		bool LoadDir(std::wstring pathToDir);

		bool FileExists(const TPUID& tpuid);

		std::unique_ptr<cPPACData> GetFileData(const TPUID& tpuid);
	};
}
