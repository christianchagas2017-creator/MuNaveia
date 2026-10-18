// Teste do adaptador SDL (sem janela; usa eventos sinteticos e a fila real do SDL):
//   g++ -std=c++17 -DPLATFORM_USE_SDL -Isource -I<SDL>/include tests/sdl_adapter_test.cpp <SDL>/libSDL2.a -lm -ldl -lpthread
#include <cstdio>
#include <cstring>
#include <string>
#include "Platform/PlatformCompat.h"
#include "Platform/PlatformSDL.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FALHOU: %s (linha %d)\n", #c, __LINE__); ++fails; } } while (0)
#define DOWN(vk) (HIBYTE(GetAsyncKeyState(vk)) == 128)

using namespace Platform;
namespace In = Platform::Input;

static SDL_Event Key(SDL_Scancode sc, bool down, int repeat = 0)
{
	SDL_Event e; std::memset(&e, 0, sizeof e);
	e.type = down ? SDL_KEYDOWN : SDL_KEYUP;
	e.key.keysym.scancode = sc; e.key.repeat = (Uint8)repeat;
	return e;
}
static SDL_Event Motion(int x, int y, Uint32 which = 0)
{
	SDL_Event e; std::memset(&e, 0, sizeof e);
	e.type = SDL_MOUSEMOTION; e.motion.x = x; e.motion.y = y; e.motion.which = which;
	return e;
}
static SDL_Event Button(Uint8 b, bool down, int x, int y, Uint32 which = 0)
{
	SDL_Event e; std::memset(&e, 0, sizeof e);
	e.type = down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
	e.button.button = b; e.button.x = x; e.button.y = y; e.button.which = which;
	return e;
}
static SDL_Event Finger(Uint32 type, long long id, float x, float y)
{
	SDL_Event e; std::memset(&e, 0, sizeof e);
	e.type = type; e.tfinger.fingerId = id; e.tfinger.x = x; e.tfinger.y = y;
	return e;
}
static SDL_Event Win(int ev, int d1 = 0, int d2 = 0)
{
	SDL_Event e; std::memset(&e, 0, sizeof e);
	e.type = SDL_WINDOWEVENT; e.window.event = (Uint8)ev; e.window.data1 = d1; e.window.data2 = d2;
	return e;
}
static void Reset() { In::ReleaseAll(); In::EndFrame(); }

