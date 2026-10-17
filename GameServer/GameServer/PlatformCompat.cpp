#ifndef _WIN32

#include "PlatformCompat.h"
#include <string>
#include <fstream>
#include <vector>

void GetLocalTime(SYSTEMTIME* st)
{
	timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	tm t;
	localtime_r(&ts.tv_sec, &t);
	st->wYear = (WORD)(t.tm_year + 1900);
	st->wMonth = (WORD)(t.tm_mon + 1);
	st->wDayOfWeek = (WORD)t.tm_wday;
	st->wDay = (WORD)t.tm_mday;
	st->wHour = (WORD)t.tm_hour;
	st->wMinute = (WORD)t.tm_min;
	st->wSecond = (WORD)t.tm_sec;
	st->wMilliseconds = (WORD)(ts.tv_nsec / 1000000);
}

void GetSystemTime(SYSTEMTIME* st)
{
	timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	tm t;
	gmtime_r(&ts.tv_sec, &t);
	st->wYear = (WORD)(t.tm_year + 1900);
	st->wMonth = (WORD)(t.tm_mon + 1);
	st->wDayOfWeek = (WORD)t.tm_wday;
	st->wDay = (WORD)t.tm_mday;
	st->wHour = (WORD)t.tm_hour;
	st->wMinute = (WORD)t.tm_min;
	st->wSecond = (WORD)t.tm_sec;
	st->wMilliseconds = (WORD)(ts.tv_nsec / 1000000);
}

void PlatformNormalizePath(char* path)
{
	for (; path && *path; ++path)
	{
		if (*path == '\\')
		{
			*path = '/';
		}
	}
}

static std::string Trim(const std::string& s)
{
	size_t a = s.find_first_not_of(" \t\r\n");
	if (a == std::string::npos) return "";
	size_t b = s.find_last_not_of(" \t\r\n");
	return s.substr(a, b - a + 1);
}

static bool EqualNoCase(const std::string& a, const char* b)
{
	return strcasecmp(a.c_str(), b) == 0;
}

// Procura "key" dentro de [section]; devolve false se nao existir.
static bool IniLookup(const char* section, const char* key, const char* file, std::string& value)
{
	std::string path(file ? file : "");
	std::vector<char> buf(path.begin(), path.end());
	buf.push_back(0);
	PlatformNormalizePath(buf.data());

	std::ifstream in(buf.data());
	if (!in) return false;

	bool inSection = false;
	std::string line;
	while (std::getline(in, line))
	{
		line = Trim(line);
		if (line.empty() || line[0] == ';') continue;
		if (line[0] == '[')
		{
			size_t e = line.find(']');
			inSection = (e != std::string::npos) && EqualNoCase(Trim(line.substr(1, e - 1)), section);
			continue;
		}
		if (!inSection) continue;
		size_t eq = line.find('=');
		if (eq == std::string::npos) continue;
		if (EqualNoCase(Trim(line.substr(0, eq)), key))
		{
			value = Trim(line.substr(eq + 1));
			return true;
		}
	}
	return false;
}

UINT GetPrivateProfileInt(LPCSTR section, LPCSTR key, int def, LPCSTR file)
{
	std::string v;
	if (!IniLookup(section, key, file, v)) return (UINT)def;
	return (UINT)strtol(v.c_str(), 0, 10);
}

DWORD GetPrivateProfileString(LPCSTR section, LPCSTR key, LPCSTR def, LPSTR out, DWORD size, LPCSTR file)
{
	std::string v;
	if (!IniLookup(section, key, file, v)) v = def ? def : "";
	if (size == 0) return 0;
	strncpy(out, v.c_str(), size - 1);
	out[size - 1] = 0;
	return (DWORD)strlen(out);
}

BOOL WritePrivateProfileString(LPCSTR, LPCSTR, LPCSTR, LPCSTR)
{
	// Ainda nao implementado fora do Windows (o servidor so le .ini no fluxo normal).
	return FALSE;
}


