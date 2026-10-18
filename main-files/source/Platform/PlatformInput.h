#pragma once

// Entrada (teclado, mouse, toque) do cliente fora do Windows.
//
// O jogo le TODO o teclado e os botoes do mouse por GetAsyncKeyState() (varredura
// de 256 teclas por quadro em CNewKeyInput::ScanAsyncKeyState) e o cursor por
// GetCursorPos()/ScreenToClient(). Aqui essas funcoes leem uma tabela de estado
// que o adaptador de janela (SDL, Android, iOS) alimenta com Platform::Input::*.
// Nenhum dos ~95 arquivos que usam VK_* precisa mudar.
//
// No Windows este arquivo nao faz nada.

#ifndef _WIN32

#include <stdint.h>
#include <mutex>
#include "PlatformCompat.h"

#ifndef _PLATFORM_COMPAT_GEOM_TYPES
#define _PLATFORM_COMPAT_GEOM_TYPES
typedef int32_t  LONG;
typedef int16_t  SHORT;
typedef uint16_t USHORT;
typedef struct tagPOINT { LONG x; LONG y; } POINT;
typedef struct tagSIZE  { LONG cx; LONG cy; } SIZE;
typedef struct tagRECT  { LONG left; LONG top; LONG right; LONG bottom; } RECT;
#endif

#ifndef HIBYTE
#define LOBYTE(w)       ((BYTE)(((DWORD)(w)) & 0xff))
#define HIBYTE(w)       ((BYTE)((((DWORD)(w)) >> 8) & 0xff))
#define LOWORD(l)       ((WORD)(((DWORD)(l)) & 0xffff))
#define HIWORD(l)       ((WORD)((((DWORD)(l)) >> 16) & 0xffff))
#define MAKELONG(a, b)  ((LONG)(((WORD)((DWORD)(a) & 0xffff)) | ((DWORD)((WORD)((DWORD)(b) & 0xffff))) << 16))
#endif

// ---- Codigos de tecla virtual (mesmos valores do Windows) ----
#define VK_LBUTTON   0x01
#define VK_RBUTTON   0x02
#define VK_CANCEL    0x03
#define VK_MBUTTON   0x04
#define VK_BACK      0x08
#define VK_TAB       0x09
#define VK_CLEAR     0x0C
#define VK_RETURN    0x0D
#define VK_SHIFT     0x10
#define VK_CONTROL   0x11
#define VK_MENU      0x12
#define VK_PAUSE     0x13
#define VK_CAPITAL   0x14
#define VK_ESCAPE    0x1B
#define VK_SPACE     0x20
#define VK_PRIOR     0x21
#define VK_NEXT      0x22
#define VK_END       0x23
#define VK_HOME      0x24
#define VK_LEFT      0x25
#define VK_UP        0x26
#define VK_RIGHT     0x27
#define VK_DOWN      0x28
#define VK_SNAPSHOT  0x2C
#define VK_INSERT    0x2D
#define VK_DELETE    0x2E
#define VK_HELP      0x2F
#define VK_LWIN      0x5B
#define VK_RWIN      0x5C
#define VK_NUMPAD0   0x60
#define VK_NUMPAD1   0x61
#define VK_NUMPAD2   0x62
#define VK_NUMPAD3   0x63
#define VK_NUMPAD4   0x64
#define VK_NUMPAD5   0x65
#define VK_NUMPAD6   0x66
#define VK_NUMPAD7   0x67
#define VK_NUMPAD8   0x68
#define VK_NUMPAD9   0x69
#define VK_MULTIPLY  0x6A
#define VK_ADD       0x6B
#define VK_SUBTRACT  0x6D
#define VK_DECIMAL   0x6E
#define VK_DIVIDE    0x6F
#define VK_F1        0x70
#define VK_F2        0x71
#define VK_F3        0x72
#define VK_F4        0x73
#define VK_F5        0x74
#define VK_F6        0x75
#define VK_F7        0x76
#define VK_F8        0x77
#define VK_F9        0x78
#define VK_F10       0x79
#define VK_F11       0x7A
#define VK_F12       0x7B
#define VK_F15       0x7E
#define VK_NUMLOCK   0x90
#define VK_SCROLL    0x91
#define VK_LSHIFT    0xA0
#define VK_RSHIFT    0xA1
#define VK_LCONTROL  0xA2
#define VK_RCONTROL  0xA3
#define VK_LMENU     0xA4
#define VK_RMENU     0xA5
#define VK_OEM_1       0xBA
#define VK_OEM_PLUS    0xBB
#define VK_OEM_COMMA   0xBC
#define VK_OEM_MINUS   0xBD
#define VK_OEM_PERIOD  0xBE
#define VK_OEM_2       0xBF
#define VK_OEM_3       0xC0
#define VK_OEM_4       0xDB
#define VK_OEM_5       0xDC
#define VK_OEM_6       0xDD
#define VK_OEM_7       0xDE

