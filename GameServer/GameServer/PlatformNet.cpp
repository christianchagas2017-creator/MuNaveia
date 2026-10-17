// Emulacao minima de Winsock IOCP / WSAAsyncSelect / fila de mensagens para Linux e macOS.
// No Windows este arquivo e vazio: o servidor usa a API nativa.
#ifndef _WIN32

#include "PlatformCompat.h"
#include <map>
#include <deque>
#include <vector>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <algorithm>
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>

#if defined(__linux__)
#include <sys/epoll.h>
#endif

namespace
{
	// ------------------------------------------------------------------
	// Porta de conclusao (IOCP) sobre epoll
	// ------------------------------------------------------------------
	struct PendingOp { bool active = false; char* buf = 0; ULONG len = 0; LPWSAOVERLAPPED ov = 0; };
	struct SockState { DWORD key = 0; bool registered = false; PendingOp recv, send; };
	struct Port { int epfd; std::mutex m; std::map<int, SockState> socks; };

	std::mutex g_portsMutex;
	std::map<int, Port*> g_portBySocket; // fd -> porta
	std::map<int, int> g_wsaSelectFd;    // placeholder para limpeza em closesocket

	void SetNonBlocking(int fd) { int f = fcntl(fd, F_GETFL, 0); fcntl(fd, F_SETFL, f | O_NONBLOCK); }

	void Arm(Port* p, int fd, SockState& st)
	{
		epoll_event ev; memset(&ev, 0, sizeof(ev));
		ev.data.fd = fd;
		ev.events = EPOLLONESHOT | EPOLLRDHUP;
		if (st.recv.active) ev.events |= EPOLLIN;
		if (st.send.active) ev.events |= EPOLLOUT;
		if (epoll_ctl(p->epfd, EPOLL_CTL_MOD, fd, &ev) != 0 && errno == ENOENT)
			epoll_ctl(p->epfd, EPOLL_CTL_ADD, fd, &ev);
	}

	Port* PortOf(int fd)
	{
		std::lock_guard<std::mutex> l(g_portsMutex);
		auto it = g_portBySocket.find(fd);
		return it == g_portBySocket.end() ? 0 : it->second;
	}

	// ------------------------------------------------------------------
	// Fila de mensagens e timers
	// ------------------------------------------------------------------
	std::mutex g_msgMutex;
	std::condition_variable g_msgCv;
	std::deque<MSG> g_msgs;
	bool g_quit = false;
	int g_quitCode = 0;

	struct TimerEntry { HWND hwnd; UINT_PTR id; UINT ms; std::atomic<bool>* stop; };
	std::mutex g_timerMutex;
	std::vector<TimerEntry> g_timers;

	// ------------------------------------------------------------------
	// WSAAsyncSelect sobre poll
	// ------------------------------------------------------------------
	struct SelectEntry { HWND hwnd; UINT msg; long events; unsigned long long lastPost; bool wrote; };
	std::mutex g_selMutex;
	std::map<int, SelectEntry> g_sel;
	std::thread* g_selThread = 0;

	unsigned long long NowMs()
	{
		return (unsigned long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	void SelectLoop()
	{
		while (true)
		{
			std::vector<pollfd> fds;
			{
				std::lock_guard<std::mutex> l(g_selMutex);
				for (auto& kv : g_sel)
				{
					pollfd p; p.fd = kv.first; p.revents = 0;
					p.events = POLLIN | POLLRDHUP;
					if ((kv.second.events & FD_WRITE) && !kv.second.wrote) p.events |= POLLOUT;
					fds.push_back(p);
				}
			}
			poll(fds.data(), (nfds_t)fds.size(), 10);
			unsigned long long now = NowMs();
			std::lock_guard<std::mutex> l(g_selMutex);
			for (auto& p : fds)
			{
				auto it = g_sel.find(p.fd);
				if (it == g_sel.end() || p.revents == 0) continue;
				SelectEntry& e = it->second;
				if ((p.revents & POLLOUT) && (e.events & FD_WRITE) && !e.wrote)
				{
					e.wrote = true;
					PostMessage(e.hwnd, e.msg, (WPARAM)p.fd, FD_WRITE);
				}
				if ((p.revents & POLLIN) && (e.events & FD_READ) && now - e.lastPost >= 5)
				{
					int avail = 0;
					ioctl(p.fd, FIONREAD, &avail);
					e.lastPost = now;
					if (avail > 0) PostMessage(e.hwnd, e.msg, (WPARAM)p.fd, FD_READ);
					else if (p.revents & (POLLHUP | POLLRDHUP)) { PostMessage(e.hwnd, e.msg, (WPARAM)p.fd, FD_CLOSE); g_sel.erase(it); }
					else PostMessage(e.hwnd, e.msg, (WPARAM)p.fd, FD_CLOSE), g_sel.erase(it);
				}
				else if (p.revents & (POLLHUP | POLLERR | POLLRDHUP))
				{
					PostMessage(e.hwnd, e.msg, (WPARAM)p.fd, FD_CLOSE);
					g_sel.erase(it);
				}
			}
		}
	}
}

int closesocket(SOCKET s)
{
	{
		std::lock_guard<std::mutex> l(g_selMutex);
		g_sel.erase(s);
	}
	Port* p = 0;
	{
		std::lock_guard<std::mutex> l(g_portsMutex);
		auto it = g_portBySocket.find(s);
		if (it != g_portBySocket.end()) { p = it->second; g_portBySocket.erase(it); }
	}
	if (p)
	{
		std::lock_guard<std::mutex> l(p->m);
		epoll_ctl(p->epfd, EPOLL_CTL_DEL, s, 0);
		p->socks.erase(s);
	}
	return close(s);
}

SOCKET WSASocket(int af, int type, int proto, void*, GROUP, DWORD)
{
	signal(SIGPIPE, SIG_IGN);
	int s = socket(af, type, proto);
	if (s >= 0) { int one = 1; setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)); }
	return s;
}