#include <sys/stat.h>
#include <dirent.h>
#include <fnmatch.h>
#include <semaphore.h>
#include <algorithm>
#include <condition_variable>
#include <atomic>
#include <memory>

// ---------------- Arquivos ----------------
namespace
{
	enum KType { K_FILE = 1, K_THREAD, K_SEM, K_TIMERQ, K_TIMER, K_FIND };
	struct KObj { int type; explicit KObj(int t) : type(t) {} virtual ~KObj() {} };
	struct KFile : KObj { int fd; KFile(int f) : KObj(K_FILE), fd(f) {} ~KFile() { if (fd >= 0) close(fd); } };
	struct KThread : KObj { pthread_t th; bool done; KThread() : KObj(K_THREAD), th(), done(false) {} };
	struct KSem : KObj { std::mutex m; std::condition_variable cv; LONG count; LONG max; KSem(LONG c, LONG mx) : KObj(K_SEM), count(c), max(mx) {} };
	struct KFind : KObj { std::vector<std::string> names; size_t pos; KFind() : KObj(K_FIND), pos(0) {} };
	thread_local DWORD g_lastError = 0;

	std::string Normalize(const char* p) { std::string s(p ? p : ""); for (char& c : s) if (c == '\\') c = '/'; return s; }

	// Windows e case-insensitive; procura o nome real caso o caminho exato nao exista.
	std::string Resolve(const std::string& path)
	{
		struct stat st;
		if (stat(path.c_str(), &st) == 0) return path;
		std::string dir = ".", base = path;
		size_t slash = path.find_last_of('/');
		if (slash != std::string::npos) { dir = path.substr(0, slash); base = path.substr(slash + 1); }
		std::string rdir = (dir == "." || dir.empty()) ? "." : Resolve(dir);
		DIR* d = opendir(rdir.c_str());
		if (!d) return path;
		std::string found;
		while (dirent* e = readdir(d)) { if (strcasecmp(e->d_name, base.c_str()) == 0) { found = e->d_name; break; } }
		closedir(d);
		if (found.empty()) return path;
		return (rdir == "." && slash == std::string::npos) ? found : rdir + "/" + found;
	}
}

HANDLE CreateFileA(LPCSTR name, DWORD access, DWORD, void*, DWORD disp, DWORD, HANDLE)
{
	std::string path = Normalize(name);
	int flags = (access & GENERIC_WRITE) ? ((access & GENERIC_READ) ? O_RDWR : O_WRONLY) : O_RDONLY;
	switch (disp)
	{
		case CREATE_NEW: flags |= O_CREAT | O_EXCL; break;
		case CREATE_ALWAYS: flags |= O_CREAT | O_TRUNC; break;
		case OPEN_ALWAYS: flags |= O_CREAT; break;
		default: break;
	}
	if (!(flags & O_CREAT)) path = Resolve(path);
	int fd = open(path.c_str(), flags, 0644);
	if (fd < 0) { g_lastError = (DWORD)errno; return INVALID_HANDLE_VALUE; }
	return new KFile(fd);
}

BOOL ReadFile(HANDLE f, void* buf, DWORD n, DWORD* got, void*)
{
	KFile* k = (KFile*)f;
	ssize_t r = read(k->fd, buf, n);
	if (r < 0) { g_lastError = (DWORD)errno; if (got) *got = 0; return FALSE; }
	if (got) *got = (DWORD)r;
	return TRUE;
}

BOOL WriteFile(HANDLE f, const void* buf, DWORD n, DWORD* written, void*)
{
	KFile* k = (KFile*)f;
	ssize_t r = write(k->fd, buf, n);
	if (r < 0) { g_lastError = (DWORD)errno; if (written) *written = 0; return FALSE; }
	if (written) *written = (DWORD)r;
	return TRUE;
}

DWORD GetFileSize(HANDLE f, DWORD* high)
{
	struct stat st;
	if (fstat(((KFile*)f)->fd, &st) != 0) return (DWORD)-1;
	if (high) *high = (DWORD)((unsigned long long)st.st_size >> 32);
	return (DWORD)st.st_size;
}