namespace Platform
{
	namespace Input
	{
		namespace detail
		{
			struct KeyInfo
			{
				bool down = false;
				bool toggled = false;        // CAPS/NUM/SCROLL
				bool pressedThisFrame = false;
				bool releasePending = false; // soltou no mesmo quadro em que apertou: segura ate EndFrame()
			};

			struct State
			{
				std::mutex mtx;
				KeyInfo keys[256];
				int ptrX = 0, ptrY = 0;
				int wheel = 0;
				bool cursorVisible = true;

				// Gesto de toque -> mouse
				int  touchId = -1;
				int  touchStartX = 0, touchStartY = 0;
				uint64_t touchStartMs = 0;
				bool touchMoved = false;
				bool touchLongPressed = false;
				int  touchButton = 0;        // VK_LBUTTON ou VK_RBUTTON enquanto o dedo esta na tela
			};

			inline State& S()
			{
				static State s;
				return s;
			}

			// VK_SHIFT/VK_CONTROL/VK_MENU sao sempre derivadas de esquerda OU direita, como no Windows.
			inline void RefreshGeneric(State& s)
			{
				s.keys[VK_SHIFT].down   = s.keys[VK_LSHIFT].down   || s.keys[VK_RSHIFT].down;
				s.keys[VK_CONTROL].down = s.keys[VK_LCONTROL].down || s.keys[VK_RCONTROL].down;
				s.keys[VK_MENU].down    = s.keys[VK_LMENU].down    || s.keys[VK_RMENU].down;
			}

			// Chamar com o mutex travado.
			inline void SetKeyLocked(State& s, int vk, bool down)
			{
				if (vk <= 0 || vk > 255) return;
				// Definir a tecla generica equivale a definir a da esquerda.
				if (vk == VK_SHIFT) vk = VK_LSHIFT;
				else if (vk == VK_CONTROL) vk = VK_LCONTROL;
				else if (vk == VK_MENU) vk = VK_LMENU;
				KeyInfo& k = s.keys[vk];
				if (down)
				{
					if (!k.down)
					{
						k.down = true;
						k.pressedThisFrame = true;
						if (vk == VK_CAPITAL || vk == VK_NUMLOCK || vk == VK_SCROLL) k.toggled = !k.toggled;
					}
					k.releasePending = false;
				}
				else
				{
					// Um clique rapido (apertar e soltar entre dois quadros) nao pode sumir:
					// o jogo so olha o estado uma vez por quadro.
					if (k.pressedThisFrame) k.releasePending = true;
					else k.down = false;
				}
				RefreshGeneric(s);
			}
		}

		// ---- Alimentados pelo adaptador de janela ----
		inline void SetKey(int vk, bool down)
		{
			auto& s = detail::S(); std::lock_guard<std::mutex> l(s.mtx);
			detail::SetKeyLocked(s, vk, down);
		}

		inline void SetPointer(int x, int y)
		{
			auto& s = detail::S(); std::lock_guard<std::mutex> l(s.mtx);
			s.ptrX = x; s.ptrY = y;
		}

		inline void AddWheel(int delta)
		{
			auto& s = detail::S(); std::lock_guard<std::mutex> l(s.mtx);
			s.wheel += delta;
		}

		// Delta acumulado da roda desde a ultima chamada (120 = um "clique", como WM_MOUSEWHEEL).
		inline int TakeWheel()
		{
			auto& s = detail::S(); std::lock_guard<std::mutex> l(s.mtx);
			int w = s.wheel; s.wheel = 0; return w;
		}

		// Solta tudo (perda de foco, app indo para segundo plano) para nao deixar tecla "presa".
		inline void ReleaseAll()
		{
			auto& s = detail::S(); std::lock_guard<std::mutex> l(s.mtx);
			for (int i = 0; i < 256; ++i)
			{
				s.keys[i].down = false;
				s.keys[i].pressedThisFrame = false;
				s.keys[i].releasePending = false;
			}
			s.touchId = -1; s.touchButton = 0;
			s.touchMoved = false; s.touchLongPressed = false;
		}

