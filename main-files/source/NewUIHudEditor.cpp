//////////////////////////////////////////////////////////////////////
// NewUIHudEditor.cpp
//newhudvictoriuri - Editor visual da HUD aberto pelo comando /hud.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "NewUIHudEditor.h"
#include "../Custom/HUDLayout.generated.h" //newhudvictoriuri - valores de reserva do layout enviado.
#if CLIENT_VERSION >= 1001
#include "NewUICommon.h"
#include "ZzzOpenglUtil.h"
#include "UIControls.h"
#include "Winmain.h"
#include "WideScreen.h"

#include <math.h>
#include <locale.h> //newhudvictoriuri - grava numeros do HUD sempre com ponto decimal.
#include <string.h>

namespace
{
	struct HUD_LAYOUT_ITEM
	{
		char id[32];
		const char* label;
		float x;
		float y;
		float w;
		float h;
		float scale;
		bool visible;
	};

	//newhudvictoriuri - Os valores sao coordenadas do PSD 1920x1080.
	HUD_LAYOUT_ITEM g_HudLayout[SEASON3B::HUD_EDITOR_COUNT];
	bool g_HudLayoutInitialized = false;
	bool g_HudEditorOpen = false;
	bool g_HudEditorDragging = false;
	bool g_HudEditorLayoutChanged = false;
	bool g_HudEditorMoveWholeLayout = true; //newhudvictoriuri - modo HUD inteira ou somente item.
	int g_HudEditorSelected = -1;
	int g_HudEditorDragMode = 0; // 1 = mover, 2 = redimensionar.
	float g_HudEditorDragStartX = 0.0f;
	float g_HudEditorDragStartY = 0.0f;
	float g_HudEditorItemStartX = 0.0f;
	float g_HudEditorItemStartY = 0.0f;
	float g_HudEditorItemStartW = 0.0f;
	float g_HudEditorItemStartH = 0.0f;
	char g_HudEditorStatus[128] = "Digite /hud para editar a HUD";
	DWORD g_HudEditorStatusUntil = 0;

	const char* const g_HudLayoutPath = "Custom\\HUDLayout.ini";
	const char* const g_HudGeneratedPath = "Custom\\HUDLayout.generated.h";

	float ClampLayoutValue(float value, float low, float high)
	{
		if (value < low) return low;
		if (value > high) return high;
		return value;
	}

	bool IsValidElement(int element)
	{
		return element >= 0 && element < SEASON3B::HUD_EDITOR_COUNT;
	}

	void ResetLayoutInternal()
	{
		//newhudvictoriuri - A source nao duplica coordenadas: o header enviado
		// e o HUDLayout.ini sao a unica origem do layout da HUD.
		for (int i = 0; i < SEASON3B::HUD_EDITOR_COUNT; ++i)
		{
			const NEW_HUD_EDITOR_GENERATED_ITEM& generated = g_NewHudEditorGeneratedLayout[i];
			strncpy(g_HudLayout[i].id, generated.id, sizeof(g_HudLayout[i].id) - 1);
			g_HudLayout[i].id[sizeof(g_HudLayout[i].id) - 1] = '\0';
			g_HudLayout[i].label = generated.id;
			g_HudLayout[i].x = generated.x;
			g_HudLayout[i].y = generated.y;
			g_HudLayout[i].w = generated.w;
			g_HudLayout[i].h = generated.h;
			g_HudLayout[i].scale = generated.scale;
			g_HudLayout[i].visible = (generated.visible != 0);
		}
	}

	void SetStatus(const char* text)
	{
		strncpy(g_HudEditorStatus, text, sizeof(g_HudEditorStatus) - 1);
		g_HudEditorStatus[sizeof(g_HudEditorStatus) - 1] = '\0';
		g_HudEditorStatusUntil = GetTickCount() + 3500;
	}

	void MarkLayoutChanged()
	{
		g_HudEditorLayoutChanged = true;
		SetStatus("Alterado - clique em SALVAR");
	}

	HUD_LAYOUT_ITEM* FindLayoutItem(const char* id)
	{
		if (id == NULL) return NULL;
		for (int i = 0; i < SEASON3B::HUD_EDITOR_COUNT; ++i)
		{
			if (stricmp(g_HudLayout[i].id, id) == 0)
				return &g_HudLayout[i];
		}
		return NULL;
	}

