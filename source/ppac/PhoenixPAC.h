#pragma once
#include <cstdint>

namespace PPAC
{
	typedef uint8_t uint8;
	typedef uint16_t uint16;
	typedef uint32_t uint32;
	typedef uint64_t uint64;
#ifdef PPAC_WIDE
	typedef uint64 WIDE;
#else
	typedef uint32 WIDE;
#endif

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
		WIDE hIndexOffset;
		WIDE hMetadataOffset;
		WIDE hTrashIndexOffset;
	} PPACHEADER;
}
