#pragma once

// Camada de compatibilidade para compilar o GameServer fora do Windows.
// No Windows (MSVC) este arquivo nao faz nada: stdafx.h continua usando windows.h.

#ifndef _WIN32

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <pthread.h>
#include <mutex>
#include <chrono>
#include <thread>

// ---- Tipos basicos do Windows ----
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t BOOL;
typedef int32_t LONG;
typedef uint32_t ULONG;
#define __int64 long long
typedef unsigned int UINT;
typedef char CHAR;
typedef char TCHAR;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef const char* LPCTSTR;
typedef void* LPVOID;
typedef void* HANDLE;
typedef void* HWND;
typedef void* HINSTANCE;
typedef BYTE* LPBYTE;
typedef DWORD* LPDWORD;
typedef intptr_t LONG_PTR;
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define MAX_PATH 260
#define _MAX_PATH 260
#define CALLBACK
#define WINAPI
#define APIENTRY
#define __stdcall
#define __cdecl
#define INFINITE 0xFFFFFFFF

#define MAKEWORD(a,b) ((WORD)(((BYTE)(a))|(((WORD)((BYTE)(b)))<<8)))
#define LOWORD(l) ((WORD)(((uintptr_t)(l)) & 0xffff))
#define HIWORD(l) ((WORD)((((uintptr_t)(l)) >> 16) & 0xffff))
#define LOBYTE(w) ((BYTE)(((uintptr_t)(w)) & 0xff))
#define HIBYTE(w) ((BYTE)((((uintptr_t)(w)) >> 8) & 0xff))
#define ZeroMemory(p,n) memset((p),0,(n))
#define CopyMemory(d,s,n) memcpy((d),(s),(n))
#define FillMemory(p,n,v) memset((p),(v),(n))

// ---- Sockets (BSD no lugar de Winsock) ----
typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
int closesocket(SOCKET s);
inline int WSAGetLastError() { int e = errno; return e == EINPROGRESS ? EWOULDBLOCK : e; }
#define WSAEWOULDBLOCK EWOULDBLOCK
#define WSAENOTSOCK ENOTSOCK
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