DWORD SetFilePointer(HANDLE f, LONG dist, LONG*, DWORD method)
{
	int whence = (method == FILE_BEGIN) ? SEEK_SET : (method == FILE_CURRENT ? SEEK_CUR : SEEK_END);
	off_t r = lseek(((KFile*)f)->fd, dist, whence);
	return r < 0 ? INVALID_SET_FILE_POINTER : (DWORD)r;
}

DWORD GetLastError() { return g_lastError; }

BOOL CloseHandle(HANDLE h)
{
	if (!h || h == INVALID_HANDLE_VALUE) return FALSE;
	KObj* k = (KObj*)h;
	if (k->type == K_THREAD) { delete (KThread*)k; return TRUE; }
	delete k;
	return TRUE;
}

// ---------------- Busca de arquivos (FindFirstFile) ----------------
HANDLE FindFirstFileA(LPCSTR pattern, WIN32_FIND_DATA* data)
{
	std::string pat = Normalize(pattern);
	std::string dir = ".", mask = pat;
	size_t slash = pat.find_last_of('/');
	if (slash != std::string::npos) { dir = pat.substr(0, slash); mask = pat.substr(slash + 1); }
	DIR* d = opendir(Resolve(dir).c_str());
	if (!d) { g_lastError = (DWORD)errno; return INVALID_HANDLE_VALUE; }
	KFind* f = new KFind();
	while (dirent* e = readdir(d))
	{
		if (fnmatch(mask.c_str(), e->d_name, FNM_CASEFOLD) == 0) f->names.push_back(e->d_name);
	}
	closedir(d);
	if (f->names.empty()) { delete f; g_lastError = 2; return INVALID_HANDLE_VALUE; }
	std::sort(f->names.begin(), f->names.end());
	strncpy(data->cFileName, f->names[0].c_str(), MAX_PATH - 1);
	data->cFileName[MAX_PATH - 1] = 0;
	data->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
	f->pos = 1;
	return f;
}

BOOL FindNextFileA(HANDLE h, WIN32_FIND_DATA* data)
{
	KFind* f = (KFind*)h;
	if (f->pos >= f->names.size()) return FALSE;
	strncpy(data->cFileName, f->names[f->pos].c_str(), MAX_PATH - 1);
	data->cFileName[MAX_PATH - 1] = 0;
	data->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
	f->pos++;
	return TRUE;
}

BOOL FindClose(HANDLE h) { if (h && h != INVALID_HANDLE_VALUE) delete (KFind*)h; return TRUE; }

// ---------------- Threads ----------------
namespace
{
	struct ThreadStart { LPTHREAD_START_ROUTINE fn; LPVOID arg; };
	void* ThreadTrampoline(void* p)
	{
		ThreadStart s = *(ThreadStart*)p;
		delete (ThreadStart*)p;
		s.fn(s.arg);
		return 0;
	}
}

HANDLE CreateThread(void*, size_t, LPTHREAD_START_ROUTINE fn, LPVOID arg, DWORD, DWORD* id)
{
	KThread* t = new KThread();
	ThreadStart* s = new ThreadStart{ fn, arg };
	if (pthread_create(&t->th, 0, ThreadTrampoline, s) != 0) { delete s; delete t; return 0; }
	if (id) *id = (DWORD)(uintptr_t)t->th;
	return t;
}

BOOL TerminateThread(HANDLE h, DWORD)
{
	if (!h) return FALSE;
	pthread_cancel(((KThread*)h)->th);
	return TRUE;
}

// ---------------- Semaforos ----------------
HANDLE CreateSemaphoreA(void*, LONG initial, LONG maximum, LPCSTR) { return new KSem(initial, maximum); }

BOOL ReleaseSemaphore(HANDLE s, LONG count, LONG* prev)
{
	KSem* k = (KSem*)s;
	std::lock_guard<std::mutex> lock(k->m);
	if (prev) *prev = k->count;
	if (k->count + count > k->max) return FALSE;
	k->count += count;
	k->cv.notify_all();
	return TRUE;
}