	void LoadFile()
	{
		FILE* file = fopen(g_HudLayoutPath, "rt");
		if (file == NULL) return;

		char line[512];
		while (fgets(line, sizeof(line), file) != NULL)
		{
			if (line[0] == '#' || line[0] == ';' || line[0] == '[' || line[0] == '\n' || line[0] == '\r')
				continue;

			char id[64] = { 0 };
			float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f, scale = 1.0f;
			int visible = 1;
			if (sscanf(line, "%63[^=]=%f,%f,%f,%f,%f,%d", id, &x, &y, &w, &h, &scale, &visible) != 6)
				continue;

			HUD_LAYOUT_ITEM* item = FindLayoutItem(id);
			if (item == NULL) continue;
			item->x = ClampLayoutValue(x, -1920.0f, 3840.0f);
			item->y = ClampLayoutValue(y, -1080.0f, 2160.0f);
			item->w = ClampLayoutValue(w, 1.0f, 3840.0f);
			item->h = ClampLayoutValue(h, 1.0f, 2160.0f);
			item->scale = ClampLayoutValue(scale, 0.25f, 4.0f);
			item->visible = (visible != 0);
		}

		fclose(file);
	}

	void EnsureLayoutLoaded()
	{
		if (!g_HudLayoutInitialized)
		{
			SEASON3B::NewHudEditorLoad();
		}
	}

	//newhudvictoriuri - O separador do arquivo e virgula; por isso o decimal
	//precisa ser sempre ponto, mesmo em Windows com locale pt-BR.
	class HudNumericLocaleGuard
	{
		char m_Previous[64];

	public:
		HudNumericLocaleGuard()
		{
			memset(m_Previous, 0, sizeof(m_Previous));
			const char* current = setlocale(LC_NUMERIC, NULL);
			if (current != NULL)
			{
				strncpy(m_Previous, current, sizeof(m_Previous) - 1);
				m_Previous[sizeof(m_Previous) - 1] = '\0';
			}
			setlocale(LC_NUMERIC, "C");
		}

		~HudNumericLocaleGuard()
		{
			if (m_Previous[0] != '\0')
				setlocale(LC_NUMERIC, m_Previous);
		}
	};

	void WriteGeneratedHeader()
	{
		HudNumericLocaleGuard numericLocale; //newhudvictoriuri
		FILE* file = fopen(g_HudGeneratedPath, "wt");
		if (file == NULL) return;

		fprintf(file, "//newhudvictoriuri - Gerado pelo comando /hud.\n");
		fprintf(file, "// Header de reserva do layout; o cliente prioriza Custom\\HUDLayout.ini.\n");
		fprintf(file, "#pragma once\n\n");
		fprintf(file, "struct NEW_HUD_EDITOR_GENERATED_ITEM { const char* id; float x; float y; float w; float h; float scale; int visible; };\n");
		fprintf(file, "static const NEW_HUD_EDITOR_GENERATED_ITEM g_NewHudEditorGeneratedLayout[] =\n{\n");
		for (int i = 0; i < SEASON3B::HUD_EDITOR_COUNT; ++i)
		{
			fprintf(file, "    { \"%s\", %.2f, %.2f, %.2f, %.2f, %.2f, %d }, //newhudvictoriuri\n",
				g_HudLayout[i].id, g_HudLayout[i].x, g_HudLayout[i].y,
				g_HudLayout[i].w, g_HudLayout[i].h, g_HudLayout[i].scale,
				g_HudLayout[i].visible ? 1 : 0);
		}
		fprintf(file, "};\n");
		fclose(file);
	}

	float ScreenToLayoutX(float screenX)
	{
		const float scale = NewHudVictoriuriScale();
		if (scale <= 0.0001f) return screenX;
		const float offsetX = (gWideScreen.g_WideWindowWidth - (1920.0f * scale)) * 0.5f;
		return (screenX - offsetX) / scale;
	}

	float ScreenToLayoutY(float screenY)
	{
		const float scale = NewHudVictoriuriScale();
		if (scale <= 0.0001f) return screenY;
		const float offsetY = gWideScreen.g_WideWindowHeight - (1080.0f * scale);
		return (screenY - offsetY) / scale;
	}

	void GetScreenRect(const HUD_LAYOUT_ITEM& item, float& x, float& y, float& w, float& h)
	{
		x = NewHudVictoriuriX(item.x);
		y = NewHudVictoriuriY(item.y);
		w = NewHudVictoriuriSize(item.w * item.scale);
		h = NewHudVictoriuriSize(item.h * item.scale);
	}

