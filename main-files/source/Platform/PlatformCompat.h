#pragma once

// Camada de compatibilidade do cliente para plataformas nao-Windows
// (Android, iOS, Linux). No Windows este arquivo NAO faz nada: o codigo
// continua usando as APIs nativas via windows.h / mmsystem.h.
//
// Os nomes (GetTickCount, timeGetTime, Sleep, QueryPerformanceCounter...) sao
// mantidos de proposito para que os pontos de chamada existentes nao mudem.

#ifndef _WIN32

#include <stdint.h>
#include <chrono>
#include <thread>

#ifndef _PLATFORM_COMPAT_BASIC_TYPES
#define _PLATFORM_COMPAT_BASIC_TYPES
typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t  BOOL;
typedef long long LONGLONG;
#include <string.h>
#ifndef ZeroMemory
#define ZeroMemory(p, n) memset((p), 0, (n))
#endif
#ifndef CopyMemory
#define CopyMemory(d, s, n) memcpy((d), (s), (n))
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#endif

typedef union _LARGE_INTEGER {
	struct { uint32_t LowPart; int32_t HighPart; } u;
	LONGLONG QuadPart;
} LARGE_INTEGER;

namespace Platform
{
	// Milissegundos monotonicos desde um ponto arbitrario (nao sofre ajuste de relogio).
	inline uint64_t NowMs()
	{
		using namespace std::chrono;
		return (uint64_t)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
	}

	inline uint64_t NowNs()
	{
		using namespace std::chrono;
		return (uint64_t)duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
	}
}

// Mesma semantica do Windows: DWORD em ms, com wrap em ~49 dias.
inline DWORD GetTickCount() { return (DWORD)Platform::NowMs(); }
inline DWORD timeGetTime()  { return (DWORD)Platform::NowMs(); }

inline void Sleep(DWORD ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

// Contador de alta resolucao em nanossegundos (frequencia = 1e9).
inline BOOL QueryPerformanceFrequency(LARGE_INTEGER* f) { f->QuadPart = 1000000000LL; return TRUE; }
inline BOOL QueryPerformanceCounter(LARGE_INTEGER* c)   { c->QuadPart = (LONGLONG)Platform::NowNs(); return TRUE; }

// ---- Arquivos ----
// Todo fopen() do cliente passa a aceitar caminhos no estilo Windows e ignorar
// maiusculas/minusculas. <cstdio> precisa vir antes (std::fopen e chamado com
// parenteses dentro de PlatformFile.h para nao ser capturado pelo macro).
#include <cstdio>
#include "PlatformFile.h"
#define fopen(path, mode) ::Platform::FOpen((path), (mode))

// ---- Rede (Winsock -> BSD sockets + poll) ----
#include "PlatformSocket.h"

#endif // !_WIN32
