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
#define PPAC_CHK_HEAD_MAGIC(head) (head.hMagic == PPAC_HEAD_MAGIC)
#define PPAC_CHK_INDEX_GUARD(index) (index.iGuard == PPAC_INDEX_GUARD)
#define PPAC_CHK_META_GUARD(meta) (meta.mGuard == PPAC_META_GUARD)
#define PPAC_CHK_TRASH_GUARD(trash) (trash.iGuard == PPAC_TRASH_GUARD)

#define PPACF_USE_LONG_OFFSETS 0x00000001
#define PPACF_JAVA_ARRAY_COMPAT 0x00000002
#define PPACF_CHK_FLAG(flags, flag) ((flags & flags) != 0)

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