		// Chamar UMA vez por quadro, depois de atualizar o jogo.
		inline void EndFrame()
		{
			auto& s = detail::S(); std::lock_guard<std::mutex> l(s.mtx);
			for (int i = 0; i < 256; ++i)
			{
				detail::KeyInfo& k = s.keys[i];
				k.pressedThisFrame = false;
				if (k.releasePending) { k.down = false; k.releasePending = false; }
			}
			detail::RefreshGeneric(s);
		}

		// ---- Toque -> mouse ----
		// 1 dedo: toque = clique esquerdo; arrastar = arrastar com o esquerdo;
		// segurar parado >= kLongPressMs = clique direito (o jogo usa o direito para skills).
		// Dedos extras sao ignorados por enquanto.
		static const int      kTouchSlopPx = 12;
		static const uint64_t kLongPressMs = 450;

		inline void TouchDown(int id, int x, int y, uint64_t nowMs)
		{
			auto& s = detail::S(); std::lock_guard<std::mutex> l(s.mtx);
			if (s.touchId != -1) return; // ja ha um dedo ativo
			s.touchId = id;
			s.touchStartX = x; s.touchStartY = y; s.touchStartMs = nowMs;
			s.touchMoved = false; s.touchLongPressed = false;
			s.ptrX = x; s.ptrY = y;
			s.touchButton = VK_LBUTTON;
			detail::SetKeyLocked(s, VK_LBUTTON, true);
		}

		inline void TouchMove(int id, int x, int y)
		{
			auto& s = detail::S(); std::lock_guard<std::mutex> l(s.mtx);
			if (id != s.touchId) return;
			s.ptrX = x; s.ptrY = y;
			int dx = x - s.touchStartX, dy = y - s.touchStartY;
			if (dx * dx + dy * dy > kTouchSlopPx * kTouchSlopPx) s.touchMoved = true;
		}

		inline void TouchUp(int id, int x, int y)
		{
			auto& s = detail::S(); std::lock_guard<std::mutex> l(s.mtx);
			if (id != s.touchId) return;
			s.ptrX = x; s.ptrY = y;
			detail::SetKeyLocked(s, s.touchButton, false);
			s.touchId = -1; s.touchButton = 0;
		}

		// Chamar a cada quadro (antes de atualizar o jogo) para detectar o toque longo.
		inline void UpdateTouch(uint64_t nowMs)
		{
			auto& s = detail::S(); std::lock_guard<std::mutex> l(s.mtx);
			if (s.touchId == -1 || s.touchMoved || s.touchLongPressed) return;
			if (nowMs - s.touchStartMs >= kLongPressMs)
			{
				s.touchLongPressed = true;
				detail::SetKeyLocked(s, VK_LBUTTON, false);
				detail::SetKeyLocked(s, VK_RBUTTON, true);
				s.touchButton = VK_RBUTTON;
			}
		}
	}
}

// ---- API do Windows usada pelo cliente ----
inline SHORT GetAsyncKeyState(int vk)
{
	if (vk < 0 || vk > 255) return 0;
	auto& s = Platform::Input::detail::S(); std::lock_guard<std::mutex> l(s.mtx);
	return s.keys[vk].down ? (SHORT)0x8000 : (SHORT)0;
}

// Bit alto = apertada agora; bit baixo = ligada (CapsLock/NumLock/ScrollLock).
inline SHORT GetKeyState(int vk)
{
	if (vk < 0 || vk > 255) return 0;
	auto& s = Platform::Input::detail::S(); std::lock_guard<std::mutex> l(s.mtx);
	return (SHORT)((s.keys[vk].down ? 0x8000 : 0) | (s.keys[vk].toggled ? 1 : 0));
}

// Coordenadas de tela == coordenadas da janela (o adaptador ja entrega o ponteiro relativo a janela).
inline BOOL GetCursorPos(POINT* p)
{
	if (!p) return FALSE;
	auto& s = Platform::Input::detail::S(); std::lock_guard<std::mutex> l(s.mtx);
	p->x = s.ptrX; p->y = s.ptrY;
	return TRUE;
}

inline BOOL SetCursorPos(int x, int y) { Platform::Input::SetPointer(x, y); return TRUE; }
inline BOOL ScreenToClient(void*, POINT*) { return TRUE; }
inline BOOL ClientToScreen(void*, POINT*) { return TRUE; }
inline BOOL ClipCursor(const RECT*) { return TRUE; }

inline int ShowCursor(BOOL show)
{
	auto& s = Platform::Input::detail::S(); std::lock_guard<std::mutex> l(s.mtx);
	s.cursorVisible = (show != 0);
	return s.cursorVisible ? 0 : -1;
}

inline unsigned int GetDoubleClickTime() { return 500; }

#endif // !_WIN32