	bool IsKeyDown(int key)
	{
		return (GetAsyncKeyState(key) & 0x8000) != 0;
	}

	bool IsPressOrRepeat(int key)
	{
		return SEASON3B::IsPress(key) || SEASON3B::IsRepeat(key);
	}

	bool PointInRect(float px, float py, float x, float y, float w, float h)
	{
		return px >= x && px <= x + w && py >= y && py <= y + h;
	}

	int HitToolbar(float x, float y)
	{
		if (PointInRect(x, y, 16.0f, 16.0f, 78.0f, 25.0f)) return 1;
		if (PointInRect(x, y, 100.0f, 16.0f, 78.0f, 25.0f)) return 2;
		if (PointInRect(x, y, 184.0f, 16.0f, 78.0f, 25.0f)) return 3;
		if (PointInRect(x, y, 268.0f, 16.0f, 78.0f, 25.0f)) return 4;
		if (PointInRect(x, y, 352.0f, 16.0f, 110.0f, 25.0f)) return 5; //newhudvictoriuri - alterna HUD inteira/item.
		return 0;
	}

	int HitLayout(float x, float y)
	{
		for (int i = SEASON3B::HUD_EDITOR_COUNT - 1; i >= 0; --i)
		{
			if (!g_HudLayout[i].visible) continue;
			float itemX, itemY, itemW, itemH;
			GetScreenRect(g_HudLayout[i], itemX, itemY, itemW, itemH);
			if (PointInRect(x, y, itemX, itemY, itemW, itemH))
				return i;
		}
		return -1;
	}

	void CycleSelection(int direction)
	{
		int next = g_HudEditorSelected;
		for (int i = 0; i < SEASON3B::HUD_EDITOR_COUNT; ++i)
		{
			next += direction;
			if (next < 0) next = SEASON3B::HUD_EDITOR_COUNT - 1;
			if (next >= SEASON3B::HUD_EDITOR_COUNT) next = 0;
			if (g_HudLayout[next].visible)
			{
				g_HudEditorSelected = next;
				return;
			}
		}
	}

	void MoveWholeLayout(float deltaX, float deltaY)
	{
		//newhudvictoriuri - Arrastar o quadro move a HUD inteira mantendo as camadas sincronizadas.
		for (int i = 0; i < SEASON3B::HUD_EDITOR_COUNT; ++i)
		{
			g_HudLayout[i].x = ClampLayoutValue(g_HudLayout[i].x + deltaX, -1920.0f, 3840.0f);
			g_HudLayout[i].y = ClampLayoutValue(g_HudLayout[i].y + deltaY, -1080.0f, 2160.0f);
		}
	}

	void DrawFilledRect(float x, float y, float w, float h, float r, float g, float b, float a)
	{
		if (w <= 0.0f || h <= 0.0f) return;
		EnableAlphaTest(true);
		glColor4f(r, g, b, a);
		RenderColor(x, y, w, h, 0.0f, 0);
		EndRenderColor();
	}

	void DrawOutline(float x, float y, float w, float h, bool selected)
	{
		if (w <= 1.0f || h <= 1.0f) return;
		const float r = selected ? 1.0f : 0.2f;
		const float g = selected ? 0.75f : 0.8f;
		const float b = selected ? 0.1f : 1.0f;
		const float thickness = selected ? 2.0f : 1.0f;
		DrawFilledRect(x, y, w, thickness, r, g, b, 0.95f);
		DrawFilledRect(x, y + h - thickness, w, thickness, r, g, b, 0.95f);
		DrawFilledRect(x, y, thickness, h, r, g, b, 0.95f);
		DrawFilledRect(x + w - thickness, y, thickness, h, r, g, b, 0.95f);
	}

	void DrawToolbarButton(float x, const char* text, bool active, float width)
	{
		DrawFilledRect(x, 16.0f, width, 25.0f, active ? 0.18f : 0.03f, active ? 0.24f : 0.03f, active ? 0.42f : 0.03f, 0.90f);
		g_pRenderText->SetFont(g_hFixFont);
		g_pRenderText->SetBgColor(0, 0, 0, 0);
		g_pRenderText->SetTextColor(245, 245, 245, 255);
		g_pRenderText->RenderText((int)x, 21, text, (int)width, 0, RT3_SORT_CENTER);
	}
}

