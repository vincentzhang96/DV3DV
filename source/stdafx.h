// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#ifndef H_STDAFX
#define H_STDAFX
#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <windowsx.h>
#include <Psapi.h>
#include <iostream>
#include <Strsafe.h>
#include <cstdint>
#include <vector>
#include <memory>
#include <map>
#include <handleapi.h>
#include <mutex>
#include <unordered_map>
#include <stdio.h>

#include "lib/GL/glew.h"
#include "lib/GL/wglew.h"

#define ELPP_NO_DEFAULT_LOG_FILE
#include "lib/easylogging++.h"

#include "lib/json.hpp"

#define DV_CLASS_NAME L"DV3DV"

#include "lib/zlib.h"
#include "lib/jpeglib.h"
#include "lib/jerror.h"

#include "lib/packed_freelist.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <utf8.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SIZES_H

//	DDS stuff
#pragma pack(push, def, 4)
typedef struct {
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwFourCC;
	DWORD dwRGBBitCount;
	DWORD dwRBitMask;
	DWORD dwGBitMask;
	DWORD dwBBitMask;
	DWORD dwABitMask;
} DDS_PIXELFORMAT;

typedef struct {
	DWORD           dwSize;
	DWORD           dwFlags;
	DWORD           dwHeight;
	DWORD           dwWidth;
	DWORD           dwPitchOrLinearSize;
	DWORD           dwDepth;
	DWORD           dwMipMapCount;
	DWORD           dwReserved1[11];
	DDS_PIXELFORMAT ddspf;
	DWORD           dwCaps;
	DWORD           dwCaps2;
	DWORD           dwCaps3;
	DWORD           dwCaps4;
	DWORD           dwReserved2;
} DDS_HEADER;

#define DDS_FOURCC_DXT1 0x31545844
#define DDS_FOURCC_DXT2 0x32545844
#define DDS_FOURCC_DXT3 0x33545844
#define DDS_FOURCC_DXT4 0x34545844
#define DDS_FOURCC_DXT5 0x35545844
#define DDS_FOURCC_DX10 0x30315844

#pragma pack(pop, def)


#define GLBUFFEROFFSET(offset) reinterpret_cast<void*>(offset)
#define GLBUFFEROFFSETZERO reinterpret_cast<void*>(0)
#define GLBUFFEROFFSET_F(offset) GLBUFFEROFFSET(offset * sizeof(GLfloat))

#endif