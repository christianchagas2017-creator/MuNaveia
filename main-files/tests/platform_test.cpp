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


	// ---- Entrada: GetAsyncKeyState / cursor / toque ----
	{
		namespace In = Platform::Input;
		CHECK(VK_LBUTTON == 0x01 && VK_ESCAPE == 0x1B && VK_F1 == 0x70 && VK_OEM_3 == 0xC0 && VK_NUMPAD0 == 0x60);

		// padrao exato usado no jogo: HIBYTE(GetAsyncKeyState(VK_X)) == 128
		In::SetKey(VK_CONTROL, true);
		CHECK(HIBYTE(GetAsyncKeyState(VK_CONTROL)) == 128);
		CHECK(HIBYTE(GetAsyncKeyState(VK_CONTROL)) & 0x80);
		In::EndFrame();
		In::SetKey(VK_CONTROL, false);
		CHECK(HIBYTE(GetAsyncKeyState(VK_CONTROL)) == 0);

		// Shift generico = esquerdo OU direito
		In::SetKey(VK_RSHIFT, true);  In::EndFrame();
		CHECK(HIBYTE(GetAsyncKeyState(VK_SHIFT)) == 128);
		In::SetKey(VK_LSHIFT, true);  In::EndFrame();
		In::SetKey(VK_RSHIFT, false); In::EndFrame();
		CHECK(HIBYTE(GetAsyncKeyState(VK_SHIFT)) == 128);   // esquerdo ainda apertado
		In::SetKey(VK_LSHIFT, false); In::EndFrame();
		CHECK(HIBYTE(GetAsyncKeyState(VK_SHIFT)) == 0);

		// CapsLock alterna o bit baixo de GetKeyState
		In::SetKey(VK_CAPITAL, true);  In::EndFrame(); In::SetKey(VK_CAPITAL, false); In::EndFrame();
		CHECK((GetKeyState(VK_CAPITAL) & 1) == 1);
		In::SetKey(VK_CAPITAL, true);  In::EndFrame(); In::SetKey(VK_CAPITAL, false); In::EndFrame();
		CHECK((GetKeyState(VK_CAPITAL) & 1) == 0);

		// cursor
		POINT pt{};
		In::SetPointer(100, 200);
		CHECK(GetCursorPos(&pt) && pt.x == 100 && pt.y == 200);
		CHECK(ScreenToClient(nullptr, &pt) && pt.x == 100 && pt.y == 200);
		CHECK(GetDoubleClickTime() == 500);

		// clique rapido: apertar e soltar no MESMO quadro nao pode sumir
		In::SetKey(VK_RETURN, true);
		In::SetKey(VK_RETURN, false);
		CHECK(HIBYTE(GetAsyncKeyState(VK_RETURN)) == 128);   // o jogo ainda enxerga neste quadro
		In::EndFrame();
		CHECK(HIBYTE(GetAsyncKeyState(VK_RETURN)) == 0);     // e solta no proximo
		// ja visto num quadro anterior: soltar e imediato
		In::SetKey(VK_TAB, true);  In::EndFrame();
		In::SetKey(VK_TAB, false);
		CHECK(HIBYTE(GetAsyncKeyState(VK_TAB)) == 0);

		// roda do mouse acumula e zera
		In::AddWheel(120); In::AddWheel(120); In::AddWheel(-120);
		CHECK(In::TakeWheel() == 120 && In::TakeWheel() == 0);

		// ---- toque simples = clique esquerdo na posicao do dedo ----
		uint64_t t = 1000;
		In::TouchDown(1, 50, 60, t);
		CHECK(HIBYTE(GetAsyncKeyState(VK_LBUTTON)) == 128);
		GetCursorPos(&pt); CHECK(pt.x == 50 && pt.y == 60);
		In::EndFrame();
		In::TouchUp(1, 50, 60);
		CHECK(HIBYTE(GetAsyncKeyState(VK_LBUTTON)) == 0);

		// ---- toque rapido (down+up no mesmo quadro) ainda gera um clique visivel ----
		In::TouchDown(1, 10, 10, t);
		In::TouchUp(1, 10, 10);
		CHECK(HIBYTE(GetAsyncKeyState(VK_LBUTTON)) == 128);
		In::EndFrame();
		CHECK(HIBYTE(GetAsyncKeyState(VK_LBUTTON)) == 0);

		// ---- toque longo parado = clique direito ----
		In::TouchDown(2, 300, 300, t);
		In::EndFrame();
		In::UpdateTouch(t + 300);
		CHECK(HIBYTE(GetAsyncKeyState(VK_LBUTTON)) == 128 && HIBYTE(GetAsyncKeyState(VK_RBUTTON)) == 0);
		In::UpdateTouch(t + 500);
		CHECK(HIBYTE(GetAsyncKeyState(VK_RBUTTON)) == 128);
		In::EndFrame();
		CHECK(HIBYTE(GetAsyncKeyState(VK_LBUTTON)) == 0);
		In::TouchUp(2, 300, 300);
		CHECK(HIBYTE(GetAsyncKeyState(VK_RBUTTON)) == 0);

		// ---- arrastar cancela o toque longo ----
		In::TouchDown(3, 100, 100, t);
		In::EndFrame();
		In::TouchMove(3, 160, 100);
		In::UpdateTouch(t + 2000);
		CHECK(HIBYTE(GetAsyncKeyState(VK_RBUTTON)) == 0 && HIBYTE(GetAsyncKeyState(VK_LBUTTON)) == 128);
		GetCursorPos(&pt); CHECK(pt.x == 160 && pt.y == 100);
		In::TouchUp(3, 160, 100);
		CHECK(HIBYTE(GetAsyncKeyState(VK_LBUTTON)) == 0);

		// ---- tremida pequena (dentro do limite) NAO cancela o toque longo ----
		In::TouchDown(4, 100, 100, t);
		In::EndFrame();
		In::TouchMove(4, 105, 103);
		In::UpdateTouch(t + 600);
		CHECK(HIBYTE(GetAsyncKeyState(VK_RBUTTON)) == 128);
		In::EndFrame();
		In::TouchUp(4, 105, 103);
		CHECK(HIBYTE(GetAsyncKeyState(VK_RBUTTON)) == 0);

		// ---- segundo dedo e ignorado ----
		In::TouchDown(5, 20, 20, t);
		In::TouchDown(6, 400, 400, t);
		GetCursorPos(&pt); CHECK(pt.x == 20 && pt.y == 20);
		In::TouchMove(6, 450, 450);
		GetCursorPos(&pt); CHECK(pt.x == 20 && pt.y == 20);
		In::TouchUp(6, 450, 450);
		CHECK(HIBYTE(GetAsyncKeyState(VK_LBUTTON)) == 128);  // o dedo 5 continua apertando
		In::EndFrame();
		In::TouchUp(5, 20, 20);
		CHECK(HIBYTE(GetAsyncKeyState(VK_LBUTTON)) == 0);
	}

	if (fails == 0) std::printf("OK: camada Platform passou em todos os testes.\n");
	return fails ? 1 : 0;
#endif
}
