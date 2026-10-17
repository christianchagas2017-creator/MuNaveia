// Teste da camada Platform fora do Windows:
//   g++ -std=c++17 -Isource -o platform_test tests/platform_test.cpp && ./platform_test
#include "Platform/PlatformCompat.h"
#include "Platform/PlatformPath.h"
#include <cstdio>
#include <string>

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

	if (fails == 0) std::printf("OK: camada Platform passou em todos os testes.\n");
	return fails ? 1 : 0;
#endif
}