namespace SEASON3B
{
	bool NewHudEditorHandleCommand(const char* text)
	{
		if (text == NULL) return false;

		while (*text == ' ' || *text == '\t') ++text;
		if (strnicmp(text, "/hud", 4) != 0) return false;
		if (text[4] != '\0' && text[4] != ' ' && text[4] != '\t') return false;

		const char* argument = text + 4;
		while (*argument == ' ' || *argument == '\t') ++argument;

		if (stricmp(argument, "save") == 0)
		{
			NewHudEditorSave();
			g_HudEditorOpen = true;
			return true;
		}
		if (stricmp(argument, "load") == 0)
		{
			NewHudEditorLoad();
			g_HudEditorSelected = -1;
			SetStatus("Layout carregado");
			g_HudEditorOpen = true;
			g_HudEditorLayoutChanged = true;
			return true;
		}
		if (stricmp(argument, "reset") == 0)
		{
			ResetLayoutInternal();
			g_HudLayoutInitialized = true;
			g_HudEditorSelected = -1;
			MarkLayoutChanged();
			g_HudEditorOpen = true;
			return true;
		}
		if (stricmp(argument, "close") == 0)
		{
			g_HudEditorOpen = false;
			g_HudEditorDragging = false;
			return true;
		}
		if (stricmp(argument, "item") == 0 || stricmp(argument, "solo") == 0)
		{
			g_HudEditorMoveWholeLayout = false;
			g_HudEditorOpen = true;
			SetStatus("Modo ITEM: move somente o selecionado");
			return true;
		}
		if (stricmp(argument, "hud") == 0 || stricmp(argument, "tudo") == 0)
		{
			g_HudEditorMoveWholeLayout = true;
			g_HudEditorOpen = true;
			SetStatus("Modo HUD: move o quadro e as camadas juntas");
			return true;
		}

		if (g_HudEditorOpen)
		{
			g_HudEditorOpen = false;
			g_HudEditorDragging = false;
		}
		else
		{
			NewHudEditorLoad();
			g_HudEditorOpen = true;
			g_HudEditorSelected = -1;
			g_HudEditorLayoutChanged = true;
			SetStatus("Editor aberto - arraste um elemento");
		}
		return true;
	}

	bool NewHudEditorIsOpen()
	{
		return g_HudEditorOpen;
	}

	void NewHudEditorLoad()
	{
		ResetLayoutInternal();
		LoadFile();
		g_HudLayoutInitialized = true;
		g_HudEditorLayoutChanged = false;
	}

	void NewHudEditorSave()
	{
		EnsureLayoutLoaded();
		HudNumericLocaleGuard numericLocale; //newhudvictoriuri
		CreateDirectoryA("Custom", NULL);

		FILE* file = fopen(g_HudLayoutPath, "wt");
		if (file == NULL)
		{
			SetStatus("Nao foi possivel salvar Custom\\HUDLayout.ini");
			return;
		}

		fprintf(file, ";newhudvictoriuri - Layout salvo pelo editor /hud\n");
		fprintf(file, "[HUD_LAYOUT]\n");
		fprintf(file, "Version=1\n");
		fprintf(file, "Resolution=1920x1080\n");
		for (int i = 0; i < HUD_EDITOR_COUNT; ++i)
		{
			fprintf(file, "%s=%.2f,%.2f,%.2f,%.2f,%.2f,%d\n",
				g_HudLayout[i].id, g_HudLayout[i].x, g_HudLayout[i].y,
				g_HudLayout[i].w, g_HudLayout[i].h, g_HudLayout[i].scale,
				g_HudLayout[i].visible ? 1 : 0);
		}
		fclose(file);
		WriteGeneratedHeader();
		g_HudEditorLayoutChanged = false;
		SetStatus("Salvo: Custom\\HUDLayout.ini + header gerado");
	}