DWORD WaitForSingleObject(HANDLE h, DWORD ms)
{
	if (!h) return WAIT_FAILED;
	KObj* o = (KObj*)h;
	if (o->type == K_SEM)
	{
		KSem* k = (KSem*)o;
		std::unique_lock<std::mutex> lock(k->m);
		auto ready = [k] { return k->count > 0; };
		if (ms == INFINITE) k->cv.wait(lock, ready);
		else if (!k->cv.wait_for(lock, std::chrono::milliseconds(ms), ready)) return WAIT_TIMEOUT;
		k->count--;
		return WAIT_OBJECT_0;
	}
	if (o->type == K_THREAD) { pthread_join(((KThread*)o)->th, 0); return WAIT_OBJECT_0; }
	return WAIT_FAILED;
}

// ---------------- Timer queue ----------------
namespace
{
	struct KTimer : KObj
	{
		std::thread th; std::atomic<bool> stop; WAITORTIMERCALLBACK cb; PVOID param; DWORD due, period;
		KTimer() : KObj(K_TIMER), stop(false), cb(0), param(0), due(0), period(0) {}
		~KTimer() { stop = true; if (th.joinable()) th.join(); }
	};
}

HANDLE CreateTimerQueue() { return new KObj(K_TIMERQ); }
BOOL DeleteTimerQueue(HANDLE q) { delete (KObj*)q; return TRUE; }

BOOL CreateTimerQueueTimer(HANDLE* timer, HANDLE, WAITORTIMERCALLBACK cb, PVOID param, DWORD due, DWORD period, ULONG)
{
	KTimer* t = new KTimer();
	t->cb = cb; t->param = param; t->due = due; t->period = period;
	t->th = std::thread([t]()
	{
		auto sleepMs = [t](DWORD ms) { for (DWORD waited = 0; waited < ms && !t->stop; waited += 10) Sleep(ms - waited < 10 ? ms - waited : 10); };
		sleepMs(t->due);
		while (!t->stop)
		{
			t->cb(t->param, TRUE);
			if (t->period == 0) break;
			sleepMs(t->period);
		}
	});
	*timer = t;
	return TRUE;
}

BOOL DeleteTimerQueueTimer(HANDLE, HANDLE timer, HANDLE) { delete (KTimer*)timer; return TRUE; }

// ---------------- Diretorios, arquivos, mutex, MessageBox ----------------
#include <sys/file.h>

BOOL CreateDirectoryA(LPCSTR path, void*)
{
	std::string p = Normalize(path);
	if (mkdir(p.c_str(), 0755) == 0) return TRUE;
	g_lastError = (errno == EEXIST) ? ERROR_ALREADY_EXISTS : (DWORD)errno;
	return FALSE;
}

BOOL DeleteFileA(LPCSTR path)
{
	std::string p = Resolve(Normalize(path));
	if (unlink(p.c_str()) == 0) return TRUE;
	g_lastError = (DWORD)errno;
	return FALSE;
}

// Mutex nomeado -> arquivo de trava (flock). Serve para impedir duas instancias do servidor.
HANDLE CreateMutexA(void*, BOOL, LPCSTR name)
{
	std::string path = "/tmp/.munaveia-";
	for (const char* p = name ? name : "default"; *p; ++p) path += (isalnum((unsigned char)*p) ? *p : '_');
	path += ".lock";
	int fd = open(path.c_str(), O_CREAT | O_RDWR, 0666);
	if (fd < 0) { g_lastError = (DWORD)errno; return 0; }
	g_lastError = (flock(fd, LOCK_EX | LOCK_NB) == 0) ? 0 : ERROR_ALREADY_EXISTS;
	return new KFile(fd);
}

int MessageBoxA(HWND, LPCSTR text, LPCSTR caption, UINT type)
{
	fprintf(stderr, "[%s] %s\n", caption ? caption : "", text ? text : "");
	return (type & MB_YESNO) ? IDYES : IDOK;
}

#endif // !_WIN32
