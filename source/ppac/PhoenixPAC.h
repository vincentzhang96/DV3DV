#pragma once
#include <cstdint>
#include <cstddef>

namespace PPAC
{
	typedef uint8_t uint8;
	typedef uint16_t uint16;
	typedef uint32_t uint32;
	typedef uint64_t uint64;
	typedef uint32_t GUARD;
#ifdef PPAC_WIDE
	typedef uint64 OFFSET;
#else
	typedef uint32 OFFSET;
#endif

#define PPAC_HEAD_MAGIC 0x50504143
#define PPAC_INDEX_GUARD 0x494E4458
#define PPAC_META_GUARD 0x4D455441
#define PPAC_TRASH_GUARD 0x54525348
#define CHECK_HEAD_MAGIC(head) (head.hMagic == PPAC_HEAD_MAGIC)
#define CHECK_INDEX_GUARD(index) (index.iGuard == PPAC_INDEX_GUARD)
#define CHECK_META_GUARD(meta) (meta.mGuard == PPAC_META_GUARD)
#define CHECK_TRASH_GUARD(trash) (trash.iGuard == PPAC_TRASH_GUARD)

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

	typedef union TPUID
	{
		struct
		{
			uint16 t;
			uint16 g;
			uint32 u;
		};
		uint64 tpuid;
	} TPUID;

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

	typedef struct PPACINDEX
	{
		uint32 iCount;
		PPACINDEXENTRY* iEntries;
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
		uint8 ieSHA256;
	} PPACINDEXENTRY;

	typedef struct PPACMETA
	{
		uint32 mSize;
		uint32 mCount;
		PPACMETABLOCK* mBlocks;
		GUARD mGuard;
	} PPACMETA;

	typedef struct PPACMETABLOCK
	{
		TPUID mbTPUID;
		uint16 mbCount;
		uint16 mbSize;
		PPACMETABLOCKENTRY* mbEntries;
	} PPACMETABLOCK;

	typedef struct PPACMETABLOCKENTRY
	{
		uint8 mbeKeySize;
		uint8 mbeValSize;
		uint8* mbeKey;
		uint8* mbeVal;
	} PPACMETABLOCKENTRY;

	typedef struct PPACTRASHINDEX
	{
		uint32 tCount;
		PPACTRASHINDEXENTRY* tEntries;
		GUARD tGuard;
	} PPACTRASHINDEX;

	typedef struct PPACTRASHINDEXENTRY
	{
		OFFSET teOffset;
		uint32 teSize;
	} PPACTRASHINDEXENTRY;
}