	bool NewHudEditorUpdateMouseEvent()
	{
		if (!g_HudEditorOpen) return false;
		EnsureLayoutLoaded();

		const float mouseX = (float)MouseX;
		const float mouseY = (float)MouseY;

		if (g_HudEditorDragging)
		{
			if (!MouseLButton)
			{
				g_HudEditorDragging = false;
				g_HudEditorDragMode = 0;
				return true;
			}

			if (IsValidElement(g_HudEditorSelected))
			{
				const float nowX = ScreenToLayoutX(mouseX);
				const float nowY = ScreenToLayoutY(mouseY);
				HUD_LAYOUT_ITEM& item = g_HudLayout[g_HudEditorSelected];
				if (g_HudEditorDragMode == 2)
				{
					const float deltaX = nowX - g_HudEditorDragStartX;
					const float deltaY = nowY - g_HudEditorDragStartY;
					item.w = ClampLayoutValue(g_HudEditorItemStartW + deltaX, 1.0f, 3840.0f);
					item.h = ClampLayoutValue(g_HudEditorItemStartH + deltaY, 1.0f, 2160.0f);
					MarkLayoutChanged();
				}
				else
				{
					const float newX = ClampLayoutValue(g_HudEditorItemStartX + nowX - g_HudEditorDragStartX, -1920.0f, 3840.0f);
					const float newY = ClampLayoutValue(g_HudEditorItemStartY + nowY - g_HudEditorDragStartY, -1080.0f, 2160.0f);
					const bool moveWholeLayout = g_HudEditorSelected == SEASON3B::HUD_EDITOR_FRAME
						&& g_HudEditorMoveWholeLayout && !IsKeyDown(VK_MENU); //newhudvictoriuri
					if (moveWholeLayout)
					{
						MoveWholeLayout(newX - item.x, newY - item.y);
					}
					else
					{
						item.x = newX;
						item.y = newY;
					}
					MarkLayoutChanged();
				}
			}
			return true;
		}

		if (MouseLButtonPush)
		{
			const int toolbar = HitToolbar(mouseX, mouseY);
			if (toolbar == 1)
			{
				NewHudEditorSave();
				MouseLButtonPush = false;
				return true;
			}
			if (toolbar == 2)
			{
				ResetLayoutInternal();
				g_HudLayoutInitialized = true;
				g_HudEditorSelected = -1;
				MarkLayoutChanged();
				MouseLButtonPush = false;
				return true;
			}
			if (toolbar == 3)
			{
				NewHudEditorLoad();
				g_HudEditorSelected = -1;
				SetStatus("Layout carregado");
				g_HudEditorLayoutChanged = true;
				MouseLButtonPush = false;
				return true;
			}
			if (toolbar == 4)
			{
				g_HudEditorOpen = false;
				g_HudEditorSelected = -1;
				MouseLButtonPush = false;
				return true;
			}
			if (toolbar == 5)
			{
				g_HudEditorMoveWholeLayout = !g_HudEditorMoveWholeLayout;
				SetStatus(g_HudEditorMoveWholeLayout
					? "Modo HUD: mover FRAME move tudo"
					: "Modo ITEM: mover/redimensionar somente o selecionado");
				MouseLButtonPush = false;
				return true;
			}

			g_HudEditorSelected = HitLayout(mouseX, mouseY);
			if (g_HudEditorSelected >= 0)
			{
				const float layoutX = ScreenToLayoutX(mouseX);
				const float layoutY = ScreenToLayoutY(mouseY);
				HUD_LAYOUT_ITEM& item = g_HudLayout[g_HudEditorSelected];
				g_HudEditorDragStartX = layoutX;
				g_HudEditorDragStartY = layoutY;
				g_HudEditorItemStartX = item.x;
				g_HudEditorItemStartY = item.y;
				g_HudEditorItemStartW = item.w;
				g_HudEditorItemStartH = item.h;
				float itemX, itemY, itemW, itemH;
				GetScreenRect(item, itemX, itemY, itemW, itemH);
				const bool onResizeCorner = IsKeyDown(VK_CONTROL)
					&& mouseX >= itemX + itemW - 14.0f && mouseY >= itemY + itemH - 14.0f;
				g_HudEditorDragMode = onResizeCorner ? 2 : 1;
				g_HudEditorDragging = true;
				MouseLButtonPush = false;
				return true;
			}
			return true;
		}

		if (MouseWheel != 0 && IsValidElement(g_HudEditorSelected))
		{
			HUD_LAYOUT_ITEM& item = g_HudLayout[g_HudEditorSelected];
			item.scale = ClampLayoutValue(item.scale + (MouseWheel > 0 ? 0.05f : -0.05f), 0.25f, 4.0f);
			MouseWheel = 0;
			MarkLayoutChanged();
		}

		return true;
	}