// ---- Tempo e espera ----
inline void Sleep(DWORD ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

inline DWORD GetTickCount()
{
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (DWORD)(ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL);
}

inline DWORD timeGetTime() { return GetTickCount(); }

// ---- Secao critica ----
struct CRITICAL_SECTION_COMPAT { std::recursive_mutex m; };
typedef CRITICAL_SECTION_COMPAT CRITICAL_SECTION;
inline void InitializeCriticalSection(CRITICAL_SECTION*) {}
inline void DeleteCriticalSection(CRITICAL_SECTION*) {}
inline void EnterCriticalSection(CRITICAL_SECTION* cs) { cs->m.lock(); }
inline void LeaveCriticalSection(CRITICAL_SECTION* cs) { cs->m.unlock(); }

// ---- Funcoes de string do Windows ----
#define lstrcpy strcpy
#define lstrcpyn(d,s,n) strncpy((d),(s),(n))
#define lstrlen strlen
#define lstrcmp strcmp
#define lstrcmpi strcasecmp
#define stricmp strcasecmp
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define strnicmp strncasecmp
#define wsprintf sprintf
#define _snprintf snprintf
#define _atoi64 atoll

#include <stdarg.h>
#include <ctype.h>
#include <strings.h>

typedef int errno_t;
inline errno_t strcpy_s(char* d, size_t n, const char* s) { if (!d || n == 0) return EINVAL; strncpy(d, s ? s : "", n - 1); d[n - 1] = 0; return 0; }
template<size_t N> inline errno_t strcpy_s(char (&d)[N], const char* s) { return strcpy_s(d, N, s); }
inline errno_t strcat_s(char* d, size_t n, const char* s) { size_t l = strnlen(d, n); if (l >= n) return EINVAL; strncat(d, s, n - l - 1); return 0; }
template<size_t N> inline errno_t strcat_s(char (&d)[N], const char* s) { return strcat_s(d, N, s); }
inline errno_t strncpy_s(char* d, size_t n, const char* s, size_t c) { if (!d || n == 0) return EINVAL; size_t m = c < n - 1 ? c : n - 1; strncpy(d, s, m); d[m] = 0; return 0; }
template<size_t N> inline errno_t strncpy_s(char (&d)[N], const char* s, size_t c) { return strncpy_s(d, N, s, c); }
inline int vsprintf_s(char* d, size_t n, const char* f, va_list a) { return vsnprintf(d, n, f, a); }
template<size_t N> inline int vsprintf_s(char (&d)[N], const char* f, va_list a) { return vsnprintf(d, N, f, a); }
inline int sprintf_s(char* d, size_t n, const char* f, ...) { va_list a; va_start(a, f); int r = vsnprintf(d, n, f, a); va_end(a); return r; }
template<size_t N> inline int sprintf_s(char (&d)[N], const char* f, ...) { va_list a; va_start(a, f); int r = vsnprintf(d, N, f, a); va_end(a); return r; }
#define memcpy_s(d,dn,s,sn) (memcpy((d),(s),((sn)<(dn)?(sn):(dn))),0)

// ---- Tipos extras (ponteiros, GUI so como declaracao) ----
typedef void* PVOID;
typedef unsigned char BOOLEAN;
typedef void* HFONT;
typedef void* HBRUSH;
typedef void* HDC;
typedef void* HMENU;
typedef void* HICON;
typedef long LRESULT;
typedef WORD ATOM;
typedef uintptr_t UINT_PTR;
struct RECT { LONG left, top, right, bottom; };

// ---- Operacoes atomicas ----
inline LONG InterlockedIncrement(volatile LONG* p) { return __sync_add_and_fetch(p, 1); }
inline LONG InterlockedDecrement(volatile LONG* p) { return __sync_sub_and_fetch(p, 1); }
inline LONG InterlockedExchange(volatile LONG* p, LONG v) { return __sync_lock_test_and_set(p, v); }
inline LONG InterlockedExchangeAdd(volatile LONG* p, LONG v) { return __sync_fetch_and_add(p, v); }

// ---- Data e hora ----
struct SYSTEMTIME { WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds; };
void GetLocalTime(SYSTEMTIME* st);
void GetSystemTime(SYSTEMTIME* st);

class CTimeSpan
{
public:
	CTimeSpan() : m_s(0) {}
	CTimeSpan(long long secs) : m_s(secs) {}
	CTimeSpan(long days, int hours, int mins, int secs) : m_s((long long)days * 86400 + hours * 3600 + mins * 60 + secs) {}
	long long GetTotalSeconds() const { return m_s; }
	long long GetTotalMinutes() const { return m_s / 60; }
	long long GetTotalHours() const { return m_s / 3600; }
	long GetDays() const { return (long)(m_s / 86400); }
	int GetHours() const { return (int)((m_s / 3600) % 24); }
	int GetMinutes() const { return (int)((m_s / 60) % 60); }
	int GetSeconds() const { return (int)(m_s % 60); }
	CTimeSpan operator+(const CTimeSpan& o) const { return CTimeSpan(m_s + o.m_s); }
	CTimeSpan operator-(const CTimeSpan& o) const { return CTimeSpan(m_s - o.m_s); }
	bool operator<(const CTimeSpan& o) const { return m_s < o.m_s; }
	bool operator>(const CTimeSpan& o) const { return m_s > o.m_s; }
private:
	long long m_s;
};

// Equivalente ao CTime do ATL (hora local, base time_t)
class CTime
{
public:
	CTime() : m_t(0) {}
	CTime(time_t t) : m_t(t) {}
	CTime(int y, int mo, int d, int h, int mi, int s, int dst = -1)
	{
		tm t; memset(&t, 0, sizeof(t));
		t.tm_year = y - 1900; t.tm_mon = mo - 1; t.tm_mday = d;
		t.tm_hour = h; t.tm_min = mi; t.tm_sec = s; t.tm_isdst = dst;
		m_t = mktime(&t);
	}
	static CTime GetTickCount() { return CTime(time(0)); }
	static CTime GetCurrentTime() { return CTime(time(0)); }
	time_t GetTime() const { return m_t; }
	int GetYear() const { return Local().tm_year + 1900; }
	int GetMonth() const { return Local().tm_mon + 1; }
	int GetDay() const { return Local().tm_mday; }
	int GetHour() const { return Local().tm_hour; }
	int GetMinute() const { return Local().tm_min; }
	int GetSecond() const { return Local().tm_sec; }
	int GetDayOfWeek() const { return Local().tm_wday + 1; } // 1 = domingo, como no ATL
	CTime operator+(const CTimeSpan& s) const { return CTime(m_t + (time_t)s.GetTotalSeconds()); }
	CTime operator-(const CTimeSpan& s) const { return CTime(m_t - (time_t)s.GetTotalSeconds()); }
	CTimeSpan operator-(const CTime& o) const { return CTimeSpan((long long)(m_t - o.m_t)); }
	CTime& operator+=(const CTimeSpan& s) { m_t += (time_t)s.GetTotalSeconds(); return *this; }
	CTime& operator-=(const CTimeSpan& s) { m_t -= (time_t)s.GetTotalSeconds(); return *this; }
	bool operator==(const CTime& o) const { return m_t == o.m_t; }
	bool operator!=(const CTime& o) const { return m_t != o.m_t; }
	bool operator<(const CTime& o) const { return m_t < o.m_t; }
	bool operator>(const CTime& o) const { return m_t > o.m_t; }
	bool operator<=(const CTime& o) const { return m_t <= o.m_t; }
	bool operator>=(const CTime& o) const { return m_t >= o.m_t; }
private:
	tm Local() const { tm t; localtime_r(&m_t, &t); return t; }
	time_t m_t;
};

// ---- Arquivos .ini (GetPrivateProfile*) ----
UINT GetPrivateProfileInt(LPCSTR section, LPCSTR key, int def, LPCSTR file);
DWORD GetPrivateProfileString(LPCSTR section, LPCSTR key, LPCSTR def, LPSTR out, DWORD size, LPCSTR file);
BOOL WritePrivateProfileString(LPCSTR section, LPCSTR key, LPCSTR value, LPCSTR file);

// Converte "Data\\Arquivo.dat" em "Data/Arquivo.dat" (uso em fopen etc.)
void PlatformNormalizePath(char* path);


// ---- Objetos do kernel: arquivos, threads, semaforos, timers ----
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#define GENERIC_READ 0x80000000u
#define GENERIC_WRITE 0x40000000u
#define FILE_SHARE_READ 1
#define FILE_SHARE_WRITE 2
#define FILE_SHARE_DELETE 4
#define CREATE_NEW 1
#define CREATE_ALWAYS 2
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define FILE_ATTRIBUTE_READONLY 1
#define FILE_ATTRIBUTE_HIDDEN 2
#define FILE_ATTRIBUTE_DIRECTORY 0x10
#define FILE_ATTRIBUTE_ARCHIVE 0x20
#define FILE_ATTRIBUTE_NORMAL 0x80
#define FILE_BEGIN 0
#define FILE_CURRENT 1
#define FILE_END 2
#define WAIT_FAILED 0xFFFFFFFFu
#define WAIT_OBJECT_0 0
#define WAIT_TIMEOUT 258

HANDLE CreateFileA(LPCSTR name, DWORD access, DWORD share, void* sec, DWORD disp, DWORD attr, HANDLE tmpl);
#define CreateFile CreateFileA
BOOL ReadFile(HANDLE f, void* buf, DWORD n, DWORD* read, void* ov);
BOOL WriteFile(HANDLE f, const void* buf, DWORD n, DWORD* written, void* ov);
DWORD GetFileSize(HANDLE f, DWORD* high);
DWORD SetFilePointer(HANDLE f, LONG dist, LONG* high, DWORD method);
BOOL CloseHandle(HANDLE h);
DWORD GetLastError();

struct WIN32_FIND_DATA { DWORD dwFileAttributes; char cFileName[MAX_PATH]; };
typedef WIN32_FIND_DATA WIN32_FIND_DATAA;
HANDLE FindFirstFileA(LPCSTR pattern, WIN32_FIND_DATA* data);
BOOL FindNextFileA(HANDLE h, WIN32_FIND_DATA* data);
BOOL FindClose(HANDLE h);
#define FindFirstFile FindFirstFileA
#define FindNextFile FindNextFileA

typedef DWORD (*LPTHREAD_START_ROUTINE)(LPVOID);
HANDLE CreateThread(void* sec, size_t stack, LPTHREAD_START_ROUTINE fn, LPVOID arg, DWORD flags, DWORD* id);
BOOL TerminateThread(HANDLE t, DWORD code);
#define THREAD_PRIORITY_HIGHEST 2
#define THREAD_PRIORITY_NORMAL 0
inline HANDLE GetCurrentThread() { return 0; }
inline BOOL SetThreadPriority(HANDLE, int) { return TRUE; }
HANDLE CreateSemaphoreA(void* sec, LONG initial, LONG maximum, LPCSTR name);
#define CreateSemaphore CreateSemaphoreA
BOOL ReleaseSemaphore(HANDLE s, LONG count, LONG* prev);
DWORD WaitForSingleObject(HANDLE h, DWORD ms);

typedef void (*WAITORTIMERCALLBACK)(PVOID param, BOOLEAN timedOut);
#define WT_EXECUTEINTIMERTHREAD 0x20
HANDLE CreateTimerQueue();
BOOL DeleteTimerQueue(HANDLE q);
BOOL CreateTimerQueueTimer(HANDLE* timer, HANDLE queue, WAITORTIMERCALLBACK cb, PVOID param, DWORD due, DWORD period, ULONG flags);
BOOL DeleteTimerQueueTimer(HANDLE queue, HANDLE timer, HANDLE completion);

// ---- Rede estilo Winsock: IOCP e WSAAsyncSelect emulados com epoll/poll ----
typedef uintptr_t DWORD_PTR;
typedef uintptr_t ULONG_PTR;
#define WSA_FLAG_OVERLAPPED 1
#define WSA_IO_PENDING 997
#define CF_ACCEPT 1
#define CF_REJECT 0
#define ERROR_NETNAME_DELETED 64
#define ERROR_CONNECTION_ABORTED 1236
#define ERROR_OPERATION_ABORTED 995
#define ERROR_SEM_TIMEOUT 121
struct WSADATA { WORD wVersion; };
inline int WSAStartup(WORD, WSADATA*) { return 0; }
inline int WSACleanup() { return 0; }
struct WSABUF { ULONG len; char* buf; };
typedef WSABUF* LPWSABUF;
struct WSAOVERLAPPED { ULONG_PTR Internal; ULONG_PTR InternalHigh; DWORD Offset; DWORD OffsetHigh; HANDLE hEvent; };
typedef WSAOVERLAPPED OVERLAPPED;
typedef WSAOVERLAPPED* LPWSAOVERLAPPED;
typedef OVERLAPPED* LPOVERLAPPED;
struct QOS { int unused; };
typedef QOS* LPQOS;
typedef unsigned int GROUP;
typedef int (CALLBACK *LPCONDITIONPROC)(LPWSABUF, LPWSABUF, LPQOS, LPQOS, LPWSABUF, LPWSABUF, GROUP*, DWORD_PTR);
typedef void* LPWSAOVERLAPPED_COMPLETION_ROUTINE;

SOCKET WSASocket(int af, int type, int proto, void* info, GROUP g, DWORD flags);
int WSARecv(SOCKET s, LPWSABUF bufs, DWORD count, DWORD* received, DWORD* flags, LPWSAOVERLAPPED ov, void* routine);
int WSASend(SOCKET s, LPWSABUF bufs, DWORD count, DWORD* sent, DWORD flags, LPWSAOVERLAPPED ov, void* routine);
SOCKET WSAAccept(SOCKET s, sockaddr* addr, int* addrlen, LPCONDITIONPROC cond, DWORD_PTR data);
HANDLE CreateIoCompletionPort(HANDLE file, HANDLE port, DWORD key, DWORD threads);
BOOL GetQueuedCompletionStatus(HANDLE port, DWORD* bytes, DWORD* key, LPOVERLAPPED* ov, DWORD ms);

struct SYSTEM_INFO { DWORD dwNumberOfProcessors; };
void GetSystemInfo(SYSTEM_INFO* si);

// ---- Fila de mensagens (substitui o loop de janela do Windows) ----
#define WM_QUIT 0x0012
#define WM_TIMER 0x0113
#define WM_USER 0x0400
#define FD_READ 0x01
#define FD_WRITE 0x02
#define FD_CONNECT 0x10
#define FD_CLOSE 0x20
struct MSG { HWND hwnd; UINT message; WPARAM wParam; LPARAM lParam; };
BOOL PostMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
BOOL GetMessage(MSG* msg, HWND hwnd, UINT min, UINT max);
void PostQuitMessage(int code);
UINT_PTR SetTimer(HWND hwnd, UINT_PTR id, UINT ms, void* proc);
BOOL KillTimer(HWND hwnd, UINT_PTR id);
int WSAAsyncSelect(SOCKET s, HWND hwnd, UINT msg, long events);

// ---- Mais equivalentes do Win32 ----
#define _TRUNCATE ((size_t)-1)
typedef LONG* LPLONG;
typedef struct hostent HOSTENT;
#define RGB(r,g,b) ((DWORD)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
#define GPTR 0x40
#define FILE_ATTRIBUTE_TEMPORARY 0x100
#define ERROR_ALREADY_EXISTS 183
#define MB_OK 0
#define MB_YESNO 4
#define MB_ICONERROR 0x10
#define MB_ICONQUESTION 0x20
#define IDOK 1
#define IDYES 6
#define IDNO 7

inline BOOL SetRect(RECT* r, int l, int t, int rt, int b) { r->left = l; r->top = t; r->right = rt; r->bottom = b; return TRUE; }
inline HANDLE GlobalAlloc(UINT flags, size_t n) { return (flags & GPTR) ? calloc(1, n) : malloc(n); }
inline HANDLE GlobalFree(HANDLE p) { free(p); return 0; }
inline LONG InterlockedIncrement(volatile long* p) { return (LONG)__sync_add_and_fetch(p, 1); }
inline LONG InterlockedDecrement(volatile long* p) { return (LONG)__sync_sub_and_fetch(p, 1); }
inline LONG InterlockedExchange(volatile long* p, long v) { return (LONG)__sync_lock_test_and_set(p, v); }
inline LONG InterlockedExchangeAdd(volatile long* p, long v) { return (LONG)__sync_fetch_and_add(p, v); }

BOOL CreateDirectoryA(LPCSTR path, void* sec);
#define CreateDirectory CreateDirectoryA
BOOL DeleteFileA(LPCSTR path);
#define DeleteFile DeleteFileA
HANDLE CreateMutexA(void* sec, BOOL initialOwner, LPCSTR name);
#define CreateMutex CreateMutexA
int MessageBoxA(HWND hwnd, LPCSTR text, LPCSTR caption, UINT type);
#define MessageBox MessageBoxA

inline void ExitProcess(UINT code) { exit((int)code); }

// ---- Pequenos equivalentes da CRT/Win32 ----
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
#define _countof(a) (sizeof(a)/sizeof((a)[0]))
#define _strcmpi strcasecmp
#define IN
#define OUT
#define FAR

inline errno_t localtime_s(tm* out, const time_t* t) { return localtime_r(t, out) ? 0 : EINVAL; }
inline errno_t asctime_s(char* buf, size_t n, const tm* t) { if (n < 26) return EINVAL; asctime_r(t, buf); return 0; }
inline errno_t fopen_s(FILE** f, const char* name, const char* mode)
{
	char path[1024]; strncpy(path, name, sizeof(path) - 1); path[sizeof(path) - 1] = 0;
	for (char* p = path; *p; ++p) if (*p == '\\') *p = '/';
	*f = fopen(path, mode);
	return *f ? 0 : errno;
}

#define HEAP_ZERO_MEMORY 8
inline HANDLE GetProcessHeap() { return 0; }
inline LPVOID HeapAlloc(HANDLE, DWORD flags, size_t n) { return (flags & HEAP_ZERO_MEMORY) ? calloc(1, n) : malloc(n); }
inline BOOL HeapFree(HANDLE, DWORD, LPVOID p) { free(p); return TRUE; }

#endif // !_WIN32
