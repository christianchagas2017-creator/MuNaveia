// Teste da camada Platform fora do Windows:
//   g++ -std=c++17 -Isource -o platform_test tests/platform_test.cpp && ./platform_test
#include <cstdio>
#include <string>
#include <fstream>
#include <filesystem>
#include "Platform/PlatformCompat.h"
#include "Platform/PlatformPath.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FALHOU: %s (linha %d)\n", #c, __LINE__); ++fails; } } while (0)

int main()
{
#ifdef _WIN32
	std::printf("No Windows a camada nao se aplica.\n");
	return 0;
#else
	DWORD t0 = GetTickCount();
	Sleep(60);
	DWORD t1 = timeGetTime();
	CHECK(t1 - t0 >= 50 && t1 - t0 < 500);

	LARGE_INTEGER f, a, b;
	CHECK(QueryPerformanceFrequency(&f) && f.QuadPart > 0);
	QueryPerformanceCounter(&a);
	Sleep(20);
	QueryPerformanceCounter(&b);
	double ms = double(b.QuadPart - a.QuadPart) * 1000.0 / double(f.QuadPart);
	CHECK(ms >= 15.0 && ms < 300.0);

	CHECK(Platform::NormalizePath("Data\\Custom\\Interface\\a.ozj") == "Data/Custom/Interface/a.ozj");
	CHECK(Platform::NormalizePath("Data/ja/normal") == "Data/ja/normal");

	// ---- Arquivos: caminho estilo Windows + maiusculas/minusculas ----
	namespace fs = std::filesystem;
	fs::path tmp = fs::temp_directory_path() / ("munaveia_pt_" + std::to_string(Platform::NowNs()));
	fs::create_directories(tmp / "Data" / "Custom" / "Interface");
	{ std::ofstream(tmp / "Data" / "Custom" / "Interface" / "Hud.OZJ") << "abc"; }
	{ std::ofstream(tmp / "Data" / "Case.txt") << "UP"; }
	{ std::ofstream(tmp / "Data" / "case.txt") << "low"; }

	auto slurp = [](FILE* f) { std::string r; if (!f) return std::string("<NULL>"); char b[64]; size_t n; while ((n = std::fread(b, 1, sizeof b, f)) > 0) r.append(b, n); std::fclose(f); return r; };

	Platform::SetRootDir(tmp.string());
	CHECK(slurp(fopen("data\\custom\\interface\\hud.ozj", "rb")) == "abc");   // macro fopen + '\\' + minusculas
	CHECK(slurp(fopen("DATA/CUSTOM/INTERFACE/HUD.OZJ", "rb")) == "abc");
	CHECK(slurp(fopen("Data\\Custom\\..\\Custom\\Interface\\Hud.OZJ", "rb")) == "abc");
	CHECK(fopen("Data\\Custom\\naoexiste.ozj", "rb") == nullptr);
	CHECK(fopen("Pasta\\Inexistente\\x.ozj", "rb") == nullptr);
	CHECK(slurp(fopen("Data/Case.txt", "rb")) == "UP");     // colisao: nome exato vence
	CHECK(slurp(fopen("Data/case.txt", "rb")) == "low");

	// cria arquivo novo e le de volta com outra caixa (cache precisa ser invalidado)
	CHECK(slurp(fopen("data\\custom\\hudlayout.ini", "rb")) == "<NULL>");        // lista o diretorio em cache
	{ FILE* w = fopen("Data\\Custom\\HUDLayout.ini", "wt"); CHECK(w != nullptr); if (w) { std::fputs("x=1", w); std::fclose(w); } }
	CHECK(fs::exists(tmp / "Data" / "Custom" / "HUDLayout.ini"));
	CHECK(slurp(fopen("DATA\\CUSTOM\\hudlayout.INI", "rb")) == "x=1");

	// caminho absoluto continua funcionando
	CHECK(slurp(fopen((tmp / "Data" / "Custom" / "Interface" / "Hud.OZJ").string().c_str(), "rb")) == "abc");
	fs::remove_all(tmp);


	// ---- Sockets: WSAAsyncSelect emulado com poll() ----
	{
		using Platform::SocketEvent;
		WSADATA wsa;
		CHECK(WSAStartup(MAKEWORD(2, 2), &wsa) == 0);

		int ls = ::socket(AF_INET, SOCK_STREAM, 0);
		sockaddr_in sa{}; sa.sin_family = AF_INET; sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK); sa.sin_port = 0;
		CHECK(::bind(ls, (sockaddr*)&sa, sizeof sa) == 0);
		socklen_t sl = sizeof sa; ::getsockname(ls, (sockaddr*)&sa, &sl);
		CHECK(::listen(ls, 1) == 0);

		// mesma ordem do CWsctlc::Connect: connect() bloqueante e depois WSAAsyncSelect
		SOCKET c = socket(PF_INET, SOCK_STREAM, 0);
		CHECK(c != INVALID_SOCKET);
		CHECK(connect(c, (LPSOCKADDR)&sa, sizeof sa) == 0);
		int srv = ::accept(ls, nullptr, nullptr);
		CHECK(srv >= 0);

		const unsigned MSG = 0x400; // WM_USER+0
		CHECK(WSAAsyncSelect(c, nullptr, MSG, FD_READ | FD_WRITE | FD_CLOSE) == 0);

		auto wait = [&](SocketEvent* ev, int max) { for (int i = 0; i < 100; ++i) { int n = Platform::PollSocketEvents(ev, max); if (n) return n; Sleep(10); } return 0; };
		SocketEvent ev[8];

		// 1) Windows envia um FD_WRITE logo apos o registro
		int n = Platform::PollSocketEvents(ev, 8);
		CHECK(n == 1 && ev[0].socket == c && ev[0].msg == MSG && ev[0].event == FD_WRITE);
		CHECK(Platform::PollSocketEvents(ev, 8) == 0); // e nada mais enquanto ocioso

		// 2) dados chegando -> FD_READ; recv() consome; some o evento
		CHECK(::send(srv, "hello", 5, 0) == 5);
		n = wait(ev, 8);
		CHECK(n == 1 && ev[0].event == FD_READ && ev[0].msg == MSG);
		char buf[16] = {};
		CHECK(recv(c, buf, sizeof buf, 0) == 5 && std::string(buf, 5) == "hello");
		CHECK(Platform::PollSocketEvents(ev, 8) == 0);

		// 3) recv() sem dados -> WSAEWOULDBLOCK; avisa FD_WRITE quando o envio liberar
		CHECK(recv(c, buf, sizeof buf, 0) == SOCKET_ERROR);
		CHECK(WSAGetLastError() == WSAEWOULDBLOCK);
		n = wait(ev, 8);
		CHECK(n == 1 && ev[0].event == FD_WRITE);

		// 4) EINPROGRESS (connect nao bloqueante no POSIX) tem que virar WSAEWOULDBLOCK
		errno = EINPROGRESS;
		CHECK(WSAGetLastError() == WSAEWOULDBLOCK);
		errno = ECONNRESET;
		CHECK(WSAGetLastError() == WSAECONNRESET);
		Platform::PollSocketEvents(ev, 8); // limpa o writeBlocked deixado pelos testes acima

		// 5) outro lado envia e fecha: FD_READ primeiro, FD_CLOSE so depois de drenar, uma unica vez
		CHECK(::send(srv, "bye", 3, 0) == 3);
		::close(srv);
		n = wait(ev, 8);
		CHECK(n >= 1 && ev[0].event == FD_READ);
		CHECK(recv(c, buf, sizeof buf, 0) == 3 && std::string(buf, 3) == "bye");
		n = wait(ev, 8);
		bool closed = false; for (int i = 0; i < n; ++i) if (ev[i].event == FD_CLOSE) closed = true;
		CHECK(closed);
		Sleep(30);
		n = Platform::PollSocketEvents(ev, 8);
		for (int i = 0; i < n; ++i) CHECK(ev[i].event != FD_CLOSE); // nao repete

		// 6) closesocket() tira o socket do registro
		CHECK(closesocket(c) == 0);
		CHECK(Platform::PollSocketEvents(ev, 8) == 0);
		::close(ls);
		WSACleanup();
	}

	if (fails == 0) std::printf("OK: camada Platform passou em todos os testes.\n");
	return fails ? 1 : 0;
#endif
}