void GetSystemInfo(SYSTEM_INFO* si)
{
	unsigned n = std::thread::hardware_concurrency();
	si->dwNumberOfProcessors = n ? n : 1;
}

HANDLE CreateIoCompletionPort(HANDLE file, HANDLE port, DWORD key, DWORD)
{
	int fd = (int)(intptr_t)file;
	if (!port)
	{
		// O codigo original cria a porta passando um socket temporario; so precisamos do epoll.
		Port* p = new Port();
		p->epfd = epoll_create1(0);
		if (p->epfd < 0) { delete p; return 0; }
		return p;
	}
	Port* p = (Port*)port;
	SetNonBlocking(fd);
	{
		std::lock_guard<std::mutex> l(g_portsMutex);
		g_portBySocket[fd] = p;
	}
	std::lock_guard<std::mutex> l(p->m);
	SockState& st = p->socks[fd];
	st.key = key;
	st.registered = true;
	return port;
}

int WSARecv(SOCKET s, LPWSABUF bufs, DWORD, DWORD* received, DWORD*, LPWSAOVERLAPPED ov, void*)
{
	Port* p = PortOf(s);
	if (!p) { errno = ENOTSOCK; return SOCKET_ERROR; }
	std::lock_guard<std::mutex> l(p->m);
	auto it = p->socks.find(s);
	if (it == p->socks.end()) { errno = ENOTSOCK; return SOCKET_ERROR; }
	it->second.recv.active = true;
	it->second.recv.buf = bufs->buf;
	it->second.recv.len = bufs->len;
	it->second.recv.ov = ov;
	Arm(p, s, it->second);
	if (received) *received = 0;
	errno = WSA_IO_PENDING;
	return SOCKET_ERROR;
}

int WSASend(SOCKET s, LPWSABUF bufs, DWORD, DWORD* sent, DWORD, LPWSAOVERLAPPED ov, void*)
{
	Port* p = PortOf(s);
	if (!p) { errno = ENOTSOCK; return SOCKET_ERROR; }
	std::lock_guard<std::mutex> l(p->m);
	auto it = p->socks.find(s);
	if (it == p->socks.end()) { errno = ENOTSOCK; return SOCKET_ERROR; }
	it->second.send.active = true;
	it->second.send.buf = bufs->buf;
	it->second.send.len = bufs->len;
	it->second.send.ov = ov;
	Arm(p, s, it->second);
	if (sent) *sent = 0;
	errno = WSA_IO_PENDING;
	return SOCKET_ERROR;
}

