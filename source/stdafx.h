// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <shellapi.h>
#include <iostream>
#include <Strsafe.h>

#include "lib/GL/glew.h"
#include "lib/GL/wglew.h"

#define ELPP_NO_DEFAULT_LOG_FILE
#include "lib/easylogging++.h"


#define DV_CLASS_NAME L"DV3DV"