int main()
{
	// ---- tabela de teclas ----
	CHECK(SDLScancodeToVK(SDL_SCANCODE_A) == 'A' && SDLScancodeToVK(SDL_SCANCODE_Z) == 'Z');
	CHECK(SDLScancodeToVK(SDL_SCANCODE_1) == '1' && SDLScancodeToVK(SDL_SCANCODE_9) == '9' && SDLScancodeToVK(SDL_SCANCODE_0) == '0');
	CHECK(SDLScancodeToVK(SDL_SCANCODE_F1) == VK_F1 && SDLScancodeToVK(SDL_SCANCODE_F12) == VK_F12);
	CHECK(SDLScancodeToVK(SDL_SCANCODE_UP) == VK_UP && SDLScancodeToVK(SDL_SCANCODE_PAGEDOWN) == VK_NEXT);
	CHECK(SDLScancodeToVK(SDL_SCANCODE_RSHIFT) == VK_RSHIFT && SDLScancodeToVK(SDL_SCANCODE_LCTRL) == VK_LCONTROL);
	CHECK(SDLScancodeToVK(SDL_SCANCODE_KP_0) == VK_NUMPAD0 && SDLScancodeToVK(SDL_SCANCODE_KP_5) == VK_NUMPAD5);
	CHECK(SDLScancodeToVK(SDL_SCANCODE_GRAVE) == VK_OEM_3 && SDLScancodeToVK(SDL_SCANCODE_EQUALS) == VK_OEM_PLUS);
	CHECK(SDLScancodeToVK(SDL_SCANCODE_AC_BACK) == VK_ESCAPE);  // botao voltar do Android
	CHECK(SDLScancodeToVK(SDL_SCANCODE_UNKNOWN) == 0);
	CHECK(SDLMouseButtonToVK(SDL_BUTTON_LEFT) == VK_LBUTTON && SDLMouseButtonToVK(SDL_BUTTON_RIGHT) == VK_RBUTTON && SDLMouseButtonToVK(SDL_BUTTON_MIDDLE) == VK_MBUTTON);

	SDLAdapter a;
	a.SetViewport(2560, 1440, 1280, 720);

	// ---- teclado ----
	a.HandleEvent(Key(SDL_SCANCODE_A, true));
	CHECK(DOWN('A'));
	a.HandleEvent(Key(SDL_SCANCODE_A, true, 1));   // auto-repeat nao muda nada
	CHECK(DOWN('A'));
	In::EndFrame();
	a.HandleEvent(Key(SDL_SCANCODE_A, false));
	CHECK(!DOWN('A'));
	Reset();

	// auto-repeat que chega DEPOIS de soltar nao pode gerar um clique fantasma
	a.HandleEvent(Key(SDL_SCANCODE_Q, true));
	a.HandleEvent(Key(SDL_SCANCODE_Q, false));      // apertou e soltou no mesmo quadro
	In::EndFrame();
	CHECK(!DOWN('Q'));
	a.HandleEvent(Key(SDL_SCANCODE_Q, true, 1));    // repeticao tardia do sistema
	CHECK(!DOWN('Q'));
	Reset();

	a.HandleEvent(Key(SDL_SCANCODE_RSHIFT, true)); In::EndFrame();
	CHECK(DOWN(VK_SHIFT));
	a.HandleEvent(Key(SDL_SCANCODE_RSHIFT, false)); In::EndFrame();
	CHECK(!DOWN(VK_SHIFT));
	a.HandleEvent(Key(SDL_SCANCODE_AC_BACK, true)); In::EndFrame();
	CHECK(DOWN(VK_ESCAPE));
	Reset();

	// ---- mouse com escala janela -> jogo ----
	POINT pt{};
	a.HandleEvent(Motion(1280, 720));
	GetCursorPos(&pt); CHECK(pt.x == 640 && pt.y == 360);
	a.HandleEvent(Button(SDL_BUTTON_LEFT, true, 2560, 1440));
	CHECK(DOWN(VK_LBUTTON));
	GetCursorPos(&pt); CHECK(pt.x == 1280 && pt.y == 720);
	In::EndFrame();
	a.HandleEvent(Button(SDL_BUTTON_LEFT, false, 2560, 1440));
	CHECK(!DOWN(VK_LBUTTON));
	Reset();

	// mouse sintetizado pelo toque deve ser ignorado (o toque e tratado a parte)
	a.HandleEvent(Motion(100, 100, SDL_TOUCH_MOUSEID));
	a.HandleEvent(Button(SDL_BUTTON_LEFT, true, 100, 100, SDL_TOUCH_MOUSEID));
	CHECK(!DOWN(VK_LBUTTON));
	GetCursorPos(&pt); CHECK(!(pt.x == 50 && pt.y == 50)); // nao virou a posicao do mouse sintetico escalada

	// roda
	In::TakeWheel();
	{ SDL_Event e; std::memset(&e, 0, sizeof e); e.type = SDL_MOUSEWHEEL; e.wheel.y = 2; a.HandleEvent(e); }
	CHECK(In::TakeWheel() == 240);
	{ SDL_Event e; std::memset(&e, 0, sizeof e); e.type = SDL_MOUSEWHEEL; e.wheel.y = 1; e.wheel.direction = SDL_MOUSEWHEEL_FLIPPED; a.HandleEvent(e); }
	CHECK(In::TakeWheel() == -120);

	// ---- toque (coordenadas normalizadas -> pixels do jogo 1280x720) ----
	a.HandleEvent(Finger(SDL_FINGERDOWN, 7, 0.5f, 0.5f));
	CHECK(DOWN(VK_LBUTTON));
	GetCursorPos(&pt); CHECK(pt.x == 640 && pt.y == 360);
	In::EndFrame();
	a.HandleEvent(Finger(SDL_FINGERMOTION, 7, 0.75f, 0.25f));
	GetCursorPos(&pt); CHECK(pt.x == 960 && pt.y == 180);
	a.HandleEvent(Finger(SDL_FINGERUP, 7, 0.75f, 0.25f));
	CHECK(!DOWN(VK_LBUTTON));
	Reset();

	// ---- perda de foco solta tudo (nada de tecla presa) ----
	a.HandleEvent(Key(SDL_SCANCODE_W, true)); In::EndFrame();
	CHECK(DOWN('W') && a.focused);
	a.HandleEvent(Win(SDL_WINDOWEVENT_FOCUS_LOST));
	CHECK(!DOWN('W') && !a.focused);
	a.HandleEvent(Win(SDL_WINDOWEVENT_FOCUS_GAINED));
	CHECK(a.focused);

	// ---- ciclo de vida mobile ----
	{ SDL_Event e; std::memset(&e, 0, sizeof e); e.type = SDL_APP_DIDENTERBACKGROUND; a.HandleEvent(e); }
	CHECK(a.inBackground && !a.focused);
	{ SDL_Event e; std::memset(&e, 0, sizeof e); e.type = SDL_APP_DIDENTERFOREGROUND; a.HandleEvent(e); }
	CHECK(!a.inBackground && a.focused);

	// ---- janela, texto, sair ----
	a.HandleEvent(Win(SDL_WINDOWEVENT_SIZE_CHANGED, 1920, 1080));
	CHECK(a.windowW == 1920 && a.windowH == 1080);
	{ SDL_Event e; std::memset(&e, 0, sizeof e); e.type = SDL_TEXTINPUT; std::strcpy(e.text.text, "ol"); a.HandleEvent(e); std::strcpy(e.text.text, "a"); a.HandleEvent(e); }
	CHECK(a.TakeText() == "ola" && a.TakeText().empty());
	CHECK(!a.quitRequested);
	{ SDL_Event e; std::memset(&e, 0, sizeof e); e.type = SDL_QUIT; a.HandleEvent(e); }
	CHECK(a.quitRequested);

	// ---- caminho real: fila do SDL + Pump() + toque longo com tempo real ----
	CHECK(SDL_Init(SDL_INIT_EVENTS) == 0);
	{
		SDLAdapter b; b.SetViewport(1000, 500, 1000, 500);
		Reset();
		SDL_Event e = Finger(SDL_FINGERDOWN, 9, 0.2f, 0.4f);
		CHECK(SDL_PushEvent(&e) == 1);
		b.Pump();
		CHECK(DOWN(VK_LBUTTON) && !DOWN(VK_RBUTTON));
		GetCursorPos(&pt); CHECK(pt.x == 200 && pt.y == 200);
		In::EndFrame();
		SDL_Delay(520);
		b.Pump();                                   // passou de 450 ms parado: vira clique direito
		CHECK(DOWN(VK_RBUTTON) && !DOWN(VK_LBUTTON));
		In::EndFrame();
		e = Finger(SDL_FINGERUP, 9, 0.2f, 0.4f);
		SDL_PushEvent(&e); b.Pump();
		CHECK(!DOWN(VK_RBUTTON));
	}
	SDL_Quit();

	if (fails == 0) std::printf("OK: adaptador SDL passou em todos os testes.\n");
	return fails ? 1 : 0;
}
