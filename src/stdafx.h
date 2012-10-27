// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include <afxwin.h>
#include <cstdio>
#include <tchar.h>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <conio.h>
#include <atlstr.h>
#include <shlobj.h> 
#include <afxmt.h>
#include <map>
#include <vector>
#include <list>
#include <iterator>
#include <algorithm>
#include <Mmdeviceapi.h>
#include <mmsystem.h>
#include <Audioclient.h>

#include "include\libspotify\api.h"

extern const uint8_t g_appkey[];
extern const size_t g_appkey_size;

#include "StudioException.h"

extern void log( std::exception& ex );
extern void log( StudioException& ex );
extern void log( const char *fmt, ... );
extern void log_status( const char *fmt, ... );

extern CString getUserDocumentDirectory();

namespace DMXStudio {

void log_status( const char *fmt, ... );

};
