#pragma once

//newhudvictoriuri - Editor de coordenadas da HUD MuAway.
#if CLIENT_VERSION >= 1001

//newhudvictoriuri - Conversao compartilhada entre o desenho da HUD e o editor.
float NewHudVictoriuriScale();
float NewHudVictoriuriX(float psdX);
float NewHudVictoriuriY(float psdY);
float NewHudVictoriuriSize(float value);

namespace SEASON3B
{
	enum NEW_HUD_EDITOR_ELEMENT
	{
		HUD_EDITOR_FRAME = 0,
		HUD_EDITOR_FPS,
		HUD_EDITOR_COORDS,
		HUD_EDITOR_HP_GAUGE,
		HUD_EDITOR_HP_NUMBER,
		HUD_EDITOR_MP_GAUGE,
		HUD_EDITOR_MP_NUMBER,
		HUD_EDITOR_AG_GAUGE,
		HUD_EDITOR_AG_NUMBER,
		HUD_EDITOR_SD_GAUGE,
		HUD_EDITOR_SD_NUMBER,
		HUD_EDITOR_EXP_GAUGE,
		HUD_EDITOR_EXP_NUMBER,
		HUD_EDITOR_SKILL_CURRENT,
		HUD_EDITOR_POTION_Q,
		HUD_EDITOR_POTION_W,
		HUD_EDITOR_POTION_E,
		HUD_EDITOR_BUTTON_PARTY,
		HUD_EDITOR_BUTTON_CHARACTER,
		HUD_EDITOR_BUTTON_INVENTORY,
		HUD_EDITOR_BUTTON_RIGHT_TOP,
		HUD_EDITOR_BUTTON_RIGHT_BOTTOM,
		HUD_EDITOR_SKILL_ARROW_UP,
		HUD_EDITOR_SKILL_ARROW_DOWN,
		HUD_EDITOR_SKILL_POPUP,
		HUD_EDITOR_COUNT
	};

	//newhudvictoriuri - Retorna true quando o comando foi consumido pelo editor.
	bool NewHudEditorHandleCommand(const char* text);
	bool NewHudEditorIsOpen();
	bool NewHudEditorUpdateMouseEvent();
	bool NewHudEditorUpdateKeyEvent();
	//newhudvictoriuri - Retorna true quando alguma coordenada mudou desde o ultimo Update.
	bool NewHudEditorUpdate();
	void NewHudEditorLoad();
	void NewHudEditorSave();
	void NewHudEditorRender();

	//newhudvictoriuri - Valores em coordenadas logicas do PSD 1920x1080.
	// O layout vem do Custom\\HUDLayout.ini; nao existem coordenadas fallback no desenho.
	float NewHudEditorX(int element);
	float NewHudEditorY(int element);
	float NewHudEditorW(int element);
	float NewHudEditorH(int element);
	float NewHudEditorScale(int element);
	bool NewHudEditorVisible(int element, bool fallback = true);
}

#endif // CLIENT_VERSION >= 1001