	bool NewHudEditorUpdateKeyEvent()
	{
		if (!g_HudEditorOpen) return false;
		EnsureLayoutLoaded();

		if (IsPress(VK_ESCAPE))
		{
			if (g_HudEditorSelected >= 0)
				g_HudEditorSelected = -1;
			else
				g_HudEditorOpen = false;
			return true;
		}

		if (IsPress(VK_F5) || (IsKeyDown(VK_CONTROL) && IsPress('S')))
		{
			NewHudEditorSave();
			return true;
		}

		if (IsPress('G'))
		{
			g_HudEditorMoveWholeLayout = !g_HudEditorMoveWholeLayout;
			SetStatus(g_HudEditorMoveWholeLayout
				? "Modo HUD: mover FRAME move tudo"
				: "Modo ITEM: mover/redimensionar somente o selecionado");
			return true;
		}

		if (IsPress(VK_TAB))
		{
			CycleSelection(IsKeyDown(VK_SHIFT) ? -1 : 1);
			return true;
		}

		if (!IsValidElement(g_HudEditorSelected)) return true;

		HUD_LAYOUT_ITEM& item = g_HudLayout[g_HudEditorSelected];
		const float step = IsKeyDown(VK_SHIFT) ? 10.0f : 1.0f;
		bool changed = false;
		float groupDeltaX = 0.0f;
		float groupDeltaY = 0.0f;
		if (IsKeyDown(VK_CONTROL) && IsPressOrRepeat('T')) //newhudvictoriuri - Ctrl+T redimensiona o item uniformemente.
		{
			const float resizeStep = IsKeyDown(VK_SHIFT) ? -10.0f : 1.0f; //newhudvictoriuri - Ctrl+Shift+T diminui.
			item.w = ClampLayoutValue(item.w + resizeStep, 1.0f, 3840.0f);
			item.h = ClampLayoutValue(item.h + resizeStep, 1.0f, 2160.0f);
			MarkLayoutChanged();
			return true;
		}

		if (IsPressOrRepeat(VK_LEFT))
		{
			if (IsKeyDown(VK_CONTROL)) item.w = ClampLayoutValue(item.w - step, 1.0f, 3840.0f);
			else if (g_HudEditorSelected == HUD_EDITOR_FRAME && g_HudEditorMoveWholeLayout) groupDeltaX -= step;
			else item.x -= step;
			changed = true;
		}
		if (IsPressOrRepeat(VK_RIGHT))
		{
			if (IsKeyDown(VK_CONTROL)) item.w = ClampLayoutValue(item.w + step, 1.0f, 3840.0f);
			else if (g_HudEditorSelected == HUD_EDITOR_FRAME && g_HudEditorMoveWholeLayout) groupDeltaX += step;
			else item.x += step;
			changed = true;
		}
		if (IsPressOrRepeat(VK_UP))
		{
			if (IsKeyDown(VK_CONTROL)) item.h = ClampLayoutValue(item.h - step, 1.0f, 2160.0f);
			else if (g_HudEditorSelected == HUD_EDITOR_FRAME && g_HudEditorMoveWholeLayout) groupDeltaY -= step;
			else item.y -= step;
			changed = true;
		}
		if (IsPressOrRepeat(VK_DOWN))
		{
			if (IsKeyDown(VK_CONTROL)) item.h = ClampLayoutValue(item.h + step, 1.0f, 2160.0f);
			else if (g_HudEditorSelected == HUD_EDITOR_FRAME && g_HudEditorMoveWholeLayout) groupDeltaY += step;
			else item.y += step;
			changed = true;
		}

		if (IsPress(VK_ADD) || IsPress(VK_OEM_PLUS))
		{
			item.scale = ClampLayoutValue(item.scale + 0.05f, 0.25f, 4.0f);
			changed = true;
		}
		if (IsPress(VK_SUBTRACT) || IsPress(VK_OEM_MINUS))
		{
			item.scale = ClampLayoutValue(item.scale - 0.05f, 0.25f, 4.0f);
			changed = true;
		}
		if (IsPress(VK_DELETE))
		{
			item.visible = !item.visible;
			changed = true;
		}

		if (changed)
		{
			if (g_HudEditorSelected == HUD_EDITOR_FRAME && g_HudEditorMoveWholeLayout && !IsKeyDown(VK_CONTROL))
				MoveWholeLayout(groupDeltaX, groupDeltaY);
			item.x = ClampLayoutValue(item.x, -1920.0f, 3840.0f);
			item.y = ClampLayoutValue(item.y, -1080.0f, 2160.0f);
			MarkLayoutChanged();
		}

		return true;
	}

