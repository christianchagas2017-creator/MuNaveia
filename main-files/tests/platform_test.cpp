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

	if (fails == 0) std::printf("OK: camada Platform passou em todos os testes.\n");
	return fails ? 1 : 0;
#endif
}
