#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN

#define _WIN32_WINNT _WIN32_WINNT_WINXP

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#define GAMESERVER_VERSION "VoxMu"


#ifndef GAMESERVER_TYPE
#define GAMESERVER_TYPE 0
#endif

#ifndef GAMESERVER_EXTRA
#define GAMESERVER_EXTRA 1
#endif

#ifndef GAMESERVER_UPDATE
#define GAMESERVER_UPDATE 505
#endif

#ifndef GAMESERVER_LANGUAGE
#define GAMESERVER_LANGUAGE 1
#endif

#define CHAOS_MACHINE_GENESIS
#define TRADE_X

// System Include
#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <mswSock.h>
#include <commctrl.h>
#else
#include "PlatformCompat.h"
#endif
#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <time.h>
#include <math.h>
#include <map>
#include <vector>
#include <queue>
#include <random>
#ifdef _WIN32
#include <Rpc.h>
#endif
#include <algorithm>
#include <string>
#ifdef _WIN32
#include <atltime.h>
#include <dbghelp.h>
#include <Psapi.h>
#include <timeapi.h>
#endif
#include <chrono>
#include <list> 
#pragma pack(push, 1)
//Includes Customs

#include "ElementSlots.h"
#include "DarkSpiritNew.h"

#define ENC(x) (char*)(x)

#ifdef _WIN32
#pragma comment(lib,"User32.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Rpcrt4.lib")
#pragma comment(lib,"dbghelp.lib")
#pragma comment(lib,"Psapi.lib")
#pragma comment(lib,"Gdi32.lib")
#pragma comment(lib,"Winmm.lib")
#endif

#if(GAMESERVER_UPDATE>=701)
#if(NDEBUG==0)
#pragma comment(lib,"..\\..\\..\\Util\\cryptopp\\Debug\\cryptlib.lib")
#else
#pragma comment(lib,"..\\..\\..\\Util\\cryptopp\\Release\\cryptlib.lib")
#endif
#pragma comment(lib,"..\\..\\..\\Util\\mapm\\mapm.lib")
#endif

extern char LuaDecryptKey[50];
extern char CustomerName[50];
extern char CustomerHardwareId[45];
extern int ClientId;

typedef char chr;

typedef float flt;

typedef short shrt;

#ifdef _WIN32
typedef unsigned __int64 QWORD;
#else
typedef unsigned long long QWORD;
#endif