BOOL GetQueuedCompletionStatus(HANDLE port, DWORD* bytes, DWORD* key, LPOVERLAPPED* ov, DWORD ms)
{
	Port* p = (Port*)port;
	while (true)
	{
		epoll_event ev;
		int n = epoll_wait(p->epfd, &ev, 1, ms == INFINITE ? -1 : (int)ms);
		if (n < 0) { if (errno == EINTR) continue; *ov = 0; return FALSE; }
		if (n == 0) { *ov = 0; return FALSE; }

		int fd = ev.data.fd;
		std::lock_guard<std::mutex> l(p->m);
		auto it = p->socks.find(fd);
		if (it == p->socks.end()) continue;
		SockState& st = it->second;

		bool readable = (ev.events & (EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLRDHUP)) != 0;
		bool writable = (ev.events & (EPOLLOUT | EPOLLERR | EPOLLHUP)) != 0;

		if (readable && st.recv.active)
		{
			ssize_t r = recv(fd, st.recv.buf, st.recv.len, 0);
			if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) { Arm(p, fd, st); continue; }
			st.recv.active = false;
			*bytes = r < 0 ? 0 : (DWORD)r; // 0 = conexao encerrada; o chamador desconecta
			*key = st.key;
			*ov = (LPOVERLAPPED)st.recv.ov;
			if (st.send.active) Arm(p, fd, st);
			return TRUE;
		}
		if (writable && st.send.active)
		{
			ssize_t r = send(fd, st.send.buf, st.send.len, MSG_NOSIGNAL);
			if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) { Arm(p, fd, st); continue; }
			st.send.active = false;
			*bytes = r < 0 ? 0 : (DWORD)r;
			*key = st.key;
			*ov = (LPOVERLAPPED)st.send.ov;
			if (st.recv.active) Arm(p, fd, st);
			return TRUE;
		}
		// evento sem operacao pendente (corrida com closesocket): ignora
	}
}

SOCKET WSAAccept(SOCKET s, sockaddr* addr, int* addrlen, LPCONDITIONPROC cond, DWORD_PTR data)
{
	sockaddr_in sa; socklen_t len = sizeof(sa);
	int c = accept(s, (sockaddr*)&sa, &len);
	if (c < 0) return SOCKET_ERROR;
	if (addr) { memcpy(addr, &sa, len < (socklen_t)*addrlen ? len : (socklen_t)*addrlen); *addrlen = (int)len; }
	if (cond)
	{
		WSABUF caller; caller.buf = (char*)&sa; caller.len = len;
		if (cond(&caller, 0, 0, 0, 0, 0, 0, data) == CF_REJECT)
		{
			close(c);
			errno = ECONNREFUSED;
			return SOCKET_ERROR;
		}
	}
	int one = 1;
	setsockopt(c, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
	return c;
}

BOOL PostMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	std::lock_guard<std::mutex> l(g_msgMutex);
	MSG m; m.hwnd = hwnd; m.message = msg; m.wParam = wp; m.lParam = lp;
	g_msgs.push_back(m);
	g_msgCv.notify_one();
	return TRUE;
}

BOOL GetMessage(MSG* msg, HWND, UINT, UINT)
{
	std::unique_lock<std::mutex> l(g_msgMutex);
	g_msgCv.wait(l, [] { return g_quit || !g_msgs.empty(); });
	if (g_quit && g_msgs.empty()) { msg->message = WM_QUIT; msg->wParam = (WPARAM)g_quitCode; return FALSE; }
	*msg = g_msgs.front();
	g_msgs.pop_front();
	return TRUE;
}

void PostQuitMessage(int code)
{
	std::lock_guard<std::mutex> l(g_msgMutex);
	g_quit = true;
	g_quitCode = code;
	g_msgCv.notify_all();
}

UINT_PTR SetTimer(HWND hwnd, UINT_PTR id, UINT ms, void*)
{
	std::atomic<bool>* stop = new std::atomic<bool>(false);
	{
		std::lock_guard<std::mutex> l(g_timerMutex);
		for (auto& t : g_timers) if (t.hwnd == hwnd && t.id == id) *t.stop = true; // substitui timer existente
		TimerEntry e = { hwnd, id, ms, stop };
		g_timers.push_back(e);
	}
	std::thread([hwnd, id, ms, stop]()
	{
		while (!*stop)
		{
			for (UINT waited = 0; waited < ms && !*stop; waited += 10) Sleep(10);
			if (*stop) break;
			PostMessage(hwnd, WM_TIMER, id, 0);
		}
	}).detach();
	return id;
}

BOOL KillTimer(HWND hwnd, UINT_PTR id)
{
	std::lock_guard<std::mutex> l(g_timerMutex);
	for (auto& t : g_timers) if (t.hwnd == hwnd && t.id == id) *t.stop = true;
	return TRUE;
}

int WSAAsyncSelect(SOCKET s, HWND hwnd, UINT msg, long events)
{
	SetNonBlocking(s);
	std::lock_guard<std::mutex> l(g_selMutex);
	SelectEntry e = { hwnd, msg, events, 0, false };
	g_sel[s] = e;
	if (!g_selThread) g_selThread = new std::thread(SelectLoop);
	return 0;
}

#endif // !_WIN32
