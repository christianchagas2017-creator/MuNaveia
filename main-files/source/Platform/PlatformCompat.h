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
typedef uint32_t DWORD;
typedef int32_t  BOOL;
typedef long long LONGLONG;
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

#endif // !_WIN32
