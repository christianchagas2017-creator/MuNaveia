#pragma once

// Sockets do cliente fora do Windows (Android, iOS, Linux).
//
// O cliente usa Winsock com WSAAsyncSelect: o Windows avisa FD_READ / FD_WRITE /
// FD_CLOSE por mensagens de janela e o WndProc chama CWsctlc::nRecv(),
// FDWriteSend() etc. Fora do Windows nao existe fila de mensagens, entao:
//
//   * WSAAsyncSelect() aqui apenas registra o socket (e o deixa nao bloqueante);
//   * a cada quadro o laco principal chama Platform::PollSocketEvents() e despacha
//     cada evento para o MESMO codigo que o WndProc executa hoje.
//
// Os nomes (SOCKET, closesocket, WSAGetLastError, WSAEWOULDBLOCK, ...) sao os do
// Winsock para que WSctlc.cpp nao precise mudar. No Windows este arquivo nao faz nada.

#ifndef _WIN32

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <mutex>
#include <vector>

#include "PlatformCompat.h" // BYTE/WORD/DWORD/BOOL

typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)
typedef struct sockaddr* LPSOCKADDR;
typedef struct linger LINGER;

#define SD_RECEIVE 0
#define SD_SEND    1
#define SD_BOTH    2

// Mesmos valores do Winsock.
#define FD_READ    0x01
#define FD_WRITE   0x02
#define FD_OOB     0x04
#define FD_ACCEPT  0x08
#define FD_CONNECT 0x10
#define FD_CLOSE   0x20

#define WSAEWOULDBLOCK   10035
#define WSAEINPROGRESS   10036
#define WSAECONNABORTED  10053
#define WSAECONNRESET    10054
#define WSAETIMEDOUT     10060
#define WSAECONNREFUSED  10061

#define WSAGETSELECTEVENT(lp)        ((WORD)((lp) & 0xFFFF))
#define WSAGETSELECTERROR(lp)        ((WORD)(((lp) >> 16) & 0xFFFF))
#define WSAMAKESELECTREPLY(ev, err)  ((long)(((DWORD)(err) << 16) | ((DWORD)(ev) & 0xFFFF)))

#ifndef MAKEWORD
#define MAKEWORD(a, b) ((WORD)(((BYTE)((a) & 0xff)) | ((WORD)((BYTE)((b) & 0xff))) << 8))
#endif

typedef struct _WSADATA { WORD wVersion; WORD wHighVersion; } WSADATA;

namespace Platform
{
	struct SocketEvent
	{
		SOCKET         socket;
		unsigned int   msg;    // o 'wMsg' passado a WSAAsyncSelect (ex.: WM_ASYNCSELECTMSG)
		unsigned short event;  // FD_READ / FD_WRITE / FD_CLOSE
		unsigned short error;  // codigo estilo Winsock (so em FD_CLOSE)
	};

	namespace detail
	{
		struct SockEntry
		{
			SOCKET       s;
			unsigned int msg;
			long         mask;
			bool         initialWrite; // Windows envia um FD_WRITE logo apos o registro
			bool         writeBlocked; // send() devolveu WOULDBLOCK: avisar quando liberar
			bool         closeReported;
		};

		struct SockState
		{
			std::mutex mtx;
			std::vector<SockEntry> entries;
		};

		inline SockState& SocketRegistry()
		{
			static SockState s;
			return s;
		}

		// errno -> codigo Winsock. EINPROGRESS vira WOULDBLOCK porque o Windows
		// devolve WSAEWOULDBLOCK num connect() nao bloqueante e o cliente testa isso.
		inline int MapErrno(int e)
		{
			if (e == EWOULDBLOCK || e == EAGAIN || e == EINPROGRESS) return WSAEWOULDBLOCK;
			switch (e)
			{
			case ECONNRESET:   return WSAECONNRESET;
			case ECONNABORTED: return WSAECONNABORTED;
			case EPIPE:        return WSAECONNABORTED;
			case ETIMEDOUT:    return WSAETIMEDOUT;
			case ECONNREFUSED: return WSAECONNREFUSED;
			default:           return e;
			}
		}

		// Chamar com o mutex travado.
		inline SockEntry* Find(SockState& st, SOCKET s)
		{
			for (SockEntry& e : st.entries)
				if (e.s == s) return &e;
			return nullptr;
		}

		inline void MarkWriteBlockedAll()
		{
			SockState& st = SocketRegistry();
			std::lock_guard<std::mutex> lock(st.mtx);
			for (SockEntry& e : st.entries) e.writeBlocked = true;
		}
	}

