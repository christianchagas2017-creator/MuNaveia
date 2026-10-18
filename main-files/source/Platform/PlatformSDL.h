#pragma once

// Adaptador SDL2: traduz eventos do SDL (janela, teclado, mouse, toque, ciclo de
// vida do app) para as camadas Platform::Input. E o equivalente, fora do Windows,
// do que o WndProc de Winmain.cpp faz hoje.
//
// Ativado com -DPLATFORM_USE_SDL (Android, iOS, Linux/macOS desktop).
// HandleEvent() recebe um SDL_Event por vez, entao da para testar sem janela.

#if defined(PLATFORM_USE_SDL) && !defined(_WIN32)

#include <SDL.h>
#include <string>
#include "PlatformCompat.h"

namespace Platform
{
	// Scancode do SDL (posicao fisica da tecla) -> VK_* do Windows. 0 = sem equivalente.
	inline int SDLScancodeToVK(SDL_Scancode sc)
	{
		if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z) return 'A' + (sc - SDL_SCANCODE_A);
		if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_9) return '1' + (sc - SDL_SCANCODE_1);
		if (sc == SDL_SCANCODE_0) return '0';
		if (sc >= SDL_SCANCODE_F1 && sc <= SDL_SCANCODE_F12) return VK_F1 + (sc - SDL_SCANCODE_F1);
		if (sc >= SDL_SCANCODE_KP_1 && sc <= SDL_SCANCODE_KP_9) return VK_NUMPAD1 + (sc - SDL_SCANCODE_KP_1);

		switch (sc)
		{
		case SDL_SCANCODE_KP_0:           return VK_NUMPAD0;
		case SDL_SCANCODE_RETURN:
		case SDL_SCANCODE_KP_ENTER:       return VK_RETURN;
		case SDL_SCANCODE_ESCAPE:
		case SDL_SCANCODE_AC_BACK:        return VK_ESCAPE;   // botao "voltar" do Android
		case SDL_SCANCODE_BACKSPACE:      return VK_BACK;
		case SDL_SCANCODE_TAB:            return VK_TAB;
		case SDL_SCANCODE_SPACE:          return VK_SPACE;
		case SDL_SCANCODE_LSHIFT:         return VK_LSHIFT;
		case SDL_SCANCODE_RSHIFT:         return VK_RSHIFT;
		case SDL_SCANCODE_LCTRL:          return VK_LCONTROL;
		case SDL_SCANCODE_RCTRL:          return VK_RCONTROL;
		case SDL_SCANCODE_LALT:           return VK_LMENU;
		case SDL_SCANCODE_RALT:           return VK_RMENU;
		case SDL_SCANCODE_LGUI:           return VK_LWIN;
		case SDL_SCANCODE_RGUI:           return VK_RWIN;
		case SDL_SCANCODE_UP:             return VK_UP;
		case SDL_SCANCODE_DOWN:           return VK_DOWN;
		case SDL_SCANCODE_LEFT:           return VK_LEFT;
		case SDL_SCANCODE_RIGHT:          return VK_RIGHT;
		case SDL_SCANCODE_PAGEUP:         return VK_PRIOR;
		case SDL_SCANCODE_PAGEDOWN:       return VK_NEXT;
		case SDL_SCANCODE_HOME:           return VK_HOME;
		case SDL_SCANCODE_END:            return VK_END;
		case SDL_SCANCODE_INSERT:         return VK_INSERT;
		case SDL_SCANCODE_DELETE:         return VK_DELETE;
		case SDL_SCANCODE_CAPSLOCK:       return VK_CAPITAL;
		case SDL_SCANCODE_NUMLOCKCLEAR:   return VK_NUMLOCK;
		case SDL_SCANCODE_SCROLLLOCK:     return VK_SCROLL;
		case SDL_SCANCODE_PAUSE:          return VK_PAUSE;
		case SDL_SCANCODE_PRINTSCREEN:    return VK_SNAPSHOT;
		case SDL_SCANCODE_HELP:           return VK_HELP;
		case SDL_SCANCODE_KP_PLUS:        return VK_ADD;
		case SDL_SCANCODE_KP_MINUS:       return VK_SUBTRACT;
		case SDL_SCANCODE_KP_MULTIPLY:    return VK_MULTIPLY;
		case SDL_SCANCODE_KP_DIVIDE:      return VK_DIVIDE;
		case SDL_SCANCODE_KP_PERIOD:      return VK_DECIMAL;
		case SDL_SCANCODE_MINUS:          return VK_OEM_MINUS;
		case SDL_SCANCODE_EQUALS:         return VK_OEM_PLUS;
		case SDL_SCANCODE_COMMA:          return VK_OEM_COMMA;
		case SDL_SCANCODE_PERIOD:         return VK_OEM_PERIOD;
		case SDL_SCANCODE_SLASH:          return VK_OEM_2;
		case SDL_SCANCODE_SEMICOLON:      return VK_OEM_1;
		case SDL_SCANCODE_GRAVE:          return VK_OEM_3;
		case SDL_SCANCODE_LEFTBRACKET:    return VK_OEM_4;
		case SDL_SCANCODE_BACKSLASH:      return VK_OEM_5;
		case SDL_SCANCODE_RIGHTBRACKET:   return VK_OEM_6;
		case SDL_SCANCODE_APOSTROPHE:     return VK_OEM_7;
		default:                          return 0;
		}
	}

	inline int SDLMouseButtonToVK(Uint8 b)
	{
		switch (b)
		{
		case SDL_BUTTON_LEFT:   return VK_LBUTTON;
		case SDL_BUTTON_RIGHT:  return VK_RBUTTON;
		case SDL_BUTTON_MIDDLE: return VK_MBUTTON;
		default:                return 0;
		}
	}

	class SDLAdapter
	{
	public:
		bool quitRequested = false;
		bool focused       = true;   // equivalente a g_bWndActive
		bool inBackground  = false;  // iOS/Android: nao chamar GL enquanto true
		int  windowW = 0, windowH = 0; // tamanho real da janela em pixels
		int  gameW   = 0, gameH   = 0; // resolucao logica do jogo (0 = sem escala)

		// Resolucao logica do jogo e tamanho da janela; as coordenadas do ponteiro
		// e do toque sao convertidas de pixels da janela para pixels do jogo.
		void SetViewport(int winW, int winH, int gW, int gH) { windowW = winW; windowH = winH; gameW = gW; gameH = gH; }

		// Texto digitado (UTF-8), equivalente ao WM_CHAR. Esvazia ao ler.
		std::string TakeText() { std::string t; t.swap(m_text); return t; }

		// Processa todos os eventos pendentes. Chamar uma vez por quadro, antes de atualizar o jogo.
		void Pump()
		{
			SDL_Event e;
			while (SDL_PollEvent(&e)) HandleEvent(e);
			Input::UpdateTouch(NowMs());
		}

		void HandleEvent(const SDL_Event& e)
		{
			switch (e.type)
			{
			case SDL_QUIT:
				quitRequested = true;
				break;

			case SDL_WINDOWEVENT:
				switch (e.window.event)
				{
				case SDL_WINDOWEVENT_SIZE_CHANGED:
				case SDL_WINDOWEVENT_RESIZED:
					windowW = e.window.data1; windowH = e.window.data2;
					break;
				case SDL_WINDOWEVENT_FOCUS_LOST:
					focused = false;
					Input::ReleaseAll(); // evita tecla "presa" ao voltar
					break;
				case SDL_WINDOWEVENT_FOCUS_GAINED:
					focused = true;
					break;
				case SDL_WINDOWEVENT_CLOSE:
					quitRequested = true;
					break;
				}
				break;

			// Ciclo de vida mobile
			case SDL_APP_WILLENTERBACKGROUND:
			case SDL_APP_DIDENTERBACKGROUND:
				inBackground = true;
				focused = false;
				Input::ReleaseAll();
				break;
			case SDL_APP_DIDENTERFOREGROUND:
			case SDL_APP_WILLENTERFOREGROUND:
				inBackground = false;
				focused = true;
				break;
			case SDL_APP_TERMINATING:
				quitRequested = true;
				break;

			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if (e.key.repeat) break; // o jogo detecta "segurando" sozinho
				{
					int vk = SDLScancodeToVK(e.key.keysym.scancode);
					if (vk) Input::SetKey(vk, e.type == SDL_KEYDOWN);
				}
				break;

			case SDL_TEXTINPUT:
				m_text += e.text.text;
				break;

			// Mouse "de verdade". Eventos de mouse sintetizados a partir do toque
			// (which == SDL_TOUCH_MOUSEID) sao ignorados: o toque e tratado abaixo.
			case SDL_MOUSEMOTION:
				if (e.motion.which == SDL_TOUCH_MOUSEID) break;
				SetPointerScaled(e.motion.x, e.motion.y);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				if (e.button.which == SDL_TOUCH_MOUSEID) break;
				SetPointerScaled(e.button.x, e.button.y);
				{
					int vk = SDLMouseButtonToVK(e.button.button);
					if (vk) Input::SetKey(vk, e.type == SDL_MOUSEBUTTONDOWN);
				}
				break;
			case SDL_MOUSEWHEEL:
				if (e.wheel.which == SDL_TOUCH_MOUSEID) break;
				{
					int y = e.wheel.y;
					if (e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) y = -y;
					Input::AddWheel(y * 120); // 120 = um "clique", como WM_MOUSEWHEEL
				}
				break;

			// Toque: coordenadas normalizadas (0..1) -> pixels do jogo.
			case SDL_FINGERDOWN:
				{
					int x, y; FingerToGame(e.tfinger, x, y);
					Input::TouchDown((int)e.tfinger.fingerId, x, y, NowMs());
				}
				break;
			case SDL_FINGERMOTION:
				{
					int x, y; FingerToGame(e.tfinger, x, y);
					Input::TouchMove((int)e.tfinger.fingerId, x, y);
				}
				break;
			case SDL_FINGERUP:
				{
					int x, y; FingerToGame(e.tfinger, x, y);
					Input::TouchUp((int)e.tfinger.fingerId, x, y);
				}
				break;

			default:
				break;
			}
		}

	private:
		std::string m_text;

		void SetPointerScaled(int x, int y)
		{
			if (windowW > 0 && gameW > 0) x = (int)((long long)x * gameW / windowW);
			if (windowH > 0 && gameH > 0) y = (int)((long long)y * gameH / windowH);
			Input::SetPointer(x, y);
		}

		void FingerToGame(const SDL_TouchFingerEvent& f, int& x, int& y) const
		{
			const int w = gameW > 0 ? gameW : windowW;
			const int h = gameH > 0 ? gameH : windowH;
			x = (int)(f.x * (float)w);
			y = (int)(f.y * (float)h);
		}
	};
}

#endif // PLATFORM_USE_SDL && !_WIN32