	bool NewHudEditorUpdate()
	{
		const bool changed = g_HudEditorLayoutChanged;
		g_HudEditorLayoutChanged = false;
		return changed;
	}

	void NewHudEditorRender()
	{
		if (!g_HudEditorOpen) return;
		EnsureLayoutLoaded();

		//newhudvictoriuri - Barra de comandos fica fora da area da HUD para nao esconder a arte.
		DrawFilledRect(12.0f, 12.0f, 560.0f, 82.0f, 0.02f, 0.02f, 0.02f, 0.88f);
		DrawToolbarButton(16.0f, "SALVAR", false, 78.0f);
		DrawToolbarButton(100.0f, "RESET", false, 78.0f);
		DrawToolbarButton(184.0f, "CARREGAR", false, 78.0f);
		DrawToolbarButton(268.0f, "FECHAR", false, 78.0f);
		DrawToolbarButton(352.0f, g_HudEditorMoveWholeLayout ? "MODO: HUD" : "MODO: ITEM", g_HudEditorMoveWholeLayout, 110.0f); //newhudvictoriuri

		g_pRenderText->SetFont(g_hFixFont);
		g_pRenderText->SetBgColor(0, 0, 0, 0);
		g_pRenderText->SetTextColor(225, 225, 225, 255);
		g_pRenderText->RenderText(16, 47, "Arraste | Ctrl+quina/T: tamanho | Ctrl+setas: tamanho | +/-: escala | G: modo", 550, 0, RT3_SORT_LEFT);

		char info[256] = { 0 };
		if (IsValidElement(g_HudEditorSelected))
		{
			HUD_LAYOUT_ITEM& item = g_HudLayout[g_HudEditorSelected];
			sprintf(info, "%s  X %.0f  Y %.0f  W %.0f  H %.0f  S %.2f  [%s]",
				item.label, item.x, item.y, item.w, item.h, item.scale,
				item.visible ? "visivel" : "oculto");
		}
		else
		{
			strncpy(info, "Clique em qualquer parte da HUD para selecionar", sizeof(info) - 1);
		}
		g_pRenderText->SetTextColor(255, 214, 120, 255);
		g_pRenderText->RenderText(16, 62, info, 550, 0, RT3_SORT_LEFT);

		for (int i = 0; i < HUD_EDITOR_COUNT; ++i)
		{
			if (!g_HudLayout[i].visible) continue;
			float x, y, w, h;
			GetScreenRect(g_HudLayout[i], x, y, w, h);
			DrawOutline(x, y, w, h, i == g_HudEditorSelected);

			const int textY = (y > 14.0f) ? (int)(y - 13.0f) : (int)(y + h + 2.0f);
			g_pRenderText->SetTextColor(i == g_HudEditorSelected ? 255 : 150,
				i == g_HudEditorSelected ? 220 : 210,
				i == g_HudEditorSelected ? 80 : 255, 255);
			g_pRenderText->RenderText((int)x, textY, g_HudLayout[i].label, 0, 0, RT3_SORT_LEFT);
		}

		if (g_HudEditorStatusUntil > GetTickCount())
		{
			g_pRenderText->SetTextColor(180, 255, 180, 255);
			g_pRenderText->RenderText(585, 20, g_HudEditorStatus, 500, 0, RT3_SORT_LEFT);
		}

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}

	float NewHudEditorX(int element)
	{
		EnsureLayoutLoaded();
		return IsValidElement(element) ? g_HudLayout[element].x : 0.0f;
	}

	float NewHudEditorY(int element)
	{
		EnsureLayoutLoaded();
		return IsValidElement(element) ? g_HudLayout[element].y : 0.0f;
	}

	float NewHudEditorW(int element)
	{
		EnsureLayoutLoaded();
		return IsValidElement(element) ? g_HudLayout[element].w * g_HudLayout[element].scale : 0.0f;
	}

	float NewHudEditorH(int element)
	{
		EnsureLayoutLoaded();
		return IsValidElement(element) ? g_HudLayout[element].h * g_HudLayout[element].scale : 0.0f;
	}

	float NewHudEditorScale(int element)
	{
		EnsureLayoutLoaded();
		return IsValidElement(element) ? g_HudLayout[element].scale : 1.0f;
	}

	bool NewHudEditorVisible(int element, bool fallback)
	{
		EnsureLayoutLoaded();
		return IsValidElement(element) ? g_HudLayout[element].visible : fallback;
	}
}

#endif // CLIENT_VERSION >= 1001