	// Eventos pendentes, sem bloquear. Chamar uma ou mais vezes por quadro e despachar
	// cada evento fora desta funcao (os tratadores podem chamar closesocket()).
	inline int PollSocketEvents(SocketEvent* out, int maxEvents)
	{
		using namespace detail;
		if (!out || maxEvents <= 0) return 0;

		SockState& st = SocketRegistry();
		std::lock_guard<std::mutex> lock(st.mtx);

		int n = 0;
		std::vector<pollfd> fds;
		std::vector<size_t> idx;
		for (size_t i = 0; i < st.entries.size(); ++i)
		{
			SockEntry& e = st.entries[i];
			if (e.closeReported) continue;

			if (e.initialWrite && (e.mask & FD_WRITE) && n < maxEvents)
			{
				out[n++] = { e.s, e.msg, (unsigned short)FD_WRITE, 0 };
				e.initialWrite = false;
			}

			pollfd p;
			p.fd = e.s;
			p.events = POLLIN | (e.writeBlocked ? POLLOUT : 0);
			p.revents = 0;
			fds.push_back(p);
			idx.push_back(i);
		}

		if (fds.empty()) return n;
		if (::poll(fds.data(), (nfds_t)fds.size(), 0) <= 0) return n;

		for (size_t k = 0; k < fds.size() && n < maxEvents; ++k)
		{
			SockEntry& e = st.entries[idx[k]];
			const short re = fds[k].revents;
			if (re == 0 || (re & POLLNVAL)) continue;

			if ((re & POLLOUT) && e.writeBlocked && (e.mask & FD_WRITE))
			{
				e.writeBlocked = false;
				out[n++] = { e.s, e.msg, (unsigned short)FD_WRITE, 0 };
				if (n >= maxEvents) break;
			}

			if (re & (POLLIN | POLLHUP | POLLERR))
			{
				unsigned short ev = 0, err = 0;
				if (re & POLLERR)
				{
					int soerr = 0;
					socklen_t len = sizeof(soerr);
					::getsockopt(e.s, SOL_SOCKET, SO_ERROR, &soerr, &len);
					ev = FD_CLOSE;
					err = (unsigned short)MapErrno(soerr);
				}
				else
				{
					// Espiar sem consumir: dados -> FD_READ; 0 bytes -> o outro lado fechou.
					char c;
					ssize_t r = ::recv(e.s, &c, 1, MSG_PEEK | MSG_DONTWAIT);
					if (r > 0) ev = FD_READ;
					else if (r == 0) ev = FD_CLOSE;
					else if (errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR)
					{
						ev = FD_CLOSE;
						err = (unsigned short)MapErrno(errno);
					}
				}

				if (ev == FD_READ && (e.mask & FD_READ))
					out[n++] = { e.s, e.msg, ev, 0 };
				else if (ev == FD_CLOSE && (e.mask & FD_CLOSE))
				{
					e.closeReported = true; // o Windows envia FD_CLOSE uma unica vez
					out[n++] = { e.s, e.msg, ev, err };
				}
			}
		}
		return n;
	}
}

// ---- Winsock ----
inline int WSAStartup(WORD, WSADATA* d)
{
	if (d) { d->wVersion = 0x0202; d->wHighVersion = 0x0202; }
	signal(SIGPIPE, SIG_IGN); // send() em socket fechado nao pode derrubar o app
	return 0;
}

inline int WSACleanup() { return 0; }

inline int WSAGetLastError()
{
	const int w = Platform::detail::MapErrno(errno);
	// O cliente consulta isto depois de send()/recv(); WOULDBLOCK significa que
	// o buffer de envio encheu, entao avisamos FD_WRITE quando liberar.
	if (w == WSAEWOULDBLOCK) Platform::detail::MarkWriteBlockedAll();
	return w;
}

inline int closesocket(SOCKET s)
{
	{
		auto& st = Platform::detail::SocketRegistry();
		std::lock_guard<std::mutex> lock(st.mtx);
		for (size_t i = 0; i < st.entries.size(); ++i)
			if (st.entries[i].s == s) { st.entries.erase(st.entries.begin() + i); break; }
	}
	return ::close(s);
}

// 'hWnd' e ignorado (nao ha janela); eventos saem por Platform::PollSocketEvents().
inline int WSAAsyncSelect(SOCKET s, void* /*hWnd*/, unsigned int wMsg, long lEvent)
{
	auto& st = Platform::detail::SocketRegistry();
	std::lock_guard<std::mutex> lock(st.mtx);

	Platform::detail::SockEntry* e = Platform::detail::Find(st, s);
	if (lEvent == 0) // cancelar
	{
		if (e) st.entries.erase(st.entries.begin() + (e - st.entries.data()));
		return 0;
	}

	int fl = ::fcntl(s, F_GETFL, 0);
	if (fl < 0 || ::fcntl(s, F_SETFL, fl | O_NONBLOCK) < 0) return SOCKET_ERROR; // WSAAsyncSelect torna o socket nao bloqueante

	if (!e)
	{
		st.entries.push_back({ s, wMsg, lEvent, true, false, false });
	}
	else
	{
		e->msg = wMsg; e->mask = lEvent; e->initialWrite = true; e->writeBlocked = false; e->closeReported = false;
	}
	return 0;
}

#endif // !_WIN32
