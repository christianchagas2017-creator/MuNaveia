//////////////////////////////////////////////////////////////////////
// NewUIMainFrameWindow.cpp: implementation of the CNewUIMainFrameWindow class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "NewUIMainFrameWindow.h"	// self
#if CLIENT_VERSION == 1002
#include "NewUIOptionWindowFairPlay.h"
#else
#include "NewUIOptionWindow.h"
#endif
#include "NewUISystem.h"
#include "UIBaseDef.h"
#include "DSPlaySound.h"
#include "ZzzInfomation.h"
#include "ZzzBMD.h"
#include "ZzzObject.h"
#include "ZzzCharacter.h"
#include "ZzzInterface.h"
#include "ZzzInventory.h"
#include "wsclientinline.h"
#include "CSItemOption.h"
#include "CSChaosCastle.h"
#include "MapManager.h"
#include "CharacterManager.h"
#include "SkillManager.h"
#include "GMDoppelGanger1.h"
#include "GMDoppelGanger2.h"
#include "GMDoppelGanger3.h"
#include "GMDoppelGanger4.h"
#include "./Time/CTimCheck.h"
#ifdef PBG_ADD_NEWCHAR_MONK_SKILL
#include "MonkSystem.h"
#endif //PBG_ADD_NEWCHAR_MONK_SKILL

#ifdef PBG_ADD_INGAMESHOP_UI_MAINFRAME
#include "GameShop/InGameShopSystem.h"
#endif //PBG_ADD_INGAMESHOP_UI_MAINFRAME

#include "WideScreen.h"
#include "NewUIHudEditor.h" //newhudvictoriuri - editor visual da HUD.
#include <vector>

extern float g_fScreenRate_x;
extern float g_fScreenRate_y;
extern int  MouseUpdateTime;
extern int  MouseUpdateTimeMax;
extern int SelectedCharacter;
extern int Attacking;

#if CLIENT_VERSION >= 1001
//newhudvictoriuri - V24.6.8: coordenadas convertidas da source MuAway 878x48 enviada pelo usuario.
//newhudvictoriuri - V24.6.7: ajuste final de tamanho + centralizacao de textos/medidores.
//newhudvictoriuri - V24.6.5: usa o HUD_MuAway.zip enviado pelo usuario como arte mestre.
//newhudvictoriuri - V24: port final para a source enviada; arte/posicoes seguem o PSD 1920x1080.
//newhudvictoriuri - V13: sistema de coordenadas EXATO do PSD 1920x1080.
// A HUD final do Photoshop foi desenhada em 1920x1080. Todas as posicoes abaixo
// usam as coordenadas reais das camadas do PSD e uma unica escala uniforme.
float NewHudVictoriuriScale()
{
	//newhudvictoriuri - V24.6.6: escala pela ALTURA do canvas 1920x1080.
	// Em janela mais estreita que 16:9 o sx deixava a HUD pequena demais.
	// Mantemos o tamanho visual correto do PSD e limitamos apenas para a arte
	// real (X=143..1777 / W=1634) nao ultrapassar a largura da janela.
	const float sy = gWideScreen.g_WideWindowHeight / 1080.0f;
	const float safeWidth = max(1.0f, gWideScreen.g_WideWindowWidth - 16.0f);
	const float hudFrameWidth = max(1.0f, SEASON3B::NewHudEditorW(SEASON3B::HUD_EDITOR_FRAME));
	const float maxHudScale = safeWidth / hudFrameWidth;
	//newhudvictoriuri - V24.6.7: em janelas menores que 1080p, aumenta 7.5% sem alterar o layout interno.
	// Em 1920x1080 ou maior, preserva exatamente a escala 1:1 do PSD.
	const float visualScale = (sy < 1.0f) ? (sy * 1.075f) : sy;
	return min(visualScale, maxHudScale);
}

//newhudvictoriuri - Centraliza horizontalmente o canvas PSD e ancora verticalmente no rodape.
float NewHudVictoriuriX(float psdX)
{
	const float s = NewHudVictoriuriScale();
	const float offsetX = (gWideScreen.g_WideWindowWidth - (1920.0f * s)) * 0.5f;
	return offsetX + (psdX * s);
}

//newhudvictoriuri - Y real da camada do PSD, sempre ancorado no rodape da janela.
float NewHudVictoriuriY(float psdY)
{
	const float s = NewHudVictoriuriScale();
	const float offsetY = gWideScreen.g_WideWindowHeight - (1080.0f * s);
	return offsetY + (psdY * s);
}

//newhudvictoriuri - Escala uniforme para largura/altura do PSD.
float NewHudVictoriuriSize(float value)
{
	return value * NewHudVictoriuriScale();
}

//newhudvictoriuri - Mantem os numeros HP/MP/SD/AG sincronizados com a escala real da HUD.
// O tamanho final continua sendo controlado pelo campo scale do HUDLayout.ini;
// nao aplicamos um minimo escondido que deixe os numeros maiores que o PSD.
static float NewHudVictoriuriReadableNumberScale(float baseScale)
{
	return baseScale * NewHudVictoriuriScale(); //newhudvictoriuri - V26: sem aumento artificial da fonte.
}

//newhudvictoriuri - V26: contador dos itens precisa continuar legivel em janelas
// menores, mas sem alterar o tamanho dos numeros HP/MP/SD/AG.
static float NewHudVictoriuriItemCountScale(float itemScale)
{
	return itemScale * max(NewHudVictoriuriScale(), 0.75f);
}

//newhudvictoriuri - V27: escolhe o maximo valido recebido pelo cliente para
// que um pacote atrasado/zerado nao transforme a barra em 100% permanente.
// O fallback conserva o primeiro valor valido enquanto o pacote de maximo nao chega.
static DWORD NewHudVictoriuriResolveGaugeMax(DWORD characterMax, DWORD masterMax,
	DWORD currentValue, DWORD& fallbackMax)
{
	const DWORD packetMax = max(characterMax, masterMax); //newhudvictoriuri
	if (packetMax > 0)
		fallbackMax = packetMax; //newhudvictoriuri - pacote real tem prioridade.
	else if (currentValue == 0)
		fallbackMax = 0; //newhudvictoriuri - permite reinicializar ao trocar/entrar no personagem.
	if (currentValue > fallbackMax)
		fallbackMax = currentValue; //newhudvictoriuri - primeiro valor observado vira referencia.
	DWORD resolvedMax = max(packetMax, fallbackMax); //newhudvictoriuri
	resolvedMax = max(resolvedMax, currentValue); //newhudvictoriuri - nunca menor que o valor exibido.
	return max(resolvedMax, (DWORD)1); //newhudvictoriuri - evita divisao por zero.
}

//newhudvictoriuri - V24.9: o pack original usa cinco pecas de 64 px de altura,
// mas somente os primeiros 48 px possuem arte. Renderizar os 64 px inteiros
// estica o padding transparente e deixa a HUD com faixas/cortes aparentes.
static void NewHudVictoriuriRenderPackedPart(GLuint texture, float x, float y,
	float width, float height, float visibleWidth, float visibleHeight)
{
	BITMAP_t* bmp = &Bitmaps[texture];
	if (bmp == NULL || bmp->Width <= 0.0f || bmp->Height <= 0.0f)
		return;

	const float u = min(visibleWidth, bmp->RealWidth) / bmp->Width;
	const float v = min(visibleHeight, bmp->RealHeight) / bmp->Height;
	if (u <= 0.0f || v <= 0.0f)
		return;

	RenderBitmap(texture, x, y, width, height, 0.0f, 0.0f, u, v);
}

//newhudvictoriuri - Monta o HUD_MuAway nativo dentro do retangulo FRAME do INI.
// Ordem do pack: lateral esquerda, menu01, menu02, menu03, lateral direita.
// As coordenadas do HUDLayout.ini continuam sendo usadas somente como destino.
static void NewHudVictoriuriRenderPackedFrame(float x, float y, float width, float height)
{
	const float scaleX = width / 878.0f;
	float partX = x;

	NewHudVictoriuriRenderPackedPart(SEASON3B::CNewUIMainFrameWindow::IMAGE_MENU_3,
		partX, y, 119.0f * scaleX, height, 119.0f, 48.0f); //newhudvictoriuri
	partX += 119.0f * scaleX;
	NewHudVictoriuriRenderPackedPart(SEASON3B::CNewUIMainFrameWindow::NEW_FRAME,
		partX, y, 256.0f * scaleX, height, 256.0f, 48.0f); //newhudvictoriuri
	partX += 256.0f * scaleX;
	NewHudVictoriuriRenderPackedPart(SEASON3B::CNewUIMainFrameWindow::IMAGE_MENU_1,
		partX, y, 128.0f * scaleX, height, 128.0f, 48.0f); //newhudvictoriuri
	partX += 128.0f * scaleX;
	NewHudVictoriuriRenderPackedPart(SEASON3B::CNewUIMainFrameWindow::IMAGE_MENU_2,
		partX, y, 256.0f * scaleX, height, 256.0f, 48.0f); //newhudvictoriuri
	partX += 256.0f * scaleX;
	NewHudVictoriuriRenderPackedPart(SEASON3B::CNewUIMainFrameWindow::IMAGE_MENU_2_1,
		partX, y, 119.0f * scaleX, height, 119.0f, 48.0f); //newhudvictoriuri
}

//newhudvictoriuri - Renderiza somente a area desenhada de cada OZT dinamico.
// O pack usa orb 53x48 dentro de 64x64 e SD/AG 15x36 dentro de 16x64.
static void NewHudVictoriuriRenderVerticalGauge(GLuint texture, float x, float y,
	float width, float height, float emptyRatio, float visibleTextureWidth,
	float visibleTextureHeight)
{
	BITMAP_t* bmp = &Bitmaps[texture];
	if (bmp == NULL || bmp->Width <= 0.0f || bmp->Height <= 0.0f || bmp->RealWidth <= 0.0f || bmp->RealHeight <= 0.0f)
		return;

	if (emptyRatio < 0.0f) emptyRatio = 0.0f;
	if (emptyRatio > 1.0f) emptyRatio = 1.0f;

	const float visible = 1.0f - emptyRatio;
	if (visible <= 0.0f)
		return;

	const float textureWidth = (visibleTextureWidth > 0.0f) ? min(visibleTextureWidth, bmp->RealWidth) : bmp->RealWidth;
	const float textureHeight = (visibleTextureHeight > 0.0f) ? min(visibleTextureHeight, bmp->RealHeight) : bmp->RealHeight;
	const float realU = textureWidth / bmp->Width;
	const float realV = textureHeight / bmp->Height;
	const float renderY = y + (height * emptyRatio);
	const float renderH = height * visible;
	const float texV = realV * emptyRatio;
	const float texH = realV * visible;

	RenderBitmap(texture, x, renderY, width, renderH, 0.0f, texV, realU, texH);
}

//newhudvictoriuri - V19: mesma correcao de UV para a barra horizontal de EXP.
static void NewHudVictoriuriRenderHorizontalGauge(GLuint texture, float x, float y, float fullWidth, float height, float ratio)
{
	BITMAP_t* bmp = &Bitmaps[texture];
	if (bmp == NULL || bmp->Width <= 0.0f || bmp->Height <= 0.0f || bmp->RealWidth <= 0.0f || bmp->RealHeight <= 0.0f)
		return;

	if (ratio < 0.0f) ratio = 0.0f;
	if (ratio > 1.0f) ratio = 1.0f;
	if (ratio <= 0.0f)
		return;

	const float realU = bmp->RealWidth / bmp->Width;
	const float realV = bmp->RealHeight / bmp->Height;
	RenderBitmap(texture, x, y, fullWidth * ratio, height, 0.0f, 0.0f, realU * ratio, realV);
}
//newhudvictoriuri - V26: usa o icone normal para os dois estados.
// Os arquivos btn_*_pressed do pack V24 sao quadros completos incompatíveis com
// a transparencia desta renderizacao e apareciam como quadrados brancos/pixelados.
// O estado pressionado continua real, mas o feedback e feito deslocando o icone
// 1 px para baixo/direita e escurecendo-o levemente, como um botao afundado.
static void NewHudVictoriuriRenderButtonIcon(GLuint normalTexture, GLuint pressedTexture,
	float boxX, float boxY, float boxW, float boxH, bool pressed)
{
	(void)pressedTexture; //newhudvictoriuri - V26: nao usa os quadros V24 que geram os quadrados.
	GLuint texture = normalTexture; //newhudvictoriuri - V26: icone normal preserva o alpha correto.
	BITMAP_t* bmp = &Bitmaps[texture];
	if (bmp == NULL || bmp->Width <= 0.0f || bmp->Height <= 0.0f || bmp->RealWidth <= 0.0f || bmp->RealHeight <= 0.0f)
		return;

	const float realU = bmp->RealWidth / bmp->Width;
	const float realV = bmp->RealHeight / bmp->Height;

	const float iconSize = NewHudVictoriuriSize(32.0f); //newhudvictoriuri - icone nativo do pack.
	const float pressOffset = pressed ? NewHudVictoriuriSize(1.0f) : 0.0f; //newhudvictoriuri - V31: desloca somente durante clique fisico iniciado no botao.
	const float renderX = boxX + (boxW - iconSize) * 0.5f + pressOffset; //newhudvictoriuri
	const float renderY = boxY + (boxH - iconSize) * 0.5f + pressOffset; //newhudvictoriuri

	//newhudvictoriuri - V32: o OZT do icone possui um fundo preto opaco 24x24.
	// Em alpha normal esse quadrado escuro parecia um botao permanentemente afundado.
	// No estado UP usamos blend aditivo: o preto nao altera a moldura que ja existe
	// na HUD e somente o desenho dourado do icone aparece. Durante o clique fisico
	// voltamos ao alpha normal + deslocamento, criando o efeito afundado de verdade.
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	if (!pressed)
	{
		EnableAlphaBlend(); //newhudvictoriuri - idle sem o quadrado preto/afundado.
	}
	else
	{
		EnableAlphaTest(); //newhudvictoriuri - clique real mostra o fundo/pressao.
		glColor4f(0.82f, 0.82f, 0.82f, 1.0f);
	}
	RenderBitmap(texture, renderX, renderY, iconSize, iconSize, 0.0f, 0.0f, realU, realV); //newhudvictoriuri
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	EnableAlphaTest(); //newhudvictoriuri - restaura o blend padrao da interface.
}

//newhudvictoriuri - V31: estado visual de pressao independente do estado das janelas.
// Nao usamos MouseLButton porque outras rotinas antigas podem consumir/manter esse flag.
// A animacao so nasce na transicao fisica UP->DOWN do Windows, se o clique COMECOU
// dentro de um dos botoes. Arrastar o mouse para cima do botao com o clique ja segurado
// nao ativa o efeito, e soltar o LMB zera imediatamente o estado visual.
enum NEW_HUD_VICTORIURI_PRESSED_BUTTON
{
	NEW_HUD_PRESS_NONE = 0,
	NEW_HUD_PRESS_PARTY,
	NEW_HUD_PRESS_CHARACTER,
	NEW_HUD_PRESS_INVENTORY,
};

static int g_NewHudVictoriuriPressedButton = NEW_HUD_PRESS_NONE; //newhudvictoriuri
static bool g_NewHudVictoriuriPhysicalLButtonWasDown = false; //newhudvictoriuri

static void NewHudVictoriuriUpdatePhysicalButtonPress(
	float partyX, float partyY, float partyW, float partyH,
	float charX, float charY, float charW, float charH,
	float invenX, float invenY, float invenW, float invenH)
{
	const bool physicalDown = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0); //newhudvictoriuri - V31: estado real do mouse no Windows.

	if (physicalDown && !g_NewHudVictoriuriPhysicalLButtonWasDown)
	{
		g_NewHudVictoriuriPressedButton = NEW_HUD_PRESS_NONE; //newhudvictoriuri

		if (SEASON3B::CheckMouseIn((int)partyX, (int)partyY, (int)partyW, (int)partyH))
			g_NewHudVictoriuriPressedButton = NEW_HUD_PRESS_PARTY; //newhudvictoriuri
		else if (SEASON3B::CheckMouseIn((int)charX, (int)charY, (int)charW, (int)charH))
			g_NewHudVictoriuriPressedButton = NEW_HUD_PRESS_CHARACTER; //newhudvictoriuri
		else if (SEASON3B::CheckMouseIn((int)invenX, (int)invenY, (int)invenW, (int)invenH))
			g_NewHudVictoriuriPressedButton = NEW_HUD_PRESS_INVENTORY; //newhudvictoriuri
	}

	if (!physicalDown)
		g_NewHudVictoriuriPressedButton = NEW_HUD_PRESS_NONE; //newhudvictoriuri - V31: soltou, volta ao normal no mesmo frame.

	g_NewHudVictoriuriPhysicalLButtonWasDown = physicalDown; //newhudvictoriuri
}

//newhudvictoriuri - V32: clique funcional independente do CCustomButtons.
// A HUD usa coordenadas dinamicas (PSD + resize), enquanto os controles antigos
// guardavam hitboxes criadas em outro momento. Isso fazia o icone aparecer no
// lugar certo, mas o clique cair fora da area real. Aqui o clique e testado
// diretamente contra as MESMAS coordenadas usadas no RenderButtons().
static bool g_NewHudVictoriuriClickLButtonWasDown = false; //newhudvictoriuri
static int g_NewHudVictoriuriClickStartedButton = NEW_HUD_PRESS_NONE; //newhudvictoriuri
static bool g_NewHudVictoriuriRightTopWasDown = false; //newhudvictoriuri - Central de Seguranca
static bool g_NewHudVictoriuriRightTopStarted = false; //newhudvictoriuri - Central de Seguranca

static int NewHudVictoriuriPollHudButtonClick(
	float partyX, float partyY, float partyW, float partyH, bool partyVisible,
	float charX, float charY, float charW, float charH, bool charVisible,
	float invenX, float invenY, float invenW, float invenH, bool invenVisible)
{
	const bool physicalDown = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0); //newhudvictoriuri

	// Registra somente onde o clique realmente COMEcOU.
	if (physicalDown && !g_NewHudVictoriuriClickLButtonWasDown)
	{
		g_NewHudVictoriuriClickStartedButton = NEW_HUD_PRESS_NONE; //newhudvictoriuri
		if (partyVisible && SEASON3B::CheckMouseIn((int)partyX, (int)partyY, (int)partyW, (int)partyH))
			g_NewHudVictoriuriClickStartedButton = NEW_HUD_PRESS_PARTY; //newhudvictoriuri
		else if (charVisible && SEASON3B::CheckMouseIn((int)charX, (int)charY, (int)charW, (int)charH))
			g_NewHudVictoriuriClickStartedButton = NEW_HUD_PRESS_CHARACTER; //newhudvictoriuri
		else if (invenVisible && SEASON3B::CheckMouseIn((int)invenX, (int)invenY, (int)invenW, (int)invenH))
			g_NewHudVictoriuriClickStartedButton = NEW_HUD_PRESS_INVENTORY; //newhudvictoriuri
	}

	int clickedButton = NEW_HUD_PRESS_NONE;
	if (!physicalDown && g_NewHudVictoriuriClickLButtonWasDown)
	{
		// So confirma se soltou em cima do MESMO botao onde pressionou.
		if (g_NewHudVictoriuriClickStartedButton == NEW_HUD_PRESS_PARTY && partyVisible
			&& SEASON3B::CheckMouseIn((int)partyX, (int)partyY, (int)partyW, (int)partyH))
			clickedButton = NEW_HUD_PRESS_PARTY; //newhudvictoriuri
		else if (g_NewHudVictoriuriClickStartedButton == NEW_HUD_PRESS_CHARACTER && charVisible
			&& SEASON3B::CheckMouseIn((int)charX, (int)charY, (int)charW, (int)charH))
			clickedButton = NEW_HUD_PRESS_CHARACTER; //newhudvictoriuri
		else if (g_NewHudVictoriuriClickStartedButton == NEW_HUD_PRESS_INVENTORY && invenVisible
			&& SEASON3B::CheckMouseIn((int)invenX, (int)invenY, (int)invenW, (int)invenH))
			clickedButton = NEW_HUD_PRESS_INVENTORY; //newhudvictoriuri

		g_NewHudVictoriuriClickStartedButton = NEW_HUD_PRESS_NONE; //newhudvictoriuri
	}

	g_NewHudVictoriuriClickLButtonWasDown = physicalDown; //newhudvictoriuri
	return clickedButton;
}

//newhudvictoriuri - V22: feedback dos dois botoes da direita desenhados na base.
// A area foi recalculada pela propria textura: X global 1449..1545.
static void NewHudVictoriuriRenderRightButtonPress(float x, float y, float width, float height, bool pressed)
{
	if (!pressed)
		return;

	EnableAlphaTest();
	glColor4f(1.0f, 0.78f, 0.35f, 0.22f);
	RenderColor(x + NewHudVictoriuriSize(2.0f), y + NewHudVictoriuriSize(2.0f),
		width - NewHudVictoriuriSize(4.0f), height - NewHudVictoriuriSize(4.0f));
	EndRenderColor();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
#endif


SEASON3B::CNewUIMainFrameWindow::CNewUIMainFrameWindow()
{
	m_bExpEffect = false;
	m_dwExpEffectTime = 0;
	m_dwPreExp = 0;
	m_dwGetExp = 0;
	m_bButtonBlink = false;
}

SEASON3B::CNewUIMainFrameWindow::~CNewUIMainFrameWindow()
{
	Release();
}

void SEASON3B::CNewUIMainFrameWindow::LoadImages()
{
#if CLIENT_VERSION >= 1001
	//newhudvictoriuri - V19: LoadBitmap recebe .tga e o loader abre automaticamente o .OZT correspondente.
	// Os arquivos V19 sao OZT validos (header OZT + 32-bit BGRA), evitando LoadBitmap Failed.
	//newhudvictoriuri - Nao passar .OZT diretamente ao LoadBitmap: isso causa LoadBitmap Failed mesmo com o arquivo presente.
	// O loader antigo do cliente nao interpreta corretamente TGA tipo 10 (RLE);
	// isso causava as faixas coloridas/corrompidas dentro de HP/Mana/SD/AG.
	//newhudvictoriuri - V24.9: usa diretamente o pack original Data.zip.
	// O loader recebe .tga e abre o OZT homonimo. A arte e montada em cinco
	// pecas para nao esticar/cortar o PSD inteiro.
	LoadBitmap("Custom\\Interface\\HUD_MuAway\\menu01.tga", NEW_FRAME, GL_LINEAR, GL_CLAMP_TO_EDGE);
	LoadBitmap("Custom\\Interface\\HUD_MuAway\\menu02.tga", IMAGE_MENU_1, GL_LINEAR, GL_CLAMP_TO_EDGE);
	LoadBitmap("Custom\\Interface\\HUD_MuAway\\menu03.tga", IMAGE_MENU_2, GL_LINEAR, GL_CLAMP_TO_EDGE);
	LoadBitmap("Custom\\Interface\\HUD_MuAway\\menu01_new_wide.tga", IMAGE_MENU_3, GL_LINEAR, GL_CLAMP_TO_EDGE);
	LoadBitmap("Custom\\Interface\\HUD_MuAway\\menu03_new_wide.tga", IMAGE_MENU_2_1, GL_LINEAR, GL_CLAMP_TO_EDGE);
	LoadBitmap("Custom\\Interface\\HUD_MuAway\\menu_blue.tga", IMAGE_GAUGE_BLUE, GL_LINEAR, GL_CLAMP_TO_EDGE); //newhudvictoriuri
	LoadBitmap("Custom\\Interface\\HUD_MuAway\\menu_green.tga", IMAGE_GAUGE_GREEN, GL_LINEAR, GL_CLAMP_TO_EDGE); //newhudvictoriuri
	LoadBitmap("Custom\\Interface\\HUD_MuAway\\menu_red.tga", IMAGE_GAUGE_RED, GL_LINEAR, GL_CLAMP_TO_EDGE); //newhudvictoriuri
	LoadBitmap("Custom\\Interface\\HUD_MuAway\\menu03_new_ag.tga", IMAGE_GAUGE_AG, GL_LINEAR, GL_CLAMP_TO_EDGE); //newhudvictoriuri
	LoadBitmap("Custom\\Interface\\HUD_MuAway\\menu01_new2_SD.tga", IMAGE_GAUGE_SD, GL_LINEAR, GL_CLAMP_TO_EDGE); //newhudvictoriuri
	LoadBitmap("Custom\\Interface\\HUD_MuAway_V24\\hud_exp_exact.tga", IMAGE_GAUGE_EXBAR, GL_LINEAR, GL_CLAMP_TO_EDGE); //newhudvictoriuri

	//newhudvictoriuri - icones individuais do pack, uma celula 32x32 cada.
	LoadBitmap("Custom\\Interface\\HUD_MuAway\\menu_party.tga", IMAGE_MENU_BTN_WINDOW, GL_LINEAR, GL_CLAMP_TO_EDGE);
	LoadBitmap("Custom\\Interface\\HUD_MuAway\\menu_character.tga", IMAGE_MENU_BTN_CHAINFO, GL_LINEAR, GL_CLAMP_TO_EDGE);
	LoadBitmap("Custom\\Interface\\HUD_MuAway\\menu_inventory.tga", IMAGE_MENU_BTN_MYINVEN, GL_LINEAR, GL_CLAMP_TO_EDGE);
	//newhudvictoriuri - V26: nao carrega os quadros pressed do pack V24;
	// eles sao quadros completos sem o alpha esperado pelo renderer da HUD.

	LoadBitmap("Interface\\newui_SW_Minimap_Frame.tga", 31602, GL_LINEAR);

#elif CLIENT_VERSION >= 401
	LoadBitmap("Interface\\newui_menu01.jpg", IMAGE_MENU_1, GL_LINEAR);
	LoadBitmap("Interface\\newui_menu02.jpg", IMAGE_MENU_2, GL_LINEAR);
	LoadBitmap("Interface\\partCharge1\\newui_menu03.jpg", IMAGE_MENU_3, GL_LINEAR);
	LoadBitmap("Interface\\newui_menu02-03.jpg", IMAGE_MENU_2_1, GL_LINEAR);
	LoadBitmap("Interface\\newui_menu_blue.jpg", IMAGE_GAUGE_BLUE, GL_LINEAR);
	LoadBitmap("Interface\\newui_menu_green.jpg", IMAGE_GAUGE_GREEN, GL_LINEAR);
	LoadBitmap("Interface\\newui_menu_red.jpg", IMAGE_GAUGE_RED, GL_LINEAR);
	LoadBitmap("Interface\\newui_menu_ag.jpg", IMAGE_GAUGE_AG, GL_LINEAR);
	LoadBitmap("Interface\\newui_menu_sd.jpg", IMAGE_GAUGE_SD, GL_LINEAR);
	LoadBitmap("Interface\\newui_exbar.jpg", IMAGE_GAUGE_EXBAR, GL_LINEAR);
	LoadBitmap("Interface\\Exbar_Master.jpg", IMAGE_MASTER_GAUGE_BAR, GL_LINEAR);
	LoadBitmap("Interface\\partCharge1\\newui_menu_Bt05.jpg", IMAGE_MENU_BTN_CSHOP, GL_LINEAR, GL_CLAMP_TO_EDGE);
	LoadBitmap("Interface\\partCharge1\\newui_menu_Bt01.jpg", IMAGE_MENU_BTN_CHAINFO, GL_LINEAR, GL_CLAMP_TO_EDGE);
	LoadBitmap("Interface\\partCharge1\\newui_menu_Bt02.jpg", IMAGE_MENU_BTN_MYINVEN, GL_LINEAR, GL_CLAMP_TO_EDGE);
	LoadBitmap("Interface\\partCharge1\\newui_menu_Bt03.jpg", IMAGE_MENU_BTN_FRIEND, GL_LINEAR, GL_CLAMP_TO_EDGE);
	LoadBitmap("Interface\\partCharge1\\newui_menu_Bt04.jpg", IMAGE_MENU_BTN_WINDOW, GL_LINEAR, GL_CLAMP_TO_EDGE);
	LoadBitmap("Interface\\newui_decor.tga", IMAGE_CUSTOM_ID, GL_LINEAR);
#elif CLIENT_VERSION <= 302
	//LOAD IMAGES OLD
#endif
}

void SEASON3B::CNewUIMainFrameWindow::UnloadImages()
{
#if  CLIENT_VERSION >= 1001
	DeleteBitmap(NEW_FRAME);
	DeleteBitmap(IMAGE_MENU_1);
	DeleteBitmap(IMAGE_MENU_2);
	//newhudvictoriuri - Partes adicionais usadas pela montagem da HUD MuAway.
	DeleteBitmap(IMAGE_MENU_3);
	DeleteBitmap(IMAGE_MENU_2_1);
	DeleteBitmap(IMAGE_GAUGE_BLUE);
	DeleteBitmap(IMAGE_GAUGE_GREEN);
	DeleteBitmap(IMAGE_GAUGE_RED);
	DeleteBitmap(IMAGE_GAUGE_AG);
	DeleteBitmap(IMAGE_GAUGE_SD);
	DeleteBitmap(IMAGE_GAUGE_EXBAR); //newhudvictoriuri - V13
	DeleteBitmap(IMAGE_MENU_BTN_CHAINFO);
	DeleteBitmap(IMAGE_MENU_BTN_MYINVEN);
	DeleteBitmap(IMAGE_MENU_BTN_WINDOW);
#elif CLIENT_VERSION >= 401
	DeleteBitmap(IMAGE_MENU_1);
	DeleteBitmap(IMAGE_MENU_2);
	DeleteBitmap(IMAGE_MENU_3);
	DeleteBitmap(IMAGE_MENU_2_1);
	DeleteBitmap(IMAGE_GAUGE_BLUE);
	DeleteBitmap(IMAGE_GAUGE_GREEN);
	DeleteBitmap(IMAGE_GAUGE_RED);
	DeleteBitmap(IMAGE_GAUGE_AG);
	DeleteBitmap(IMAGE_GAUGE_SD);
	DeleteBitmap(IMAGE_GAUGE_EXBAR);
	DeleteBitmap(IMAGE_MENU_BTN_CHAINFO);
	DeleteBitmap(IMAGE_MENU_BTN_MYINVEN);
	DeleteBitmap(IMAGE_MENU_BTN_FRIEND);
	DeleteBitmap(IMAGE_MENU_BTN_WINDOW);
#endif
}

bool SEASON3B::CNewUIMainFrameWindow::Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng)
{
	if (NULL == pNewUIMng || NULL == pNewUI3DRenderMng)
		return false;

	m_pNewUIMng = pNewUIMng;
	m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_MAINFRAME, this);

	m_pNewUI3DRenderMng = pNewUI3DRenderMng;
	m_pNewUI3DRenderMng->Add3DRenderObj(this, ITEMHOTKEYNUMBER_CAMERA_Z_ORDER);

	LoadImages();

#if CLIENT_VERSION >= 1001
	NewHudEditorLoad(); //newhudvictoriuri - carrega o ultimo layout antes de criar as hitboxes.
#endif
	SetButtonInfo();

	Show(true);

	return true;
}

void SEASON3B::CNewUIMainFrameWindow::SetButtonInfo()
{
#if  CLIENT_VERSION >= 401 && CLIENT_VERSION <= 502
	int x_Next = gWideScreen.GetCenterPosition(489);
	int y_Next = 480 - 51;
	int x_Add = 30;
	int y_Add = 41;
	m_BtnCShop.ChangeTextBackColor(RGBA(255, 255, 255, 0));
	m_BtnCShop.ChangeButtonImgState(true, IMAGE_MENU_BTN_CSHOP, true);
	m_BtnCShop.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
	x_Next += x_Add;
	m_BtnCShop.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
	m_BtnCShop.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
	m_BtnCShop.ChangeToolTipText(GlobalText[2277], true);

	m_BtnChaInfo.ChangeTextBackColor(RGBA(255, 255, 255, 0));
	m_BtnChaInfo.ChangeButtonImgState(true, IMAGE_MENU_BTN_CHAINFO, true);
	m_BtnChaInfo.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
	x_Next += x_Add;
	m_BtnChaInfo.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
	m_BtnChaInfo.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
	m_BtnChaInfo.ChangeToolTipText(GlobalText[362], true);

	m_BtnMyInven.ChangeTextBackColor(RGBA(255, 255, 255, 0));
	m_BtnMyInven.ChangeButtonImgState(true, IMAGE_MENU_BTN_MYINVEN, true);
	m_BtnMyInven.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
	x_Next += x_Add;
	m_BtnMyInven.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
	m_BtnMyInven.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
	m_BtnMyInven.ChangeToolTipText(GlobalText[363], true);

	m_BtnFriend.ChangeTextBackColor(RGBA(255, 255, 255, 0));
	m_BtnFriend.ChangeButtonImgState(true, IMAGE_MENU_BTN_FRIEND, true);
	m_BtnFriend.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
	x_Next += x_Add;
	m_BtnFriend.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
	m_BtnFriend.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
	m_BtnFriend.ChangeToolTipText(GlobalText[1043], true);

	m_BtnWindow.ChangeTextBackColor(RGBA(255, 255, 255, 0));
	m_BtnWindow.ChangeButtonImgState(true, IMAGE_MENU_BTN_WINDOW, true);
	m_BtnWindow.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
	m_BtnWindow.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
	m_BtnWindow.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
	m_BtnWindow.ChangeToolTipText(GlobalText[1744], true);
#elif CLIENT_VERSION <= 302
	int x_Next = gWideScreen.GetCenterPosition(489);
	int y_Next = 480 - 51;
	int x_Add = 30;
	int y_Add = 41;

	m_BtnCShop.ChangeTextBackColor(RGBA(255, 255, 255, 0));
	m_BtnCShop.ChangeButtonImgState(true, IMAGE_MENU_BTN_CSHOP, true);
	m_BtnCShop.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
	x_Next += x_Add;
	m_BtnCShop.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
	m_BtnCShop.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
	m_BtnCShop.ChangeToolTipText(GlobalText[2277], true);

	m_BtnChaInfo.ChangeTextBackColor(RGBA(255, 255, 255, 0));
	m_BtnChaInfo.ChangeButtonImgState(true, IMAGE_MENU_BTN_CHAINFO, true);
	m_BtnChaInfo.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
	x_Next += x_Add;
	m_BtnChaInfo.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
	m_BtnChaInfo.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
	m_BtnChaInfo.ChangeToolTipText(GlobalText[362], true);

	m_BtnMyInven.ChangeTextBackColor(RGBA(255, 255, 255, 0));
	m_BtnMyInven.ChangeButtonImgState(true, IMAGE_MENU_BTN_MYINVEN, true);
	m_BtnMyInven.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
	x_Next += x_Add;
	m_BtnMyInven.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
	m_BtnMyInven.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
	m_BtnMyInven.ChangeToolTipText(GlobalText[363], true);

	m_BtnFriend.ChangeTextBackColor(RGBA(255, 255, 255, 0));
	m_BtnFriend.ChangeButtonImgState(true, IMAGE_MENU_BTN_FRIEND, true);
	m_BtnFriend.ChangeButtonInfo(x_Next, y_Next, x_Add, y_Add);
	x_Next += x_Add;
	m_BtnFriend.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
	m_BtnFriend.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
	m_BtnFriend.ChangeToolTipText(GlobalText[1043], true);
#elif CLIENT_VERSION >= 1001
	//newhudvictoriuri - V13: hitboxes dos botoes nas coordenadas EXATAS do PSD.
	// Camadas normais: X=1012,1070,1128,1186 / Y=1028 / 45x45. Sem animacao visual.
	//newhudvictoriuri - V24.6.5: hitboxes calculadas pelas celulas 32x32 reais do HUD_MuAway.zip.
	const int partyY = (int)NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_BUTTON_PARTY)); //newhudvictoriuri
	const int charY = (int)NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_BUTTON_CHARACTER)); //newhudvictoriuri
	const int invenY = (int)NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_BUTTON_INVENTORY)); //newhudvictoriuri
	const int partyW = (int)NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_BUTTON_PARTY)); //newhudvictoriuri
	const int partyH = (int)NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_BUTTON_PARTY)); //newhudvictoriuri
	const int charW = (int)NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_BUTTON_CHARACTER)); //newhudvictoriuri
	const int charH = (int)NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_BUTTON_CHARACTER)); //newhudvictoriuri
	const int invenW = (int)NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_BUTTON_INVENTORY)); //newhudvictoriuri
	const int invenH = (int)NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_BUTTON_INVENTORY)); //newhudvictoriuri
	const int partyX = (int)NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_BUTTON_PARTY)); //newhudvictoriuri
	const int charX = (int)NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_BUTTON_CHARACTER)); //newhudvictoriuri
	const int invenX = (int)NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_BUTTON_INVENTORY)); //newhudvictoriuri

	m_BtnParty.ChangeTextBackColor(RGBA(255, 255, 255, 0));
	m_BtnParty.ChangeButtonImgState(true, IMAGE_MENU_BTN_WINDOW, true);
	m_BtnParty.ChangeButtonInfo(partyX, partyY, partyW, partyH);
	m_BtnParty.SetSize(partyW, partyH);
	//newhudvictoriuri - todos os estados usam o icone com alpha correto;
	// o feedback visual de pressionado e desenhado em RenderButtons().
	m_BtnParty.UnRegisterButtonState();
	m_BtnParty.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_WINDOW, 0);
	m_BtnParty.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_WINDOW, 0);
	m_BtnParty.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_WINDOW, 0); //newhudvictoriuri
	m_BtnParty.ChangeImgColor(BUTTON_STATE_UP, RGBA(235, 235, 235, 255)); //newhudvictoriuri - V20
	m_BtnParty.ChangeImgColor(BUTTON_STATE_OVER, RGBA(255, 242, 198, 255)); //newhudvictoriuri - V20
	m_BtnParty.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255)); //newhudvictoriuri
	m_BtnParty.ChangeImgIndex(IMAGE_MENU_BTN_WINDOW, 0);
	m_BtnParty.ChangeToolTipText(GlobalText[361], true);
	if (NewHudEditorVisible(HUD_EDITOR_BUTTON_PARTY)) m_BtnParty.UnLock(); else m_BtnParty.Lock(); //newhudvictoriuri

	m_BtnChaInfo.ChangeTextBackColor(RGBA(255, 255, 255, 0));
	m_BtnChaInfo.ChangeButtonImgState(true, IMAGE_MENU_BTN_CHAINFO, true, false, true);
	m_BtnChaInfo.ChangeButtonInfo(charX, charY, charW, charH);
	m_BtnChaInfo.SetSize(charW, charH);
	//newhudvictoriuri - Use the same source frame for up/over/down on the static icon.
	m_BtnChaInfo.UnRegisterButtonState();
	m_BtnChaInfo.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_CHAINFO, 0);
	m_BtnChaInfo.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_CHAINFO, 0);
	m_BtnChaInfo.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_CHAINFO, 0); //newhudvictoriuri
	m_BtnChaInfo.ChangeImgColor(BUTTON_STATE_UP, RGBA(235, 235, 235, 255)); //newhudvictoriuri - V20
	m_BtnChaInfo.ChangeImgColor(BUTTON_STATE_OVER, RGBA(255, 242, 198, 255)); //newhudvictoriuri - V20
	m_BtnChaInfo.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255)); //newhudvictoriuri
	m_BtnChaInfo.ChangeImgIndex(IMAGE_MENU_BTN_CHAINFO, 0);
	m_BtnChaInfo.ChangeToolTipText(GlobalText[362], true);
	if (NewHudEditorVisible(HUD_EDITOR_BUTTON_CHARACTER)) m_BtnChaInfo.UnLock(); else m_BtnChaInfo.Lock(); //newhudvictoriuri

	m_BtnMyInven.ChangeTextBackColor(RGBA(255, 255, 255, 0));
	m_BtnMyInven.ChangeButtonImgState(true, IMAGE_MENU_BTN_MYINVEN, true);
	m_BtnMyInven.ChangeButtonInfo(invenX, invenY, invenW, invenH);
	m_BtnMyInven.SetSize(invenW, invenH);
	//newhudvictoriuri - Inventory icon is also a single-frame texture.
	m_BtnMyInven.UnRegisterButtonState();
	m_BtnMyInven.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_MYINVEN, 0);
	m_BtnMyInven.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_MYINVEN, 0);
	m_BtnMyInven.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_MYINVEN, 0); //newhudvictoriuri
	m_BtnMyInven.ChangeImgColor(BUTTON_STATE_UP, RGBA(235, 235, 235, 255)); //newhudvictoriuri - V20
	m_BtnMyInven.ChangeImgColor(BUTTON_STATE_OVER, RGBA(255, 242, 198, 255)); //newhudvictoriuri - V20
	m_BtnMyInven.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255)); //newhudvictoriuri
	m_BtnMyInven.ChangeImgIndex(IMAGE_MENU_BTN_MYINVEN, 0);
	m_BtnMyInven.ChangeToolTipText(GlobalText[363], true);
	if (NewHudEditorVisible(HUD_EDITOR_BUTTON_INVENTORY)) m_BtnMyInven.UnLock(); else m_BtnMyInven.Lock(); //newhudvictoriuri
#endif
}

void SEASON3B::CNewUIMainFrameWindow::Release()
{
	UnloadImages();

	if (m_pNewUI3DRenderMng)
	{
		m_pNewUI3DRenderMng->Remove3DRenderObj(this);
		m_pNewUI3DRenderMng = NULL;
	}

	if (m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj(this);
		m_pNewUIMng = NULL;
	}
}

bool SEASON3B::CNewUIMainFrameWindow::Render()
{
	EnableAlphaTest();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	RenderFrame();

	g_pSkillList->RenderCurrentSkillAndHotSkillList();

	EnableAlphaTest();
	RenderLifeMana();
	RenderGuageSD();
	RenderGuageAG();
	RenderButtons();
#if CLIENT_VERSION >= 401 && CLIENT_VERSION < 502
	RenderExperience();
#elif CLIENT_VERSION >= 1001
	RenderExperience(); //newhudvictoriuri - V13: experiencia visivel tambem nesta base.
#endif

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	g_pNewUISystem->m_Lua.Generic_Call("MainProc", ">");
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

#if CLIENT_VERSION >= 1001
	//newhudvictoriuri - V24.6.6: MainProc pode desligar o GL_BLEND/alpha test.
	// Reativar o alpha ANTES do segundo repaint evita que os pixels transparentes
	// do HUD_MuAway aparecam como um retangulo preto. Nao desativamos aqui porque
	// Mana/HP/SD/AG e os demais elementos seguintes tambem usam transparência.
	EnableAlphaTest(true);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	//newhudvictoriuri - V24.9: repaint completo apos MainProc. A base montada e os
	// elementos dinamicos sao redesenhados juntos para manter o pack sincronizado.
	RenderFrame();
	g_pSkillList->RenderCurrentSkillAndHotSkillList();
	RenderLifeMana();
	RenderGuageSD();
	RenderGuageAG();
	RenderButtons();
	RenderExperience();
#endif

	m_pNewUI3DRenderMng->RenderUI2DEffect(ITEMHOTKEYNUMBER_CAMERA_Z_ORDER, UI2DEffectCallback, this, 0, 0);
#if CLIENT_VERSION >= 1001
	NewHudEditorRender(); //newhudvictoriuri - overlay do editor precisa ser o ultimo desenho da HUD.
#endif
	/*
	if (!g_pCharacterInfoWindow->IsVisible() && !g_pMyInventory->IsVisible()
		&& !g_pGuildInfoWindow->IsVisible() && !g_pWindowMgr->IsVisible()
		&& !g_pPartyInfoWindow->IsVisible() && !g_pMyQuestInfoWindow->IsVisible()
		&& !g_pCommandWindow->IsVisible())
	{
		DeleteBitmap(31604);
		unicode::t_char szText[256] = { 0, };
		unicode::_sprintf(szText, "Custom\\MiniMap\\Map%d.jpg", gMapManager.WorldActive + 1);
		LoadBitmap(szText, 31604, GL_LINEAR);


		int m_MiniMapPosx = gWideScreen.g_WideWindowWidth - 154;
		int m_MiniMapPosy = 280;

		int m_HeroPosInWorldx = (Hero->PositionX) / 1;
		int m_HeroPosInWorldy = (256 - (Hero->PositionY)) / 1;

		int m_MiniMapScaleOffsetx = max((m_HeroPosInWorldx - (64 * 1)), 0);
		int m_MiniMapScaleOffsety = min(max((m_HeroPosInWorldy - (64 * 1)), 0), 128);

		int m_HeroPosInMiniMapx = m_HeroPosInWorldx - m_MiniMapScaleOffsetx + m_MiniMapPosx + 20;
		int m_HeroPosInMiniMapy = m_HeroPosInWorldy - m_MiniMapScaleOffsety + m_MiniMapPosy;

		float m_fMiniMapTexU = (float)(m_MiniMapScaleOffsetx) / (256.f / (float)1);
		float m_fMiniMapTexV = (float)(m_MiniMapScaleOffsety) / (256.f / (float)1);



		EnableAlphaTest();
		glColor4f(1.f, 1.f, 1.f, 1.0f);
		g_pRenderText->SetFont(g_hFontBold);
		g_pRenderText->SetTextColor(255, 255, 255, 255);
		g_pRenderText->SetBgColor(0, 0, 0, 0);

		RenderBitmap(31604, (float)(m_MiniMapPosx + 20), (float)(m_MiniMapPosy), 128.f, 128.f, m_fMiniMapTexU, m_fMiniMapTexV, 0.5f * 1, 0.5f * 1);

		RenderImage(31602, m_MiniMapPosx, m_MiniMapPosy - 21, 154, 162);
		//RenderImage(31603, m_TimeUIPos.x, m_TimeUIPos.y, 134, 37);

		glColor4f(1.f, 1.f, 1.f, 1.0f);


		glColor4f(1.f, 1.f, 0.f, 1.0f);
		RenderColor((float)(m_HeroPosInMiniMapx), (float)(m_HeroPosInMiniMapy), 3, 3);
	}
	*/




	DisableAlphaBlend();

	return true;
}

void SEASON3B::CNewUIMainFrameWindow::Render3D()
{
	m_ItemHotKey.RenderItems();
}

void SEASON3B::CNewUIMainFrameWindow::UI2DEffectCallback(LPVOID pClass, DWORD dwParamA, DWORD dwParamB)
{
	g_pMainFrame->RenderHotKeyItemCount();

}

bool SEASON3B::CNewUIMainFrameWindow::IsVisible() const
{
	return CNewUIObj::IsVisible();
}

void SEASON3B::CNewUIMainFrameWindow::RenderFrame()
{

#if CLIENT_VERSION >= 402 && CLIENT_VERSION <= 502
	float width, height;
	float x, y;
	width = 256.f; height = 51.f;
	x = gWideScreen.GetCenterPosition(0); y = 480.f - height;
	SEASON3B::RenderImage(IMAGE_MENU_1, x, y, width, height);
	RenderBitmap(IMAGE_CUSTOM_ID, x - 70.28048780487805, y, 70.28048780487805, height, 0.0, 0.0, 0.8785000443, 0.6409999728, 1, 1, 0.0);
	width = 128.f;
	x += 256;
	SEASON3B::RenderImage(IMAGE_MENU_2, x, y, width, height);
	width = 256.f;
	x += 128;
	SEASON3B::RenderImage(IMAGE_MENU_3, x, y, width, height);
	RenderBitmap(IMAGE_CUSTOM_ID, x + width, y, 70.28048780487805, height, 0.8785000443, 0.0, -0.8785000443, 0.6409999728, 1, 1, 0.0);

	if (g_pSkillList->IsSkillListUp() == true)
	{
		width = 160.f; height = 40.f;
		x = gWideScreen.GetCenterPosition(222);
		SEASON3B::RenderImage(IMAGE_MENU_2_1, x, y, width, height);
	}
#endif

#if CLIENT_VERSION >= 1001
	//newhudvictoriuri - V24.9: as pecas originais do Data.zip sao montadas no
	// retangulo FRAME definido no HUDLayout.ini.
	if (NewHudEditorVisible(HUD_EDITOR_FRAME)) //newhudvictoriuri - editor controla visibilidade/tamanho do frame.
	{
		NewHudVictoriuriRenderPackedFrame(
			NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_FRAME)),
			NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_FRAME)),
			NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_FRAME)),
			NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_FRAME)));
	}

	g_pRenderText->SetBgColor(0, 0, 0, 0);
	if (Hero != NULL)
	{
		unicode::t_char fpsValue[16] = { 0 };
		unicode::_sprintf(fpsValue, "%d", (int)max(0.0, min(999.0, FPS_AVG)));
		g_pRenderText->SetFont(g_hFixFont);
		g_pRenderText->SetTextColor(235, 235, 235, 255);
		//newhudvictoriuri - V24.7: texto vivo sincronizado com a referencia final.
		// A base ja contem o rotulo "FPS:"; somente o valor entra no quadro
		// X432..469 / Y1008..1025, sem cobrir o rotulo.
		if (NewHudEditorVisible(HUD_EDITOR_FPS)) //newhudvictoriuri
		{
			g_pRenderText->RenderText((int)NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_FPS)),
				(int)NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_FPS)),
				fpsValue, (int)NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_FPS)), 0, RT3_WRITE_CENTER);
		}

		//newhudvictoriuri - As coordenadas ocupam duas celulas da referencia:
		// X386..423 e Y432..469. O editor move o grupo inteiro mantendo esse vao.
		unicode::t_char posXText[12] = { 0 };
		unicode::t_char posYText[12] = { 0 };
		unicode::_sprintf(posXText, "%d", Hero->PositionX);
		unicode::_sprintf(posYText, "%d", Hero->PositionY);
		g_pRenderText->SetTextColor(235, 235, 235, 255);
		if (NewHudEditorVisible(HUD_EDITOR_COORDS)) //newhudvictoriuri
		{
			const float coordsX = NewHudEditorX(HUD_EDITOR_COORDS); //newhudvictoriuri - layout vindo do INI
			const float coordsY = NewHudEditorY(HUD_EDITOR_COORDS);
			const float cellWidth = NewHudVictoriuriSize(37.0f);
			g_pRenderText->RenderText((int)NewHudVictoriuriX(coordsX),
				(int)NewHudVictoriuriY(coordsY),
				posXText, (int)cellWidth, 0, RT3_WRITE_CENTER);
			g_pRenderText->RenderText((int)NewHudVictoriuriX(coordsX + 46.0f),
				(int)NewHudVictoriuriY(coordsY),
				posYText, (int)cellWidth, 0, RT3_WRITE_CENTER);
		}
		g_pRenderText->SetFont(g_hFont);
	}
#endif

	//RenderBitmap(600016, gWideScreen.GetAddPosition(345), 450, 25, 24, 0.008, 0.002, 0.83, 0.20, 1, 1, 1.0);
	//RenderBitmap(600018, gWideScreen.GetAddPosition(345) + 31.5, 450, 25, 24, 0.008, 0.002, 0.83, 0.20, 1, 1, 1.0);
	//RenderBitmap(600017, gWideScreen.GetAddPosition(345) + 31.5 + 31.5, 450, 25, 24, 0.008, 0.002, 0.83, 0.20, 1, 1, 1.0);
	//RenderBitmap(600020, 92 + addw, 458, 16, 16, 0.008, 0.002, 0.83, 0.20, 1, 1, 1.0);
}

void SEASON3B::CNewUIMainFrameWindow::RenderLifeMana()
{
	DWORD wLifeMax, wLife, wManaMax, wMana;


	if (gCharacterManager.IsMasterLevel(Hero->Class) == true)
	{
		wLifeMax = Master_Level_Data.wMaxLife;
		wLife = min(max(0, CharacterAttribute->Life), wLifeMax);
		wManaMax = Master_Level_Data.wMaxMana;
		wMana = min(max(0, CharacterAttribute->Mana), wManaMax);
	}
	else
	{
		wLifeMax = CharacterAttribute->LifeMax;
		wLife = min(max(0, CharacterAttribute->Life), wLifeMax);
		wManaMax = CharacterAttribute->ManaMax;
		wMana = min(max(0, CharacterAttribute->Mana), wManaMax);
	}

	if (wLifeMax > 0)
	{
		if (wLife > 0 && (wLife / (float)wLifeMax) < 0.2f)
		{
			PlayBuffer(SOUND_HEART);
		}
	}

	float fLife = 0.f;
	float fMana = 0.f;

	if (wLifeMax > 0)
	{
		fLife = (wLifeMax - wLife) / (float)wLifeMax;
	}
	if (wManaMax > 0)
	{
		fMana = (wManaMax - wMana) / (float)wManaMax;
	}

	float width, height;
	float x, y;
	float fY, fH, fV;

	// life
#if CLIENT_VERSION >= 402 && CLIENT_VERSION <= 502
	width = 45.f;
	x = gWideScreen.GetCenterPosition(158);
	height = 39.f;
	y = 480.f - 48.f;
#elif CLIENT_VERSION >= 1001
	//newhudvictoriuri - V23: HP usa novamente a camada ORIGINAL EXATA do PSD
	// (99x89 em X546 Y991). O asset V15/V23 e pixel-identico a referencia enviada.
	//newhudvictoriuri - V24.6.5: orb vermelha segue o menu_red.OZT original (64x64) encaixado na base enviada.
	//newhudvictoriuri - V24.7: camada sincronizada com a referencia final (99x89 em X546 Y991).
	width = NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_HP_GAUGE)); //newhudvictoriuri
	x = NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_HP_GAUGE)); //newhudvictoriuri
	height = NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_HP_GAUGE)); //newhudvictoriuri
	y = NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_HP_GAUGE)); //newhudvictoriuri
#endif

	// 45, 45, 0, 0, 0.7906, 0.760600, 1, 1, 1.0

	fY = y + (fLife * height);
	fH = height - (fLife * height);
	fV = fLife;
	//newhudvictoriuri - MuAway ships static gauge strips; do not reference the old animated atlases.
	bool animated = false;

	if (g_isCharacterBuff((&Hero->Object), eDeBuff_Poison))
	{
#if CLIENT_VERSION >= 402 && CLIENT_VERSION <= 502
		RenderBitmap(IMAGE_GAUGE_GREEN, x, fY, width, fH, 0.f, fV * height / 64.f, width / 64.f, (1.0f - fV) * height / 64.f);
#elif CLIENT_VERSION >= 1001
		//newhudvictoriuri - V15: recorte vertical do Poison com alpha transparente; preserva a moldura/base do PSD sem fundo preto.
		if (NewHudEditorVisible(HUD_EDITOR_HP_GAUGE)) //newhudvictoriuri
			NewHudVictoriuriRenderVerticalGauge(IMAGE_GAUGE_GREEN, x, y, width, height, fV, 53.0f, 48.0f); //newhudvictoriuri
#endif
	}
	else
	{
#if CLIENT_VERSION >= 402 && CLIENT_VERSION <= 502
		RenderBitmap(IMAGE_GAUGE_RED, x, fY, width, fH, 0.f, fV * height / 64.f, width / 64.f, (1.0f - fV) * height / 64.f);
#elif CLIENT_VERSION >= 1001
		//newhudvictoriuri - V15: recorte vertical da HP com alpha transparente; preserva a moldura/base do PSD sem fundo preto.
		if (NewHudEditorVisible(HUD_EDITOR_HP_GAUGE)) //newhudvictoriuri
			NewHudVictoriuriRenderVerticalGauge(IMAGE_GAUGE_RED, x, y, width, height, fV, 53.0f, 48.0f); //newhudvictoriuri
#endif
	}
#if CLIENT_VERSION >= 402 && CLIENT_VERSION <= 502
	SEASON3B::RenderNumber(x + 25, 480 - 18, wLife);
#endif

#if CLIENT_VERSION >= 1001
	glColor3f(1.0f, 0.81f, 0.55f);
	//newhudvictoriuri - V24.7: valor HP centralizado sob a orb, no mesmo tamanho
	// do exemplo da referencia. A chamada usa X600 porque o sprite antigo avanca
	// cada digito em 0.8 largura; assim o grupo visivel fica centrado em X595.
	if (NewHudEditorVisible(HUD_EDITOR_HP_NUMBER)) //newhudvictoriuri
	{
		SEASON3B::RenderOldNumber(
			NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_HP_NUMBER)),
			NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_HP_NUMBER)),
			wLife, NewHudEditorScale(HUD_EDITOR_HP_NUMBER) * NewHudVictoriuriReadableNumberScale(1.0f)); //newhudvictoriuri - escala final vem do INI.
	}
	glColor3f(1.0f, 1.0f, 1.0f);
#endif

	char strTipText[256];
	//newhudvictoriuri - V23: range/hitbox do HP acompanha o range EXATO da camada 99x89.
	const float lifeHitX = x;
	const float lifeHitY = y;
	const float lifeHitW = width;
	const float lifeHitH = height;
#if CLIENT_VERSION >= 1001
	if (NewHudEditorVisible(HUD_EDITOR_HP_GAUGE)
		&& SEASON3B::CheckMouseIn((int)lifeHitX, (int)lifeHitY, (int)lifeHitW, (int)lifeHitH) == true)
#else
	if (SEASON3B::CheckMouseIn((int)lifeHitX, (int)lifeHitY, (int)lifeHitW, (int)lifeHitH) == true)
#endif
	{
		sprintf(strTipText, GlobalText[358], wLife, wLifeMax);
		RenderTipText((int)lifeHitX, (int)gWideScreen.GetBottomPosition(418), strTipText);
	}

	// mana


#if CLIENT_VERSION >= 402 && CLIENT_VERSION <= 502
	width = 45.f;
	x = gWideScreen.GetCenterPosition(256.f + 128.f + 53.f);
	height = 39.f;
	y = 480.f - 48.f;
#endif

#if CLIENT_VERSION >= 1001
	//newhudvictoriuri - V23: Mana usa novamente a camada ORIGINAL EXATA do PSD
	// (99x89 em X1273 Y991). O asset V15/V23 e pixel-identico a referencia enviada.
	//newhudvictoriuri - V24.7: camada sincronizada com a referencia final (99x89 em X1273 Y991).
	width = NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_MP_GAUGE)); //newhudvictoriuri
	x = NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_MP_GAUGE)); //newhudvictoriuri
	height = NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_MP_GAUGE)); //newhudvictoriuri
	y = NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_MP_GAUGE)); //newhudvictoriuri
#endif

	fY = y + (fMana * height);
	fH = height - (fMana * height);
	fV = fMana;
#if CLIENT_VERSION >= 402 && CLIENT_VERSION <= 502
	RenderBitmap(IMAGE_GAUGE_BLUE, x, fY, width, fH, 0.f, fV * height / 64.f, width / 64.f, (1.0f - fV) * height / 64.f);
#elif CLIENT_VERSION >= 1001
	//newhudvictoriuri - V15: recorte vertical da Mana com alpha transparente; preserva a moldura/base do PSD sem fundo preto.
	if (NewHudEditorVisible(HUD_EDITOR_MP_GAUGE)) //newhudvictoriuri
		NewHudVictoriuriRenderVerticalGauge(IMAGE_GAUGE_BLUE, x, y, width, height, fV, 53.0f, 48.0f); //newhudvictoriuri
#endif


#if CLIENT_VERSION >= 402 && CLIENT_VERSION <= 502
	SEASON3B::RenderNumber(x + 30, 480 - 18, wMana);
#endif

#if CLIENT_VERSION >= 1001
	glColor3f(1.0f, 0.81f, 0.55f);
	//newhudvictoriuri - V24.7: valor MP centralizado sob a orb, no mesmo tamanho
	// do exemplo da referencia. A chamada usa X1327 para compensar o espacamento
	// interno do sprite e manter o grupo visivel centrado em X1322.
	if (NewHudEditorVisible(HUD_EDITOR_MP_NUMBER)) //newhudvictoriuri
	{
		SEASON3B::RenderOldNumber(
			NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_MP_NUMBER)),
			NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_MP_NUMBER)),
			wMana, NewHudEditorScale(HUD_EDITOR_MP_NUMBER) * NewHudVictoriuriReadableNumberScale(1.0f)); //newhudvictoriuri - escala final vem do INI.
	}
	glColor3f(1.0f, 1.0f, 1.0f);
#endif

	// mana
	//newhudvictoriuri - V23: range/hitbox da Mana acompanha o range EXATO da camada 99x89.
	const float manaHitX = x;
	const float manaHitY = y;
	const float manaHitW = width;
	const float manaHitH = height;
#if CLIENT_VERSION >= 1001
	if (NewHudEditorVisible(HUD_EDITOR_MP_GAUGE)
		&& SEASON3B::CheckMouseIn((int)manaHitX, (int)manaHitY, (int)manaHitW, (int)manaHitH) == true)
#else
	if (SEASON3B::CheckMouseIn((int)manaHitX, (int)manaHitY, (int)manaHitW, (int)manaHitH) == true)
#endif
	{
		sprintf(strTipText, GlobalText[359], wMana, wManaMax);
		RenderTipText((int)manaHitX, (int)gWideScreen.GetBottomPosition(415), strTipText);
	}
}

void SEASON3B::CNewUIMainFrameWindow::RenderGuageAG()
{
	float x, y, width, height;
	float fY, fH, fV;

	DWORD dwMaxSkillMana, dwSkillMana;
	static DWORD s_fallbackSkillManaMax = 0; //newhudvictoriuri - conserva o maximo durante a sincronizacao.
	const DWORD currentSkillMana = (CharacterAttribute->SkillMana > 0) ? (DWORD)CharacterAttribute->SkillMana : 0; //newhudvictoriuri - leitura atual a cada frame.
	const DWORD characterSkillManaMax = (CharacterAttribute->SkillManaMax > 0) ? (DWORD)CharacterAttribute->SkillManaMax : 0; //newhudvictoriuri
	const DWORD masterSkillManaMax = (gCharacterManager.IsMasterLevel(Hero->Class) && Master_Level_Data.wMaxBP > 0)
		? (DWORD)Master_Level_Data.wMaxBP : 0; //newhudvictoriuri
	dwMaxSkillMana = NewHudVictoriuriResolveGaugeMax(characterSkillManaMax, masterSkillManaMax, currentSkillMana, s_fallbackSkillManaMax); //newhudvictoriuri
	dwSkillMana = min(dwMaxSkillMana, currentSkillMana); //newhudvictoriuri - valor real atualizado.

	float fSkillMana = 0.0f;

	if (dwMaxSkillMana > 0)
	{
		fSkillMana = (dwMaxSkillMana - dwSkillMana) / (float)dwMaxSkillMana;
	}

#if CLIENT_VERSION >= 402 && CLIENT_VERSION <= 502
	width = 16.f, height = 39.f;
	x = gWideScreen.GetCenterPosition(256 + 128 + 36); y = 480.f - 49.f;
	fY = y + (fSkillMana * height);
	fH = height - (fSkillMana * height);
	fV = fSkillMana;

	RenderBitmap(IMAGE_GAUGE_AG, x, fY, width, fH, 0.f, fV * height / 64.f, width / 16.f, (1.0f - fV) * height / 64.f);
	SEASON3B::RenderNumber(x + 10, 480 - 18, (int)dwSkillMana);
#elif CLIENT_VERSION >= 1001
	//newhudvictoriuri - V13: camada AG exata do PSD: X1390 Y1002 W28 H67.
	//newhudvictoriuri - V24.7: camada AG final X1390 Y1002 W28 H67.
	width = NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_AG_GAUGE)); //newhudvictoriuri
	height = NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_AG_GAUGE)); //newhudvictoriuri
	x = NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_AG_GAUGE)); //newhudvictoriuri
	y = NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_AG_GAUGE)); //newhudvictoriuri
	fY = y + (fSkillMana * height);
	fH = height - (fSkillMana * height);
	fV = fSkillMana;
	if (NewHudEditorVisible(HUD_EDITOR_AG_GAUGE)) //newhudvictoriuri
		NewHudVictoriuriRenderVerticalGauge(IMAGE_GAUGE_AG, x, y, width, height, fV, 15.0f, 36.0f); //newhudvictoriuri
	glColor3f(1.0f, 0.81f, 0.55f);
	//newhudvictoriuri - V24.7: valor AG centralizado na linha inferior da referencia.
	// O X1408 compensa o espacamento interno do sprite e deixa o grupo em X1403.
	if (NewHudEditorVisible(HUD_EDITOR_AG_NUMBER)) //newhudvictoriuri
	{
		SEASON3B::RenderOldNumber(
			NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_AG_NUMBER)),
			NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_AG_NUMBER)),
			(int)dwSkillMana, NewHudEditorScale(HUD_EDITOR_AG_NUMBER) * NewHudVictoriuriReadableNumberScale(1.0f)); //newhudvictoriuri - escala final vem do INI.
	}
	glColor3f(1.0f, 1.0f, 1.0f);
#endif

#if CLIENT_VERSION >= 1001
	if (NewHudEditorVisible(HUD_EDITOR_AG_GAUGE)
		&& SEASON3B::CheckMouseIn(x, y, width, height) == true)
#else
	if (SEASON3B::CheckMouseIn(x, y, width, height) == true)
#endif
	{
		char strTipText[256];

		sprintf(strTipText, GlobalText[214], dwSkillMana, dwMaxSkillMana);
		RenderTipText((int)x - 20, (int)gWideScreen.GetBottomPosition(418), strTipText);
	}
}

void SEASON3B::CNewUIMainFrameWindow::RenderGuageSD()
{
	float x, y, width, height;
	float fY, fH, fV;
	DWORD wMaxShield, wShield;
	static DWORD s_fallbackShieldMax = 0; //newhudvictoriuri - conserva o maximo durante a sincronizacao.
	const DWORD currentShield = (CharacterAttribute->Shield > 0) ? (DWORD)CharacterAttribute->Shield : 0; //newhudvictoriuri - leitura atual a cada frame.
	const DWORD characterShieldMax = (CharacterAttribute->ShieldMax > 0) ? (DWORD)CharacterAttribute->ShieldMax : 0; //newhudvictoriuri
	const DWORD masterShieldMax = (gCharacterManager.IsMasterLevel(Hero->Class) && Master_Level_Data.wMaxShield > 0)
		? (DWORD)Master_Level_Data.wMaxShield : 0; //newhudvictoriuri
	wMaxShield = NewHudVictoriuriResolveGaugeMax(characterShieldMax, masterShieldMax, currentShield, s_fallbackShieldMax); //newhudvictoriuri
	wShield = min(wMaxShield, currentShield); //newhudvictoriuri - valor real atualizado.

	float fShield = 0.0f;

	if (wMaxShield > 0)
	{
		fShield = (wMaxShield - wShield) / (float)wMaxShield;
	}
#if CLIENT_VERSION >= 402 && CLIENT_VERSION <= 502
	width = 16.f, height = 39.f;
	x = gWideScreen.GetCenterPosition(204); y = 480.f - 49.f;
	fY = y + (fShield * height);
	fH = height - (fShield * height);
	fV = fShield;

	RenderBitmap(IMAGE_GAUGE_SD, x, fY, width, fH, 0.f, fV * height / 64.f, width / 16.f, (1.0f - fV) * height / 64.f);
	SEASON3B::RenderNumber(x + 15, 480 - 18, (int)wShield);
#elif CLIENT_VERSION >= 1001
	//newhudvictoriuri - V13: camada SD exata do PSD: X500 Y1002 W28 H67.
	//newhudvictoriuri - V24.7: camada SD final X500 Y1002 W28 H67.
	width = NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_SD_GAUGE)); //newhudvictoriuri
	height = NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_SD_GAUGE)); //newhudvictoriuri
	x = NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_SD_GAUGE)); //newhudvictoriuri
	y = NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_SD_GAUGE)); //newhudvictoriuri
	fY = y + (fShield * height);
	fH = height - (fShield * height);
	fV = fShield;
	if (wMaxShield > 0 && NewHudEditorVisible(HUD_EDITOR_SD_GAUGE)) //newhudvictoriuri
		NewHudVictoriuriRenderVerticalGauge(IMAGE_GAUGE_SD, x, y, width, height, fV, 15.0f, 36.0f); //newhudvictoriuri
	glColor3f(1.0f, 0.81f, 0.55f);
	//newhudvictoriuri - V24.7: valor SD centralizado na linha inferior da referencia.
	if (NewHudEditorVisible(HUD_EDITOR_SD_NUMBER)) //newhudvictoriuri
	{
		SEASON3B::RenderOldNumber(
			NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_SD_NUMBER)),
			NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_SD_NUMBER)),
			(int)wShield, NewHudEditorScale(HUD_EDITOR_SD_NUMBER) * NewHudVictoriuriReadableNumberScale(1.0f)); //newhudvictoriuri - escala final vem do INI.
	}
	glColor3f(1.0f, 1.0f, 1.0f);
#endif


	//newhudvictoriuri - V24.7: o hitbox do SD precisa usar a mesma camada
	// desenhada acima; a coordenada legada só vale para CLIENT_VERSION antigo.
#if CLIENT_VERSION < 1001
	height = 39.f;
	y = gWideScreen.GetBottomPosition(480.f - 10.f - 39.f);
#endif
#if CLIENT_VERSION >= 1001
	if (NewHudEditorVisible(HUD_EDITOR_SD_GAUGE)
		&& SEASON3B::CheckMouseIn(x, y, width, height) == true)
#else
	if (SEASON3B::CheckMouseIn(x, y, width, height) == true)
#endif
	{
		char strTipText[256];

		sprintf(strTipText, GlobalText[2037], wShield, wMaxShield);
		RenderTipText((int)x - 20, (int)gWideScreen.GetBottomPosition(418), strTipText);
	}
}

void SEASON3B::CNewUIMainFrameWindow::RenderExperience()
{

	__int64 wLevel;
	__int64 dwNexExperience;
	__int64 dwExperience;
	float x, y, width, height;

#if CLIENT_VERSION >= 1001
	wLevel = CharacterAttribute->Level;
	dwNexExperience = CharacterAttribute->NextExperince;
	dwExperience = CharacterAttribute->Experience;
	//newhudvictoriuri - V20: barra de EXP preenchida pela progressao TOTAL da fase,
	// nao apenas pela fracao da secao atual. Isso evita a faixa aparentemente vazia.
	WORD v20PriorLevel = (wLevel > 0) ? (WORD)(wLevel - 1) : 0;
	__int64 v20PriorExp = 0;
	if (v20PriorLevel > 0)
	{
		v20PriorExp = (__int64)(9 + v20PriorLevel) * v20PriorLevel * v20PriorLevel * 10;
		if (v20PriorLevel > 255)
		{
			int over = v20PriorLevel - 255;
			v20PriorExp += (__int64)(9 + over) * over * over * 1000;
		}
	}

	double need = (double)dwNexExperience - (double)v20PriorExp;
	double have = (double)dwExperience - (double)v20PriorExp;
	if (have < 0.0) have = 0.0;

	double totalRatio = (need > 0.0) ? (have / need) : 0.0;
	if (totalRatio < 0.0) totalRatio = 0.0;
	if (totalRatio > 0.9999) totalRatio = 0.9999;

	double ten = totalRatio * 10.0;
	int section = (int)ten;
	if (section < 0) section = 0;
	if (section > 9) section = 9;

	const float expX = NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_EXP_GAUGE)); //newhudvictoriuri - layout vindo do INI
	const float expY = NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_EXP_GAUGE)); //newhudvictoriuri
	const float expW = NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_EXP_GAUGE)); //newhudvictoriuri
	const float expH = NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_EXP_GAUGE)); //newhudvictoriuri
	if (NewHudEditorVisible(HUD_EDITOR_EXP_GAUGE)) //newhudvictoriuri
		NewHudVictoriuriRenderHorizontalGauge(IMAGE_GAUGE_EXBAR, expX, expY, expW, expH, (float)totalRatio); //newhudvictoriuri - V20

	glColor3f(1.0f, 0.81f, 0.55f);
	//newhudvictoriuri - V24.7: indice da EXP no tamanho/linha do exemplo da referencia.
	if (NewHudEditorVisible(HUD_EDITOR_EXP_NUMBER)) //newhudvictoriuri
	{
		SEASON3B::RenderOldNumber(
			NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_EXP_NUMBER)),
			NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_EXP_NUMBER)),
			section, NewHudEditorScale(HUD_EDITOR_EXP_NUMBER) * NewHudVictoriuriScale());
	}
	glColor3f(1.0f, 1.0f, 1.0f);

	if (NewHudEditorVisible(HUD_EDITOR_EXP_GAUGE)
		&& SEASON3B::CheckMouseIn((int)expX, (int)(expY - NewHudVictoriuriSize(2.0f)), (int)expW, (int)NewHudVictoriuriSize(14.0f)) == true)
	{
		char strTipText[256];
		sprintf(strTipText, GlobalText[1748], (__int64)dwExperience, (__int64)dwNexExperience);
		RenderTipText((int)expX + (int)NewHudVictoriuriSize(40.0f), (int)(expY - NewHudVictoriuriSize(18.0f)), strTipText);
	}
	return;
#endif

#if CLIENT_VERSION >= 401 && CLIENT_VERSION < 502
	if (gCharacterManager.IsMasterLevelExpCheck(CharacterAttribute->Class) == true)
	{
		wLevel = (__int64)Master_Level_Data.nMLevel;
		dwNexExperience = (__int64)Master_Level_Data.lNext_MasterLevel_Experince;
		dwExperience = (__int64)Master_Level_Data.lMasterLevel_Experince;
	}
	else
#endif
	{
		wLevel = CharacterAttribute->Level;
		dwNexExperience = CharacterAttribute->NextExperince;
		dwExperience = CharacterAttribute->Experience;
	}

#if CLIENT_VERSION >= 401 && CLIENT_VERSION <= 502
	if (gCharacterManager.IsMasterLevelExpCheck(CharacterAttribute->Class) == true)
	{
		x = gWideScreen.GetCenterPosition(0); y = 470; width = 6; height = 4;

		__int64 iTotalLevel = wLevel + 400;
		__int64 iTOverLevel = iTotalLevel - 255;
		__int64 iBaseExperience = 0;

		__int64 iData_Master =	// A
			(
				(
					(__int64)9 + (__int64)iTotalLevel
					)
				* (__int64)iTotalLevel
				* (__int64)iTotalLevel
				* (__int64)10
				)
			+
			(
				(
					(__int64)9 + (__int64)iTOverLevel
					)
				* (__int64)iTOverLevel
				* (__int64)iTOverLevel
				* (__int64)1000
				);
		iBaseExperience = (iData_Master - (__int64)3892250000) / (__int64)2;	// B

		double fNeedExp = (double)dwNexExperience - (double)iBaseExperience;
		double fExp = (double)dwExperience - (double)iBaseExperience;

		if (dwExperience < iBaseExperience)
		{
			fExp = 0.f;
		}

		double fExpBarNum = 0.f;
		if (fExp > 0.f && fNeedExp > 0)
		{
			fExpBarNum = ((double)fExp / (double)fNeedExp) * (double)10.f;
		}

		double fProgress = fExpBarNum - __int64(fExpBarNum);

		if (m_bExpEffect == true)
		{
			double fPreProgress = 0.f;
			double fPreExp = (double)m_loPreExp - (double)iBaseExperience;
			if (m_loPreExp < iBaseExperience)
			{
				x = gWideScreen.GetCenterPosition(2); y = 473.f; width = fProgress * 629.f; height = 4.f;
				RenderBitmap(IMAGE_MASTER_GAUGE_BAR, x, y, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);
				glColor4f(1.f, 1.f, 1.f, 0.6f);
				RenderColor(x, y, width, height);
				EndRenderColor();
			}
			else
			{
				int iPreExpBarNum = 0;
				int iExpBarNum = 0;
				if (fPreExp > 0.f && fNeedExp > 0.f)
				{
					fPreProgress = ((double)fPreExp / (double)fNeedExp) * (double)10.f;
					iPreExpBarNum = (int)fPreProgress;
					fPreProgress = (double)fPreProgress - __int64(fPreProgress);
				}
				iExpBarNum = (int)fExpBarNum;

				if (iExpBarNum > iPreExpBarNum)
				{
					x = gWideScreen.GetCenterPosition(2); y = 473.f; width = fProgress * 629.f; height = 4.f;
					RenderBitmap(IMAGE_MASTER_GAUGE_BAR, x, y, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);
					glColor4f(1.f, 1.f, 1.f, 0.6f);
					RenderColor(x, y, width, height);
					EndRenderColor();
				}
				else
				{
					double fGapProgress = 0.f;
					fGapProgress = (double)fProgress - (double)fPreProgress;
					x = gWideScreen.GetCenterPosition(2); y = 473.f; width = (double)fPreProgress * (double)629.f; height = 4.f;
					RenderBitmap(IMAGE_MASTER_GAUGE_BAR, x, y, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);

					x += width; width = (double)fGapProgress * (double)629.f;
					RenderBitmap(IMAGE_MASTER_GAUGE_BAR, x, y, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);
					glColor4f(1.f, 1.f, 1.f, 0.6f);
					RenderColor(x, y, width, height);
					EndRenderColor();
				}
			}
		}
		else
		{
			x = gWideScreen.GetCenterPosition(2); y = 473.f; width = fProgress * 629.f; height = 4.f;
			RenderBitmap(IMAGE_MASTER_GAUGE_BAR, x, y, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);
		}

		int iExp = (int)fExpBarNum;
		x = gWideScreen.GetCenterPosition(635); y = 469.f;
		SEASON3B::RenderNumber(x, y, iExp);

		x = gWideScreen.GetCenterPosition(2); y = 473.f; width = 629.f; height = 4.f;
		if (SEASON3B::CheckMouseIn(x, y, width, height) == true)
		{
			char strTipText[256];

			sprintf(strTipText, GlobalText[1748], dwExperience, dwNexExperience);
			RenderTipText(280, 418, strTipText);
		}
	}
	else
#endif
	{
#if CLIENT_VERSION >= 401 && CLIENT_VERSION < 502
		x = gWideScreen.GetCenterPosition(0); y = 470; width = 6; height = 4;

		WORD wPriorLevel = wLevel - 1;
		DWORD dwPriorExperience = 0;

		if (wPriorLevel > 0)
		{
			dwPriorExperience = (9 + wPriorLevel) * wPriorLevel * wPriorLevel * 10;

			if (wPriorLevel > 255)
			{
				int iLevelOverN = wPriorLevel - 255;
				dwPriorExperience += (9 + iLevelOverN) * iLevelOverN * iLevelOverN * 1000;
			}
		}

		float fNeedExp = dwNexExperience - dwPriorExperience;
		float fExp = dwExperience - dwPriorExperience;

		if (dwExperience < dwPriorExperience)
		{
			fExp = 0.f;
		}

		float fExpBarNum = 0.f;
		if (fExp > 0.f && fNeedExp > 0)
		{
			fExpBarNum = (fExp / fNeedExp) * 10.f;
		}

		float fProgress = fExpBarNum;
		fProgress = fProgress - (int)fProgress;

		if (m_bExpEffect == true)
		{
			float fPreProgress = 0.f;
			fExp = m_dwPreExp - dwPriorExperience;
			if (m_dwPreExp < dwPriorExperience)
			{
				x = gWideScreen.GetCenterPosition(2); y = 473.f; width = fProgress * 629.f; height = 4.f;
				RenderBitmap(IMAGE_GAUGE_EXBAR, x, y, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);
				glColor4f(1.f, 1.f, 1.f, 0.4f);
				RenderColor(x, y, width, height);
				EndRenderColor();
			}
			else
			{
				int iPreExpBarNum = 0;
				int iExpBarNum = 0;
				if (fExp > 0.f && fNeedExp > 0.f)
				{
					fPreProgress = (fExp / fNeedExp) * 10.f;
					iPreExpBarNum = (int)fPreProgress;
					fPreProgress = fPreProgress - (int)fPreProgress;
				}

				iExpBarNum = (int)fExpBarNum;

				if (iExpBarNum > iPreExpBarNum)
				{
					x = gWideScreen.GetCenterPosition(2); y = 473.f; width = fProgress * 629.f; height = 4.f;
					RenderBitmap(IMAGE_GAUGE_EXBAR, x, y, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);
					glColor4f(1.f, 1.f, 1.f, 0.4f);
					RenderColor(x, y, width, height);
					EndRenderColor();
				}
				else
				{
					float fGapProgress = 0.f;
					fGapProgress = fProgress - fPreProgress;
					x = gWideScreen.GetCenterPosition(2); y = 473.f; width = fPreProgress * 629.f; height = 4.f;
					RenderBitmap(IMAGE_GAUGE_EXBAR, x, y, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);
					x += width; width = fGapProgress * 629.f;
					RenderBitmap(IMAGE_GAUGE_EXBAR, x, y, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);
					glColor4f(1.f, 1.f, 1.f, 0.4f);
					RenderColor(x, y, width, height);
					EndRenderColor();
				}
			}
		}
		else
		{
			x = gWideScreen.GetCenterPosition(2); y = 473.f; width = fProgress * 629.f; height = 4.f;
			RenderBitmap(IMAGE_GAUGE_EXBAR, x, y, width, height, 0.f, 0.f, 6.f / 8.f, 4.f / 4.f);
		}

		int iExp = (int)fExpBarNum;
		x = gWideScreen.GetCenterPosition(635); y = 469.f;
		SEASON3B::RenderNumber(x, y, iExp);

		x = gWideScreen.GetCenterPosition(2); y = 473.f; width = 629.f; height = 4.f;
		if (SEASON3B::CheckMouseIn(x, y, width, height) == true)
		{
			char strTipText[256];

			sprintf(strTipText, GlobalText[1748], dwExperience, dwNexExperience);
			RenderTipText(280, 418, strTipText);
		}
#elif CLIENT_VERSION == 1002

		WORD wPriorLevel = wLevel - 1;
		DWORD dwPriorExperience = 0;

		if (wPriorLevel > 0)
		{
			dwPriorExperience = (9 + wPriorLevel) * wPriorLevel * wPriorLevel * 10;

			if (wPriorLevel > 255)
			{
				int iLevelOverN = wPriorLevel - 255;
				dwPriorExperience += (9 + iLevelOverN) * iLevelOverN * iLevelOverN * 1000;
			}
		}

		float fNeedExp = floor(dwNexExperience - dwPriorExperience);
		int fExp = dwExperience - dwPriorExperience;

		if (dwExperience < dwPriorExperience)
		{
			fExp = 0.f;
		}

		float stepValue = fNeedExp / 10;

		int currentSection = floor(fExp / stepValue);

		double sectionExp = fExp % (int)stepValue;

		float sectionProgress = sectionExp / stepValue;

		width = sectionProgress * 200;


		x = gWideScreen.GetCenterPosition(220); y = gWideScreen.GetBottomPosition(437.f); height = 4.f;

		glColor4f(1.0f, 0.81f, 0.55f, 0.4f);
		RenderColor(x, y, width, height);
		EndRenderColor();

		glColor3f(1.0f, 0.81f, 0.55f);
		SEASON3B::RenderOldNumber(x + 214, y - 6.0f, currentSection, 0.75f); //newhudvictoriuri - V11: numero da secao de experiencia alinhado ao topo da faixa dourada da referencia.
		glColor3f(1.0f, 1.0f, 1.0f);

#endif
	}
}

void SEASON3B::CNewUIMainFrameWindow::RenderHotKeyItemCount()
{
	m_ItemHotKey.RenderItemCount();
}

void SEASON3B::CNewUIMainFrameWindow::RenderButtons()
{


#if CLIENT_VERSION >= 402 && CLIENT_VERSION <= 502
#ifdef PBG_ADD_INGAMESHOP_UI_MAINFRAME
	m_BtnCShop.Render();
#endif //defined PBG_ADD_INGAMESHOP_UI_MAINFRAME
	RenderCharInfoButton();
	m_BtnMyInven.Render();

	RenderFriendButton();
	m_BtnWindow.Render();
#endif
#if CLIENT_VERSION >= 1001
	//newhudvictoriuri - V22: nao manter nenhum botao visualmente afundado apenas
	// porque a janela esta aberta. O efeito ocorre somente enquanto o mouse esta clicando.
	m_BtnMyInven.ForceButtonDown(false);
	m_BtnChaInfo.ForceButtonDown(false);
	m_BtnParty.ForceButtonDown(false);

	//newhudvictoriuri - V24.9: o pack traz os icones separados em 32x32.
	// Renderizamos cada icone sempre, centralizado na caixa do HUDLayout.ini;
	// a hitbox continua sendo a caixa inteira de 45x45.
	const float partyX = NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_BUTTON_PARTY)); //newhudvictoriuri
	const float partyY = NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_BUTTON_PARTY)); //newhudvictoriuri
	const float partyW = NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_BUTTON_PARTY)); //newhudvictoriuri
	const float partyH = NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_BUTTON_PARTY)); //newhudvictoriuri
	const float charX = NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_BUTTON_CHARACTER)); //newhudvictoriuri
	const float charY = NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_BUTTON_CHARACTER)); //newhudvictoriuri
	const float charW = NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_BUTTON_CHARACTER)); //newhudvictoriuri
	const float charH = NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_BUTTON_CHARACTER)); //newhudvictoriuri
	const float invenX = NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_BUTTON_INVENTORY)); //newhudvictoriuri
	const float invenY = NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_BUTTON_INVENTORY)); //newhudvictoriuri
	const float invenW = NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_BUTTON_INVENTORY)); //newhudvictoriuri
	const float invenH = NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_BUTTON_INVENTORY)); //newhudvictoriuri
	//newhudvictoriuri - V31: remove definitivamente o falso estado de botao apertado.
	// O estado visual agora nasce apenas no clique FISICO iniciado dentro da propria hitbox.
	// Janela aberta, hover e flags antigas do cliente nao conseguem mais deixar o botao afundado.
	NewHudVictoriuriUpdatePhysicalButtonPress(
		partyX, partyY, partyW, partyH,
		charX, charY, charW, charH,
		invenX, invenY, invenW, invenH); //newhudvictoriuri
	const bool partyPressed = (g_NewHudVictoriuriPressedButton == NEW_HUD_PRESS_PARTY); //newhudvictoriuri
	const bool charPressed = (g_NewHudVictoriuriPressedButton == NEW_HUD_PRESS_CHARACTER); //newhudvictoriuri
	const bool invenPressed = (g_NewHudVictoriuriPressedButton == NEW_HUD_PRESS_INVENTORY); //newhudvictoriuri
	if (NewHudEditorVisible(HUD_EDITOR_BUTTON_PARTY)) //newhudvictoriuri
		NewHudVictoriuriRenderButtonIcon(IMAGE_MENU_BTN_WINDOW, 0,
			partyX, partyY, partyW, partyH, partyPressed); //newhudvictoriuri
	if (NewHudEditorVisible(HUD_EDITOR_BUTTON_CHARACTER)) //newhudvictoriuri
		NewHudVictoriuriRenderButtonIcon(IMAGE_MENU_BTN_CHAINFO, 0,
			charX, charY, charW, charH, charPressed); //newhudvictoriuri
	if (NewHudEditorVisible(HUD_EDITOR_BUTTON_INVENTORY)) //newhudvictoriuri
		NewHudVictoriuriRenderButtonIcon(IMAGE_MENU_BTN_MYINVEN, 0,
			invenX, invenY, invenW, invenH, invenPressed); //newhudvictoriuri

	if (NewHudEditorVisible(HUD_EDITOR_BUTTON_PARTY) && SEASON3B::CheckMouseIn((int)partyX, (int)partyY, (int)partyW, (int)partyH))
		RenderTipText((int)partyX, (int)(partyY - NewHudVictoriuriSize(12.0f)), GlobalText[361]);
	if (NewHudEditorVisible(HUD_EDITOR_BUTTON_CHARACTER) && SEASON3B::CheckMouseIn((int)charX, (int)charY, (int)charW, (int)charH))
		RenderTipText((int)charX, (int)(charY - NewHudVictoriuriSize(12.0f)), GlobalText[362]);
	if (NewHudEditorVisible(HUD_EDITOR_BUTTON_INVENTORY) && SEASON3B::CheckMouseIn((int)invenX, (int)invenY, (int)invenW, (int)invenH))
		RenderTipText((int)invenX, (int)(invenY - NewHudVictoriuriSize(12.0f)), GlobalText[363]);

	//newhudvictoriuri - V22: range real dos 2 botoes da direita medido na textura base.
	// Local X 1306..1402 + origem PSD 143 = global X 1449..1545.
	//newhudvictoriuri - V24.6.5: Central de Seguranca = botao SUPERIOR com cadeado do menu03 original.
	//newhudvictoriuri - V24.6.7: hitboxes exatas do cadeado (Central de Seguranca) e espadas.
	//newhudvictoriuri - V24.6.8: referencia 581/432 e 581/454, tamanho 52x18.
	const float rightBtnX = NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_BUTTON_RIGHT_TOP)); //newhudvictoriuri
	const float rightTopY = NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_BUTTON_RIGHT_TOP)); //newhudvictoriuri
	const float rightBottomY = NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_BUTTON_RIGHT_BOTTOM)); //newhudvictoriuri
	const float rightTopW = NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_BUTTON_RIGHT_TOP)); //newhudvictoriuri
	const float rightTopH = NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_BUTTON_RIGHT_TOP)); //newhudvictoriuri
	const float rightBottomW = NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_BUTTON_RIGHT_BOTTOM)); //newhudvictoriuri
	const float rightBottomH = NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_BUTTON_RIGHT_BOTTOM)); //newhudvictoriuri
	const bool physicalLButtonDown = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0); //newhudvictoriuri - V31: nao usa flag logica que pode ficar presa.
	const bool rightTopPressed = physicalLButtonDown && SEASON3B::CheckMouseIn((int)rightBtnX, (int)rightTopY, (int)rightTopW, (int)rightTopH); //newhudvictoriuri - V31
	const bool rightBottomPressed = physicalLButtonDown && SEASON3B::CheckMouseIn((int)rightBtnX, (int)rightBottomY, (int)rightBottomW, (int)rightBottomH); //newhudvictoriuri - V31
	NewHudVictoriuriRenderRightButtonPress(rightBtnX, rightTopY, rightTopW, rightTopH, rightTopPressed && NewHudEditorVisible(HUD_EDITOR_BUTTON_RIGHT_TOP));
	NewHudVictoriuriRenderRightButtonPress(rightBtnX, rightBottomY, rightBottomW, rightBottomH, rightBottomPressed && NewHudEditorVisible(HUD_EDITOR_BUTTON_RIGHT_BOTTOM));
	if (NewHudEditorVisible(HUD_EDITOR_BUTTON_RIGHT_TOP) && SEASON3B::CheckMouseIn((int)rightBtnX, (int)rightTopY, (int)rightTopW, (int)rightTopH))
		RenderTipText((int)rightBtnX, (int)(rightTopY - NewHudVictoriuriSize(12.0f)), "Central de Seguranca"); //newhudvictoriuri - V24.6.5
#endif



}

void SEASON3B::CNewUIMainFrameWindow::RenderCharInfoButton()
{
#if CLIENT_VERSION >= 402 && CLIENT_VERSION <= 502
	m_BtnChaInfo.Render();



	if (g_QuestMng.IsQuestIndexByEtcListEmpty())
		return;

	if (g_Time.GetTimeCheck(5, 500))
		m_bButtonBlink = !m_bButtonBlink;

	if (m_bButtonBlink)
	{
		if (!(g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_QUEST_PROGRESS_ETC)
			|| g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHARACTER)))
			RenderImage(IMAGE_MENU_BTN_CHAINFO, gWideScreen.GetCenterPosition(489) + 30, 480 - 51, 30, 41, 0.0f, 41.f);
	}
#endif

#if CLIENT_VERSION >= 1001
	//newhudvictoriuri - CLIENT_VERSION 1001 renders the character button through
	//m_BtnChaInfo in RenderButtons().  The old manual atlas draw duplicated
	//the icon and sampled UVs that do not exist in the single-frame asset.
#endif
}

void SEASON3B::CNewUIMainFrameWindow::RenderFriendButton()
{
	m_BtnFriend.Render();

	int iBlinkTemp = g_pFriendMenu->GetBlinkTemp();
	BOOL bIsAlertTime = (iBlinkTemp % 24 < 12);

	if (g_pFriendMenu->IsNewChatAlert() && bIsAlertTime)
	{
		RenderFriendButtonState();
	}
	if (g_pFriendMenu->IsNewMailAlert())
	{
		if (bIsAlertTime)
		{
			RenderFriendButtonState();

			if (iBlinkTemp % 24 == 11)
			{
				g_pFriendMenu->IncreaseLetterBlink();
			}
		}
	}
	else if (g_pLetterList->CheckNoReadLetter())
	{
		RenderFriendButtonState();
	}

	g_pFriendMenu->IncreaseBlinkTemp();
}

void SEASON3B::CNewUIMainFrameWindow::RenderFriendButtonState()
{
#ifdef PBG_ADD_INGAMESHOP_UI_MAINFRAME
	if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_FRIEND) == true)
	{
		RenderImage(IMAGE_MENU_BTN_FRIEND, gWideScreen.GetCenterPosition(489) + (30 * 3), 480 - 51, 30, 41, 0.0f, 123.f);
	}
	else
	{
		RenderImage(IMAGE_MENU_BTN_FRIEND, gWideScreen.GetCenterPosition(489) + (30 * 3), 480 - 51, 30, 41, 0.0f, 41.f);
	}
#else //defined PBG_ADD_INGAMESHOP_UI_MAINFRAME
	if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_FRIEND) == true)
	{
		RenderImage(IMAGE_MENU_BTN_FRIEND, gWideScreen.GetCenterPosition(488) + 76, 480 - 51, 38, 42, 0.0f, 126.f);
	}
	else
	{
		RenderImage(IMAGE_MENU_BTN_FRIEND, gWideScreen.GetCenterPosition(488) + 76, 480 - 51, 38, 42, 0.0f, 42.f);
	}
#endif//defined PBG_ADD_INGAMESHOP_UI_MAINFRAME
}

bool SEASON3B::CNewUIMainFrameWindow::UpdateMouseEvent()
{
	if (g_pNewUIHotKey->IsStateGameOver() == true)
	{
		return true;
	}

#if CLIENT_VERSION >= 1001
	if (NewHudEditorIsOpen())
	{
		NewHudEditorUpdateMouseEvent(); //newhudvictoriuri - bloqueia cliques do jogo durante a edicao.
		return false;
	}
#endif

	if (BtnProcess() == true)
	{
		return false;
	}

	return true;
}

bool SEASON3B::CNewUIMainFrameWindow::BtnProcess()
{
#if CLIENT_VERSION >= 1001
	//newhudvictoriuri - V32: os tres botoes centrais usam a propria hitbox renderizada.
	// Isso elimina a dependencia do Process()/IsRelease do CCustomButtons, que
	// estava deixando Party/Character/Inventory sem resposta nesta HUD escalada.
	const float partyX = NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_BUTTON_PARTY));
	const float partyY = NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_BUTTON_PARTY));
	const float partyW = NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_BUTTON_PARTY));
	const float partyH = NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_BUTTON_PARTY));
	const float charX = NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_BUTTON_CHARACTER));
	const float charY = NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_BUTTON_CHARACTER));
	const float charW = NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_BUTTON_CHARACTER));
	const float charH = NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_BUTTON_CHARACTER));
	const float invenX = NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_BUTTON_INVENTORY));
	const float invenY = NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_BUTTON_INVENTORY));
	const float invenW = NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_BUTTON_INVENTORY));
	const float invenH = NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_BUTTON_INVENTORY));

	const int hudClickedButton = NewHudVictoriuriPollHudButtonClick(
		partyX, partyY, partyW, partyH, NewHudEditorVisible(HUD_EDITOR_BUTTON_PARTY),
		charX, charY, charW, charH, NewHudEditorVisible(HUD_EDITOR_BUTTON_CHARACTER),
		invenX, invenY, invenW, invenH, NewHudEditorVisible(HUD_EDITOR_BUTTON_INVENTORY)); //newhudvictoriuri

	// Central de Seguranca (cadeado superior da lateral direita).
	/*
	// Botao superior direito da HUD: funcao desativada, como na source antiga.
	// Este bloco abria INTERFACE_WINDOW_MENU (Move, Trade e outras funcoes),
	// e nao a Central de Seguranca de contas.
	const float rightBtnX = NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_BUTTON_RIGHT_TOP));
	const float rightTopY = NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_BUTTON_RIGHT_TOP));
	const float rightTopW = NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_BUTTON_RIGHT_TOP));
	const float rightTopH = NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_BUTTON_RIGHT_TOP));
	const bool rightTopVisible = NewHudEditorVisible(HUD_EDITOR_BUTTON_RIGHT_TOP);
	const bool physicalLButtonDown = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
	const bool mouseInRightTop = rightTopVisible &&
		SEASON3B::CheckMouseIn((int)rightBtnX, (int)rightTopY, (int)rightTopW, (int)rightTopH);

	if (physicalLButtonDown && !g_NewHudVictoriuriRightTopWasDown)
	{
		g_NewHudVictoriuriRightTopStarted = mouseInRightTop;
	}

	if (!physicalLButtonDown && g_NewHudVictoriuriRightTopWasDown)
	{
		if (g_NewHudVictoriuriRightTopStarted && mouseInRightTop && g_pNewUIHotKey->CanUpdateKeyEvent() == true)
		{
			g_pNewUISystem->Toggle(SEASON3B::INTERFACE_WINDOW_MENU);
			PlayBuffer(SOUND_CLICK01);
			g_NewHudVictoriuriRightTopStarted = false;
			g_NewHudVictoriuriRightTopWasDown = physicalLButtonDown;
			return true;
		}
		g_NewHudVictoriuriRightTopStarted = false;
	}

	g_NewHudVictoriuriRightTopWasDown = physicalLButtonDown;
	*/

	if (hudClickedButton == NEW_HUD_PRESS_INVENTORY)
	{
		if (g_pNewUIHotKey->CanUpdateKeyEventRelatedMyInventory() == true || g_pNewUIHotKey->CanUpdateKeyEvent() == true)
		{
			g_pNewUISystem->Toggle(SEASON3B::INTERFACE_INVENTORY); //newhudvictoriuri
			PlayBuffer(SOUND_CLICK01);
			return true;
		}
	}
	else if (hudClickedButton == NEW_HUD_PRESS_CHARACTER)
	{
		if (g_pNewUIHotKey->CanUpdateKeyEvent() == true)
		{
			g_pNewUISystem->Toggle(SEASON3B::INTERFACE_CHARACTER); //newhudvictoriuri
			PlayBuffer(SOUND_CLICK01);
			if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHARACTER))
				g_QuestMng.SendQuestIndexByEtcSelection();
			return true;
		}
	}
	else if (hudClickedButton == NEW_HUD_PRESS_PARTY)
	{
		if (g_pNewUIHotKey->CanUpdateKeyEvent() == true)
		{
			g_pNewUISystem->Toggle(SEASON3B::INTERFACE_PARTY); //newhudvictoriuri
			PlayBuffer(SOUND_CLICK01);

			if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHARACTER))
				g_pNewUISystem->Toggle(SEASON3B::INTERFACE_CHARACTER);
			if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INVENTORY))
				g_pNewUISystem->Toggle(SEASON3B::INTERFACE_INVENTORY);
			if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_GUILDINFO))
				g_pNewUISystem->Toggle(SEASON3B::INTERFACE_GUILDINFO);
			return true;
		}
	}
#endif

	if (g_pNewUIHotKey->CanUpdateKeyEventRelatedMyInventory() == true)
	{
		if (m_BtnMyInven.UpdateMouseEvent() == true)
		{
			g_pNewUISystem->Toggle(SEASON3B::INTERFACE_INVENTORY);
			PlayBuffer(SOUND_CLICK01);
			return true;
		}
	}
	else if (g_pNewUIHotKey->CanUpdateKeyEvent() == true)
	{
		if (m_BtnMyInven.UpdateMouseEvent() == true)
		{
			g_pNewUISystem->Toggle(SEASON3B::INTERFACE_INVENTORY);
			PlayBuffer(SOUND_CLICK01);
			return true;
		}
		else if (m_BtnChaInfo.UpdateMouseEvent() == true)
		{
			g_pNewUISystem->Toggle(SEASON3B::INTERFACE_CHARACTER);
			PlayBuffer(SOUND_CLICK01);

			if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHARACTER))
				g_QuestMng.SendQuestIndexByEtcSelection();

			return true;
		}
		else if (m_BtnFriend.UpdateMouseEvent() == true)
		{
			if (gMapManager.InChaosCastle() == true)
			{
				PlayBuffer(SOUND_CLICK01);
				return true;
			}

			int iLevel = CharacterAttribute->Level;

			if (iLevel < 6)
			{
				if (g_pChatListBox->CheckChatRedundancy(GlobalText[1067]) == FALSE)
				{
					g_pChatListBox->AddText("", GlobalText[1067], SEASON3B::TYPE_SYSTEM_MESSAGE);
				}
			}
			else
			{
				g_pNewUISystem->Toggle(SEASON3B::INTERFACE_FRIEND);
			}
			PlayBuffer(SOUND_CLICK01);
			return true;
		}
#if CLIENT_VERSION >= 1001
		else if (m_BtnParty.UpdateMouseEvent() == true)
		{
			g_pNewUISystem->Toggle(SEASON3B::INTERFACE_PARTY);
			PlayBuffer(SOUND_CLICK01);

			if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHARACTER))
				g_pNewUISystem->Toggle(SEASON3B::INTERFACE_CHARACTER);

			if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INVENTORY))
				g_pNewUISystem->Toggle(SEASON3B::INTERFACE_INVENTORY);

			if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_GUILDINFO))
				g_pNewUISystem->Toggle(SEASON3B::INTERFACE_GUILDINFO);

			return true;
		}
#endif
		//else if(m_BtnWindow.UpdateMouseEvent() == true)
		//{
		//	g_pNewUISystem->Toggle(SEASON3B::INTERFACE_WINDOW_MENU);
		//	PlayBuffer(SOUND_CLICK01);
		//	return true;
		//}
		/*
#ifdef PBG_ADD_INGAMESHOP_UI_MAINFRAME
		else if(m_BtnCShop.UpdateMouseEvent() == true)
		{
			if(g_pInGameShop->IsInGameShopOpen() == false)
				return false;

#ifdef KJH_MOD_SHOP_SCRIPT_DOWNLOAD
			if( g_InGameShopSystem->IsScriptDownload() == true )
			{
				if( g_InGameShopSystem->ScriptDownload() == false )
					return false;
			}

			if( g_InGameShopSystem->IsBannerDownload() == true )
			{
				g_InGameShopSystem->BannerDownload();
			}
#endif // KJH_MOD_SHOP_SCRIPT_DOWNLOAD

			if( g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INGAMESHOP) == false)
			{
				if( g_InGameShopSystem->GetIsRequestShopOpenning() == false )
				{
					SendRequestIGS_CashShopOpen(0);
					g_InGameShopSystem->SetIsRequestShopOpenning(true);

#ifdef KJH_MOD_SHOP_SCRIPT_DOWNLOAD
					g_pMainFrame->SetBtnState(MAINFRAME_BTN_PARTCHARGE, true);
#endif // KJH_MOD_SHOP_SCRIPT_DOWNLOAD

				}
			}
			else
			{
				SendRequestIGS_CashShopOpen(1);
				g_pNewUISystem->Hide(SEASON3B::INTERFACE_INGAMESHOP);
			}

			return true;
		}
#endif //PBG_ADD_INGAMESHOP_UI_MAINFRAME
		*/
	}

	return false;
}

bool SEASON3B::CNewUIMainFrameWindow::UpdateKeyEvent()
{
#if CLIENT_VERSION >= 1001
	if (NewHudEditorIsOpen())
	{
		NewHudEditorUpdateKeyEvent(); //newhudvictoriuri - setas, escala e atalhos do editor.
		return false;
	}
#endif

	if (m_ItemHotKey.UpdateKeyEvent() == false)
	{
		return false;
	}
	return true;
}

bool SEASON3B::CNewUIMainFrameWindow::Update()
{
#if CLIENT_VERSION >= 1001
	if (NewHudEditorUpdate()) //newhudvictoriuri - atualiza hitboxes cacheadas apos arraste/teclado.
	{
		SetButtonInfo();
		if (g_pSkillList != NULL)
			g_pSkillList->SetButtonInfo();
	}

	//newhudvictoriuri
	// These three HUD buttons cache their position in Create(). Re-anchor them
	// only when maximize/restore changes the widescreen working area.
	static int s_LastWideWidth = -1;
	static int s_LastWideHeight = -1;
	const int currentWideWidth = (int)gWideScreen.g_WideWindowWidth;
	const int currentWideHeight = (int)gWideScreen.g_WideWindowHeight;
	if (currentWideWidth != s_LastWideWidth || currentWideHeight != s_LastWideHeight)
	{
		SetButtonInfo();
		s_LastWideWidth = currentWideWidth;
		s_LastWideHeight = currentWideHeight;
	}
#endif

	if (m_bExpEffect == true)
	{
		if (timeGetTime() - m_dwExpEffectTime > 2000)
		{
			m_bExpEffect = false;
			m_dwExpEffectTime = 0;
			m_dwGetExp = 0;
		}
	}

	return true;
}

float SEASON3B::CNewUIMainFrameWindow::GetLayerDepth()
{
	return 10.6f;
}

float SEASON3B::CNewUIMainFrameWindow::GetKeyEventOrder()
{
	return 2.9f;
}

void SEASON3B::CNewUIMainFrameWindow::SetItemHotKey(int iHotKey, int iItemType, int iItemLevel)
{
	m_ItemHotKey.SetHotKey(iHotKey, iItemType, iItemLevel);
}

int SEASON3B::CNewUIMainFrameWindow::GetItemHotKey(int iHotKey)
{
	return m_ItemHotKey.GetHotKey(iHotKey);
}

int SEASON3B::CNewUIMainFrameWindow::GetItemHotKeyLevel(int iHotKey)
{
	return m_ItemHotKey.GetHotKeyLevel(iHotKey);
}

void SEASON3B::CNewUIMainFrameWindow::UseHotKeyItemRButton()
{
	m_ItemHotKey.UseItemRButton();
}

void SEASON3B::CNewUIMainFrameWindow::UpdateItemHotKey()
{
	m_ItemHotKey.UpdateKeyEvent();
}

void SEASON3B::CNewUIMainFrameWindow::ResetSkillHotKey()
{
	g_pSkillList->Reset();
}

void SEASON3B::CNewUIMainFrameWindow::SetSkillHotKey(int iHotKey, int iSkillType)
{
	g_pSkillList->SetHotKey(iHotKey, iSkillType);
}

int SEASON3B::CNewUIMainFrameWindow::GetSkillHotKey(int iHotKey)
{
	return g_pSkillList->GetHotKey(iHotKey);
}

int SEASON3B::CNewUIMainFrameWindow::GetSkillHotKeyIndex(int iSkillType)
{
	return g_pSkillList->GetSkillIndex(iSkillType);
}

SEASON3B::CNewUIItemHotKey::CNewUIItemHotKey()
{
	for (int i = 0; i < HOTKEY_COUNT; ++i)
	{
		m_iHotKeyItemType[i] = -1;
		m_iHotKeyItemLevel[i] = 0;
	}
}

SEASON3B::CNewUIItemHotKey::~CNewUIItemHotKey()
{

}

bool SEASON3B::CNewUIItemHotKey::UpdateKeyEvent()
{
	int iIndex = -1;
	static bool bAutoHoldQ = false;
	static bool bAutoHoldW = false;
	static bool bAutoHoldE = false;

	// Verifica se houve um novo pressionamento (sem repetio)
	//if (SEASON3B::IsPress('Q')) 
	//{
	//    bAutoHoldQ = !bAutoHoldQ; // Alterna entre ligado/desligado
	//}
	//if (SEASON3B::IsPress('W')) 
	//{
	//    bAutoHoldW = !bAutoHoldW;
	//}
	//if (SEASON3B::IsPress('E')) 
	//{
	//    bAutoHoldE = !bAutoHoldE;
	//}
	//
	//// Define iIndex com base no estado atual
	if (SEASON3B::IsPress('Q'))
	{
		iIndex = GetHotKeyItemIndex(HOTKEY_Q);
	}
	else if (SEASON3B::IsPress('W'))
	{
		iIndex = GetHotKeyItemIndex(HOTKEY_W);
	}
	else if (SEASON3B::IsPress('E'))
	{
		iIndex = GetHotKeyItemIndex(HOTKEY_E);
	}
	else
	{
		iIndex = -1; // Nenhuma tecla ativa
	}


#if CLIENT_VERSION >= 401 && CLIENT_VERSION <= 502
	else if (SEASON3B::IsPress('R') == true)
	{
		iIndex = GetHotKeyItemIndex(HOTKEY_R);
		}
#endif

		if (iIndex != -1)
		{
			ITEM* pItem = NULL;
			pItem = g_pMyInventory->FindItem(iIndex);
			if ((pItem->Type >= ITEM_POTION + 78 && pItem->Type <= ITEM_POTION + 82))
			{
				std::list<eBuffState> secretPotionbufflist;
				secretPotionbufflist.push_back(eBuff_SecretPotion1);
				secretPotionbufflist.push_back(eBuff_SecretPotion2);
				secretPotionbufflist.push_back(eBuff_SecretPotion3);
				secretPotionbufflist.push_back(eBuff_SecretPotion4);
				secretPotionbufflist.push_back(eBuff_SecretPotion5);

				if (g_isCharacterBufflist((&Hero->Object), secretPotionbufflist) != eBuffNone) {
					SEASON3B::CreateOkMessageBox(GlobalText[2530], RGBA(255, 30, 0, 255));
				}
				else {
					SendRequestUse(iIndex, 0);
				}
			}
			else

			{
				SendRequestUse(iIndex, 0);
			}
			return false;
		}

		return true;
}

int SEASON3B::CNewUIItemHotKey::GetHotKeyItemIndex(int iType, bool bItemCount)
{
	int iStartItemType = 0, iEndItemType = 0;
	int i, j;

	switch (iType)
	{
	case HOTKEY_Q:
		if (GetHotKeyCommonItem(iType, iStartItemType, iEndItemType) == false)
		{
			if (m_iHotKeyItemType[iType] >= ITEM_POTION + 4 && m_iHotKeyItemType[iType] <= ITEM_POTION + 6)
			{
				iStartItemType = ITEM_POTION + 6; iEndItemType = ITEM_POTION + 4;
			}
			else
			{
				iStartItemType = ITEM_POTION + 3; iEndItemType = ITEM_POTION + 0;
			}
		}
		break;
	case HOTKEY_W:
		if (GetHotKeyCommonItem(iType, iStartItemType, iEndItemType) == false)
		{
			if (m_iHotKeyItemType[iType] >= ITEM_POTION + 0 && m_iHotKeyItemType[iType] <= ITEM_POTION + 3)
			{
				iStartItemType = ITEM_POTION + 3; iEndItemType = ITEM_POTION + 0;
			}
			else
			{
				iStartItemType = ITEM_POTION + 6; iEndItemType = ITEM_POTION + 4;
			}
		}
		break;
	case HOTKEY_E:
		if (GetHotKeyCommonItem(iType, iStartItemType, iEndItemType) == false)
		{
			if (m_iHotKeyItemType[iType] >= ITEM_POTION + 0 && m_iHotKeyItemType[iType] <= ITEM_POTION + 3)
			{
				iStartItemType = ITEM_POTION + 3; iEndItemType = ITEM_POTION + 0;
			}
			else if (m_iHotKeyItemType[iType] >= ITEM_POTION + 4 && m_iHotKeyItemType[iType] <= ITEM_POTION + 6)
			{
				iStartItemType = ITEM_POTION + 6; iEndItemType = ITEM_POTION + 4;
			}
			else
			{
				iStartItemType = ITEM_POTION + 8; iEndItemType = ITEM_POTION + 8;
			}
		}
		break;
#if CLIENT_VERSION >= 401 && CLIENT_VERSION <= 502
	case HOTKEY_R:
		if (GetHotKeyCommonItem(iType, iStartItemType, iEndItemType) == false)
		{
			if (m_iHotKeyItemType[iType] >= ITEM_POTION + 0 && m_iHotKeyItemType[iType] <= ITEM_POTION + 3)
			{
				iStartItemType = ITEM_POTION + 3; iEndItemType = ITEM_POTION + 0;
			}
			else if (m_iHotKeyItemType[iType] >= ITEM_POTION + 4 && m_iHotKeyItemType[iType] <= ITEM_POTION + 6)
			{
				iStartItemType = ITEM_POTION + 6; iEndItemType = ITEM_POTION + 4;
			}
			else
			{
				iStartItemType = ITEM_POTION + 37; iEndItemType = ITEM_POTION + 35;
			}
		}
		break;
#endif
	}

	int iItemCount = 0;
	ITEM* pItem = NULL;

	int iNumberofItems = g_pMyInventory->GetInventoryCtrl()->GetNumberOfItems();
	for (i = iStartItemType; i >= iEndItemType; --i)
	{
		if (bItemCount)
		{
			for (j = 0; j < iNumberofItems; ++j)
			{
				pItem = g_pMyInventory->GetInventoryCtrl()->GetItem(j);
				if (pItem == NULL)
				{
					continue;
				}

				if (
					(pItem->Type == i && ((pItem->Level >> 3) & 15) == m_iHotKeyItemLevel[iType])
					|| (pItem->Type == i && (pItem->Type >= ITEM_POTION + 0 && pItem->Type <= ITEM_POTION + 3))
					)
				{
					if (pItem->Type == ITEM_POTION + 9
						|| pItem->Type == ITEM_POTION + 10
						|| pItem->Type == ITEM_POTION + 20
						)
					{
						iItemCount++;
					}
					else
					{
						iItemCount += pItem->Durability;
					}
				}
			}
		}
		else
		{
			int iIndex = -1;
			if (i >= ITEM_POTION + 0 && i <= ITEM_POTION + 3)
			{
				iIndex = g_pMyInventory->FindItemReverseIndex(i);
			}
			else
			{
				iIndex = g_pMyInventory->FindItemReverseIndex(i, m_iHotKeyItemLevel[iType]);
			}

			if (-1 != iIndex)
			{
				pItem = g_pMyInventory->FindItem(iIndex);
				if ((pItem->Type != ITEM_POTION + 7
					&& pItem->Type != ITEM_POTION + 10
					&& pItem->Type != ITEM_POTION + 20)
					|| ((pItem->Level >> 3) & 15) == m_iHotKeyItemLevel[iType]
					)
				{
					return iIndex;
				}
			}
		}
	}

	if (bItemCount == true)
	{
		return iItemCount;
	}

	return -1;
}

bool SEASON3B::CNewUIItemHotKey::GetHotKeyCommonItem(IN int iHotKey, OUT int& iStart, OUT int& iEnd)
{
	switch (m_iHotKeyItemType[iHotKey])
	{
	case ITEM_POTION + 7:
	case ITEM_POTION + 8:
	case ITEM_POTION + 9:
	case ITEM_POTION + 10:
	case ITEM_POTION + 20:
	case ITEM_POTION + 46:
	case ITEM_POTION + 47:
	case ITEM_POTION + 48:
	case ITEM_POTION + 49:
	case ITEM_POTION + 50:
	case ITEM_POTION + 70:
	case ITEM_POTION + 71:
	case ITEM_POTION + 78:
	case ITEM_POTION + 79:
	case ITEM_POTION + 80:
	case ITEM_POTION + 81:
	case ITEM_POTION + 82:
	case ITEM_POTION + 94:
	case ITEM_POTION + 85:
	case ITEM_POTION + 86:
	case ITEM_POTION + 87:
	case ITEM_POTION + 133:
		if (m_iHotKeyItemType[iHotKey] != ITEM_POTION + 20 || m_iHotKeyItemLevel[iHotKey] == 0)
		{
			iStart = iEnd = m_iHotKeyItemType[iHotKey];
			return true;
		}
		break;
	default:
		if (m_iHotKeyItemType[iHotKey] >= ITEM_POTION + 35 && m_iHotKeyItemType[iHotKey] <= ITEM_POTION + 37)
		{
			iStart = ITEM_POTION + 37; iEnd = ITEM_POTION + 35;
			return true;
		}
		else if (m_iHotKeyItemType[iHotKey] >= ITEM_POTION + 38 && m_iHotKeyItemType[iHotKey] <= ITEM_POTION + 40)
		{
			iStart = ITEM_POTION + 40; iEnd = ITEM_POTION + 38;
			return true;
		}
		break;
	}
	return false;
}

int SEASON3B::CNewUIItemHotKey::GetHotKeyItemCount(int iType)
{
	if (iType < 0 || iType >= HOTKEY_COUNT) //newhudvictoriuri - evita leitura fora dos atalhos Q/W/E.
		return 0;
	return GetHotKeyItemIndex(iType, true); //newhudvictoriuri - quantidade real somada do inventario.
}

void SEASON3B::CNewUIItemHotKey::SetHotKey(int iHotKey, int iItemType, int iItemLevel)
{
	if (iHotKey != -1 && CNewUIMyInventory::CanRegisterItemHotKey(iItemType) == true
		)
	{
		m_iHotKeyItemType[iHotKey] = iItemType;
		m_iHotKeyItemLevel[iHotKey] = iItemLevel;
	}
	else
	{
		m_iHotKeyItemType[iHotKey] = -1;
		m_iHotKeyItemLevel[iHotKey] = 0;
	}
}

int SEASON3B::CNewUIItemHotKey::GetHotKey(int iHotKey)
{
	if (iHotKey != -1)
	{
		return m_iHotKeyItemType[iHotKey];
	}

	return -1;
}

int SEASON3B::CNewUIItemHotKey::GetHotKeyLevel(int iHotKey)
{
	if (iHotKey != -1)
	{
		return m_iHotKeyItemLevel[iHotKey];
	}

	return 0;
}

void SEASON3B::CNewUIItemHotKey::RenderItems()
{
	float x, y, width, height;


	for (int i = 0; i < HOTKEY_COUNT; ++i)
	{
		int iIndex = GetHotKeyItemIndex(i);
		if (iIndex != -1)
		{
			ITEM* pItem = g_pMyInventory->FindItem(iIndex);
			if (pItem)
			{
#if CLIENT_VERSION >= 401 && CLIENT_VERSION <= 502
				x = gWideScreen.GetCenterPosition(10) + (i * 38); y = gWideScreen.GetBottomPosition(443); width = 20; height = 20;
#elif CLIENT_VERSION >= 1001
				//newhudvictoriuri - HUD das potions/itens (Q/W/E/R) reancorado nos slots reais da HUD MuAway.
				const int hudItemElement = HUD_EDITOR_POTION_Q + i; //newhudvictoriuri
				if (!NewHudEditorVisible(hudItemElement))
					continue;
				x = NewHudVictoriuriX(NewHudEditorX(hudItemElement));
				y = NewHudVictoriuriY(NewHudEditorY(hudItemElement));
				width = NewHudVictoriuriSize(NewHudEditorW(hudItemElement));
				height = NewHudVictoriuriSize(NewHudEditorH(hudItemElement)); //newhudvictoriuri - V24.6.8
#endif
				RenderItem3D(x, y, width, height, pItem->Type, pItem->Level, 0, 0);
			}
		}
	}
}

void SEASON3B::CNewUIItemHotKey::RenderItemCount()
{
	float x, y, width, height;


	for (int i = 0; i < HOTKEY_COUNT; ++i)
	{
		int iCount = GetHotKeyItemCount(i); //newhudvictoriuri - usa a mesma contagem do inventario.
		//newhudvictoriuri - algumas bases atualizam FindItem() antes da lista visual
		// do inventario; nesse intervalo, recupera a durabilidade do item do atalho.
		if (iCount <= 0)
		{
			const int itemIndex = GetHotKeyItemIndex(i); //newhudvictoriuri
			if (itemIndex != -1)
			{
				ITEM* pHotKeyItem = g_pMyInventory->FindItem(itemIndex); //newhudvictoriuri
				if (pHotKeyItem != NULL)
					iCount = (pHotKeyItem->Durability > 0) ? pHotKeyItem->Durability : 1; //newhudvictoriuri
			}
		}
		if (iCount > 0)
		{
#if CLIENT_VERSION >= 401 && CLIENT_VERSION <= 502
			glColor4f(1.f, 1.f, 1.f, 1.f);
			x = gWideScreen.GetCenterPosition(30) + (i * 38); y = 457; width = 8; height = 9;
			SEASON3B::RenderNumber(x, y, iCount);
#elif CLIENT_VERSION >= 1001
			glColor4f(1.0f, 0.81f, 0.55f, 1.f);
			//newhudvictoriuri - V28: quantidade da potion vai para o buraquinho circular vazio acima de Q/W/E.
			// O contador e ancorado no FRAME (onde o buraco realmente esta), e nao na imagem 3D da potion.
			const int hudItemElement = HUD_EDITOR_POTION_Q + i; //newhudvictoriuri
			if (!NewHudEditorVisible(hudItemElement) || !NewHudEditorVisible(HUD_EDITOR_FRAME))
				continue;

			//newhudvictoriuri - Coordenadas centrais dos 3 buracos no PSD 1920x1080.
			static const float potionCountHoleX[HOTKEY_COUNT] = { 783.0f, 840.0f, 897.0f };
			const float potionCountHoleY = 1025.0f; // topo do numero, centralizado verticalmente dentro do buraco.

			//newhudvictoriuri - Mantem o numero preso ao buraco mesmo se o FRAME for movido/redimensionado no /hud.
			const float frameBaseX = 145.09f;
			const float frameBaseY = 991.00f;
			const float frameBaseW = 1634.00f;
			const float frameBaseH = 89.00f;
			const float frameX = NewHudEditorX(HUD_EDITOR_FRAME);
			const float frameY = NewHudEditorY(HUD_EDITOR_FRAME);
			const float frameW = NewHudEditorW(HUD_EDITOR_FRAME);
			const float frameH = NewHudEditorH(HUD_EDITOR_FRAME);

			const float holeX = frameX + ((potionCountHoleX[i] - frameBaseX) / frameBaseW) * frameW; //newhudvictoriuri
			const float holeY = frameY + ((potionCountHoleY - frameBaseY) / frameBaseH) * frameH; //newhudvictoriuri
			x = NewHudVictoriuriX(holeX);
			y = NewHudVictoriuriY(holeY);

			//newhudvictoriuri - 0.90 cabe os 3 digitos dentro do circulo sem invadir a potion ou a letra Q/W/E.
			const float frameScaleX = frameW / frameBaseW;
			const float frameScaleY = frameH / frameBaseH;
			const float countScale = 0.90f * min(frameScaleX, frameScaleY);
			width = NewHudVictoriuriSize(8.0f * countScale);
			height = NewHudVictoriuriSize(9.0f * countScale); //newhudvictoriuri
			SEASON3B::RenderNumber(x, y, iCount, NewHudVictoriuriItemCountScale(countScale)); //newhudvictoriuri - contador dentro do buraco
#endif
		}
	}
}

void SEASON3B::CNewUIItemHotKey::UseItemRButton()
{
	int x, y, width, height;

	for (int i = 0; i < HOTKEY_COUNT; ++i)
	{
#if CLIENT_VERSION >= 1001
		//newhudvictoriuri - Hitbox dos itens/potions acompanha o novo encaixe do HUD MuAway.
		const int hudItemElement = HUD_EDITOR_POTION_Q + i; //newhudvictoriuri
		if (!NewHudEditorVisible(hudItemElement))
			continue;
		x = (int)NewHudVictoriuriX(NewHudEditorX(hudItemElement));
		y = (int)NewHudVictoriuriY(NewHudEditorY(hudItemElement));
		width = (int)NewHudVictoriuriSize(NewHudEditorW(hudItemElement));
		height = (int)NewHudVictoriuriSize(NewHudEditorH(hudItemElement)); //newhudvictoriuri - V24.6.8
#else
		x = gWideScreen.GetCenterPosition(10) + (i * 38); y = gWideScreen.GetBottomPosition(445); width = 20; height = 20;
#endif
		if (SEASON3B::CheckMouseIn(x, y, width, height) == true)
		{
			if (MouseRButtonPush)
			{
				MouseRButtonPush = false;
				int iIndex = GetHotKeyItemIndex(i);
				if (iIndex != -1)
				{
					SendRequestUse(iIndex, 0);
					break;
				}
			}
		}
	}
}

SEASON3B::CNewUISkillList::CNewUISkillList()
{
	m_pNewUIMng = NULL;
	Reset();
}

SEASON3B::CNewUISkillList::~CNewUISkillList()
{
	Release();
}

bool SEASON3B::CNewUISkillList::Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng)
{
	if (NULL == pNewUIMng)
		return false;

	m_pNewUIMng = pNewUIMng;
	m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_SKILL_LIST, this);

	m_pNewUI3DRenderMng = pNewUI3DRenderMng;

	LoadImages();

#if CLIENT_VERSION >= 1001
	SetButtonInfo();
#endif

	Show(true);

	return true;
}

#if CLIENT_VERSION >= 1001
void SEASON3B::CNewUISkillList::SetButtonInfo()
{
	m_SkillListButtonUp.ChangeButtonImgState(true, IMAGE_SKILL_ARROW_UP, true);
	m_SkillListButtonUp.ChangeButtonInfo(
		(int)NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_SKILL_ARROW_UP)),
		(int)NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_SKILL_ARROW_UP)),
		(int)NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_SKILL_ARROW_UP)),
		(int)NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_SKILL_ARROW_UP))); //newhudvictoriuri
	m_SkillListButtonUp.ForceButtonDown(true);
	m_SkillListButtonUp.Lock();

	m_SkillListButtonDown.ChangeButtonImgState(true, IMAGE_SKILL_ARROW_DOWN, true);
	m_SkillListButtonDown.ChangeButtonInfo(
		(int)NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_SKILL_ARROW_DOWN)),
		(int)NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_SKILL_ARROW_DOWN)),
		(int)NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_SKILL_ARROW_DOWN)),
		(int)NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_SKILL_ARROW_DOWN))); //newhudvictoriuri
	m_SkillListButtonDown.ForceButtonDown(true);
	m_SkillListButtonDown.Lock();

}
#endif

void SEASON3B::CNewUISkillList::Release()
{
	if (m_pNewUI3DRenderMng)
	{
		m_pNewUI3DRenderMng->DeleteUI2DEffectObject(UI2DEffectCallback);
	}

	UnloadImages();

	if (m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj(this);
		m_pNewUIMng = NULL;
	}
}

void SEASON3B::CNewUISkillList::Reset()
{
	m_bSkillList = false;
	m_bHotKeySkillListUp = false;

#if CLIENT_VERSION >= 1001
	m_SkillPage = 0;
	m_SkillListButtonUp.ForceButtonDown(true);
	m_SkillListButtonUp.Lock();

	m_SkillListButtonDown.ForceButtonDown(true);
	m_SkillListButtonDown.Lock();
#endif
	m_bRenderSkillInfo = false;
	m_iRenderSkillInfoType = 0;
	m_iRenderSkillInfoPosX = 0;
	m_iRenderSkillInfoPosY = 0;

	for (int i = 0; i < SKILLHOTKEY_COUNT; ++i)
	{
		m_iHotKeySkillType[i] = -1;
	}

	m_EventState = EVENT_NONE;
}

void SEASON3B::CNewUISkillList::LoadImages()
{
#if CLIENT_VERSION >= 402 && CLIENT_VERSION <= 502
	LoadBitmap("Interface\\newui_skill.jpg", IMAGE_SKILL1, GL_LINEAR);
	LoadBitmap("Interface\\newui_skill2.jpg", IMAGE_SKILL2, GL_LINEAR);
	LoadBitmap("Interface\\newui_command.jpg", IMAGE_COMMAND, GL_LINEAR);
	LoadBitmap("Interface\\newui_skillbox.jpg", IMAGE_SKILLBOX, GL_LINEAR);
	LoadBitmap("Interface\\newui_skillbox2.jpg", IMAGE_SKILLBOX_USE, GL_LINEAR);
	LoadBitmap("Interface\\newui_non_skill.jpg", IMAGE_NON_SKILL1, GL_LINEAR);
	LoadBitmap("Interface\\newui_non_skill2.jpg", IMAGE_NON_SKILL2, GL_LINEAR);
	LoadBitmap("Interface\\newui_non_command.jpg", IMAGE_NON_COMMAND, GL_LINEAR);
#ifdef PBG_ADD_NEWCHAR_MONK_SKILL
	LoadBitmap("Interface\\newui_skill3.jpg", IMAGE_SKILL3, GL_LINEAR);
	LoadBitmap("Interface\\newui_non_skill3.jpg", IMAGE_NON_SKILL3, GL_LINEAR);
#endif //PBG_ADD_NEWCHAR_MONK_SKILL
#elif CLIENT_VERSION >= 1001
	//newhudvictoriuri - Atlas de skills e comandos da HUD MuAway.
	LoadBitmap("Custom\\Interface\\HUD_MuAway\\away_skill1.tga", IMAGE_SKILL1, GL_LINEAR); //newhudvictoriuri
	LoadBitmap("Custom\\Interface\\HUD_MuAway\\away_skill2.tga", IMAGE_SKILL2, GL_LINEAR); //newhudvictoriuri
	LoadBitmap("Custom\\Interface\\HUD_MuAway\\away_command.tga", IMAGE_COMMAND, GL_LINEAR); //newhudvictoriuri
	LoadBitmap("Interface\\newui_skillbox.jpg", IMAGE_SKILLBOX, GL_LINEAR);
	LoadBitmap("Interface\\newui_skillbox2.jpg", IMAGE_SKILLBOX_USE, GL_LINEAR);
	//LoadBitmap("Interface\\newui_non_skill.jpg", IMAGE_NON_SKILL1, GL_LINEAR);
	//("Interface\\newui_non_skill2.jpg", IMAGE_NON_SKILL2, GL_LINEAR);
	LoadBitmap("Interface\\newui_non_command.jpg", IMAGE_NON_COMMAND, GL_LINEAR);
#ifdef PBG_ADD_NEWCHAR_MONK_SKILL
	//newhudvictoriuri - Terceiro atlas da HUD MuAway quando Monk estiver habilitado.
	LoadBitmap("Custom\\Interface\\HUD_MuAway\\away_skill3.tga", IMAGE_SKILL3, GL_LINEAR); //newhudvictoriuri
	LoadBitmap("Interface\\newui_non_skill3.jpg", IMAGE_NON_SKILL3, GL_LINEAR);
#endif //PBG_ADD_NEWCHAR_MONK_SKILL
	LoadBitmap("Interface\\skill_render.jpg", IMAGE_SKILLBOX_LARGE, GL_LINEAR);
	LoadBitmap("Interface\\skill_render_on.tga", IMAGE_SKILL_USE, GL_LINEAR);
	LoadBitmap("Interface\\skill_render_page.jpg", IMAGE_SKILL_PAGE, GL_LINEAR);
	LoadBitmap("Interface\\btn_arrowup.jpg", IMAGE_SKILL_ARROW_UP, GL_LINEAR);
	LoadBitmap("Interface\\btn_arrowdown.jpg", IMAGE_SKILL_ARROW_DOWN, GL_LINEAR);

#endif
}

void SEASON3B::CNewUISkillList::UnloadImages()
{
	DeleteBitmap(IMAGE_SKILL1);
	DeleteBitmap(IMAGE_SKILL2);
	DeleteBitmap(IMAGE_COMMAND);
	DeleteBitmap(IMAGE_SKILLBOX);
	DeleteBitmap(IMAGE_SKILLBOX_USE);
	//DeleteBitmap(IMAGE_NON_SKILL1);
	//DeleteBitmap(IMAGE_NON_SKILL2);
	DeleteBitmap(IMAGE_NON_COMMAND);
#ifdef PBG_ADD_NEWCHAR_MONK_SKILL
	DeleteBitmap(IMAGE_SKILL3);
	DeleteBitmap(IMAGE_NON_SKILL3);
#endif //PBG_ADD_NEWCHAR_MONK_SKILL
#if CLIENT_VERSION >= 1001
	DeleteBitmap(IMAGE_SKILLBOX_LARGE);
	DeleteBitmap(IMAGE_SKILL_USE);
	DeleteBitmap(IMAGE_SKILL_PAGE);
	DeleteBitmap(IMAGE_SKILL_ARROW_UP);
	DeleteBitmap(IMAGE_SKILL_ARROW_DOWN);
#endif
}

// TOOLTIP E CLICK
bool SEASON3B::CNewUISkillList::UpdateMouseEvent()
{
	if (g_isCharacterBuff((&Hero->Object), eBuff_DuelWatch))
	{
		m_bSkillList = false;
		return true;
	}

	BYTE bySkillNumber = CharacterAttribute->SkillNumber;
	BYTE bySkillMasterNumber = CharacterAttribute->SkillMasterNumber;

	float x, y, width, height;

	m_bRenderSkillInfo = false;

	if (bySkillNumber <= 0)
	{
		return true;
	}
#if CLIENT_VERSION >= 402 && CLIENT_VERSION <= 502
	x = gWideScreen.GetCenterPosition(385); y = 431.f; width = 32.f; height = 38.f;
#elif CLIENT_VERSION >= 1001
	//newhudvictoriuri - Hitbox da skill atual acompanhando o slot central correto.
	x = NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_SKILL_CURRENT)); //newhudvictoriuri - V24 PSD
	y = NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_SKILL_CURRENT)); //newhudvictoriuri - V24 PSD
	width = NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_SKILL_CURRENT));
	height = NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_SKILL_CURRENT)); //newhudvictoriuri
	if (!NewHudEditorVisible(HUD_EDITOR_SKILL_CURRENT))
		width = height = 0.0f; //newhudvictoriuri
#endif
	if (m_EventState == EVENT_NONE && MouseLButtonPush == false
		&& SEASON3B::CheckMouseIn(x, y, width, height) == true)
	{
		m_EventState = EVENT_BTN_HOVER_CURRENTSKILL;
		return true;
	}
	if (m_EventState == EVENT_BTN_HOVER_CURRENTSKILL && MouseLButtonPush == false
		&& SEASON3B::CheckMouseIn(x, y, width, height) == false)
	{
		m_EventState = EVENT_NONE;
		return true;
	}
	if (m_EventState == EVENT_BTN_HOVER_CURRENTSKILL && (MouseLButtonPush == true || MouseLButtonDBClick == true)
		&& SEASON3B::CheckMouseIn(x, y, width, height) == true)
	{
#if CLIENT_VERSION >= 1001
		if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHATINPUTBOX))
#endif
			m_EventState = EVENT_BTN_DOWN_CURRENTSKILL;
		return false;
	}
	if (m_EventState == EVENT_BTN_DOWN_CURRENTSKILL)
	{
		if (MouseLButtonPush == false && MouseLButtonDBClick == false)
		{
			if (SEASON3B::CheckMouseIn(x, y, width, height) == true)
			{
				m_bSkillList = !m_bSkillList;

				PlayBuffer(SOUND_CLICK01);

#if CLIENT_VERSION >= 1001
				m_SkillPage = 0;
				m_SkillListButtonUp.ForceButtonDown(true);
				m_SkillListButtonUp.Lock();

				m_SkillListButtonDown.ForceButtonDown(true);
				m_SkillListButtonDown.Lock();
#endif
				m_EventState = EVENT_NONE;
				return false;
			}
			m_EventState = EVENT_NONE;
			return true;
		}

	}

	if (m_EventState == EVENT_BTN_HOVER_CURRENTSKILL)
	{
		if (m_bSkillList) {
			m_EventState = EVENT_NONE;
			return false;
		}
		m_bRenderSkillInfo = true;
		m_iRenderSkillInfoType = Hero->CurrentSkill;
		m_iRenderSkillInfoPosX = x + NewHudVictoriuriSize(10.0f); //newhudvictoriuri - V12: tooltip acompanha a skill escalada.
		m_iRenderSkillInfoPosY = y - NewHudVictoriuriSize(10.0f); //newhudvictoriuri - V12: tooltip acompanha a escala vertical.

		return false;
	}
	else if (m_EventState == EVENT_BTN_DOWN_CURRENTSKILL)
	{
		return false;
	}
#if CLIENT_VERSION >= 402 && CLIENT_VERSION <= 502
	x = gWideScreen.GetCenterPosition(222); y = 431.f; width = 32.f * 5.f; height = 38.f;
	if (m_EventState == EVENT_NONE && MouseLButtonPush == false
		&& SEASON3B::CheckMouseIn(x, y, width, height) == true)
	{
		m_EventState = EVENT_BTN_HOVER_SKILLHOTKEY;
		return true;
	}
	if (m_EventState == EVENT_BTN_HOVER_SKILLHOTKEY && MouseLButtonPush == false
		&& SEASON3B::CheckMouseIn(x, y, width, height) == false)
	{
		m_EventState = EVENT_NONE;
		return true;
	}
	if (m_EventState == EVENT_BTN_HOVER_SKILLHOTKEY && MouseLButtonPush == true
		&& SEASON3B::CheckMouseIn(x, y, width, height) == true)
	{
		m_EventState = EVENT_BTN_DOWN_SKILLHOTKEY;
		return false;
	}

	x = gWideScreen.GetCenterPosition(190); y = 431.f; width = 32.f; height = 38.f;
	int iStartIndex = (m_bHotKeySkillListUp == true) ? 6 : 1;
	for (int i = 0, iIndex = iStartIndex; i < 5; ++i, iIndex++)
	{
		x += width;

		if (iIndex == 10)
		{
			iIndex = 0;
		}
		if (SEASON3B::CheckMouseIn(x, y, width, height) == true)
		{
			if (m_iHotKeySkillType[iIndex] == -1)
			{
				if (m_EventState == EVENT_BTN_HOVER_SKILLHOTKEY)
				{
					m_bRenderSkillInfo = false;
					m_iRenderSkillInfoType = -1;
				}
				if (m_EventState == EVENT_BTN_DOWN_SKILLHOTKEY && MouseLButtonPush == false)
				{
					m_EventState = EVENT_NONE;
				}
				continue;
			}

			WORD bySkillType = CharacterAttribute->Skill[m_iHotKeySkillType[iIndex]];

			if (bySkillType == 0 || (bySkillType >= AT_SKILL_STUN && bySkillType <= AT_SKILL_REMOVAL_BUFF))
				continue;

			BYTE bySkillUseType = SkillAttribute[bySkillType].SkillUseType;

			if (bySkillUseType == SKILL_USE_TYPE_MASTERLEVEL)
			{
				continue;
			}

			if (m_EventState == EVENT_BTN_HOVER_SKILLHOTKEY)
			{
				m_bRenderSkillInfo = true;
				m_iRenderSkillInfoType = m_iHotKeySkillType[iIndex];
				m_iRenderSkillInfoPosX = x - 5;
				m_iRenderSkillInfoPosY = y;
				return true;
			}
			if (m_EventState == EVENT_BTN_DOWN_SKILLHOTKEY)
			{
				if (MouseLButtonPush == false)
				{
					if (m_iRenderSkillInfoType == m_iHotKeySkillType[iIndex])
					{
						m_EventState = EVENT_NONE;
						m_wHeroPriorSkill = CharacterAttribute->Skill[Hero->CurrentSkill];
						Hero->CurrentSkill = m_iHotKeySkillType[iIndex];
						PlayBuffer(SOUND_CLICK01);
						return false;
					}
					else
					{
						m_EventState = EVENT_NONE;
					}
				}
			}
		}
	}

	x = gWideScreen.GetCenterPosition(222); y = 431.f; width = 32.f * 5.f; height = 38.f;
	if (m_EventState == EVENT_BTN_DOWN_SKILLHOTKEY)
	{
		if (MouseLButtonPush == false && SEASON3B::CheckMouseIn(x, y, width, height) == false)
		{
			m_EventState = EVENT_NONE;
			return true;
		}
		return false;
	}

	if (m_bSkillList == false)
		return true;

	WORD bySkillType = 0;

	int iSkillCount = 0;
	bool bMouseOnSkillList = false;

	x = gWideScreen.GetCenterPosition(385); y = 390; width = 32; height = 38;
	float fOrigX = x;// 385.f;

	EVENT_STATE PrevEventState = m_EventState;

	for (int i = 0; i < MAX_MAGIC; ++i)
	{
		bySkillType = CharacterAttribute->Skill[i];

		if (bySkillType == 0 || (bySkillType >= AT_SKILL_STUN && bySkillType <= AT_SKILL_REMOVAL_BUFF))
			continue;

		BYTE bySkillUseType = SkillAttribute[bySkillType].SkillUseType;

		if (bySkillUseType == SKILL_USE_TYPE_MASTERLEVEL)
		{
			continue;
		}


		if (iSkillCount == 18)
		{
			y -= height;
		}

		if (iSkillCount < 14)
		{
			int iRemainder = iSkillCount % 2;
			int iQuotient = iSkillCount / 2;

			if (iRemainder == 0)
			{
				x = fOrigX + iQuotient * width;
			}
			else
			{
				x = fOrigX - (iQuotient + 1) * width;
			}
		}
		else if (iSkillCount >= 14 && iSkillCount < 18)
		{
			x = fOrigX - (8 * width) - ((iSkillCount - 14) * width);
		}
		else
		{
			x = fOrigX - (12 * width) + ((iSkillCount - 17) * width);
		}

		iSkillCount++;

		if (SEASON3B::CheckMouseIn(x, y, width, height) == true)
		{
			bMouseOnSkillList = true;
			if (m_EventState == EVENT_NONE && MouseLButtonPush == false)
			{
				m_EventState = EVENT_BTN_HOVER_SKILLLIST;
				break;
			}
		}

		if (m_EventState == EVENT_BTN_HOVER_SKILLLIST && MouseLButtonPush == true
			&& SEASON3B::CheckMouseIn(x, y, width, height) == true)
		{
#if CLIENT_VERSION >= 1001
			if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHATINPUTBOX))
#endif
				m_EventState = EVENT_BTN_DOWN_SKILLLIST;
			break;
		}

		if (m_EventState == EVENT_BTN_HOVER_SKILLLIST && MouseLButtonPush == false
			&& SEASON3B::CheckMouseIn(x, y, width, height) == true)
		{
			m_bRenderSkillInfo = true;
			m_iRenderSkillInfoType = i;
			m_iRenderSkillInfoPosX = x;
			m_iRenderSkillInfoPosY = y;
		}

		if (m_EventState == EVENT_BTN_DOWN_SKILLLIST && MouseLButtonPush == false
			&& m_iRenderSkillInfoType == i && SEASON3B::CheckMouseIn(x, y, width, height) == true)
		{
			m_EventState = EVENT_NONE;

			m_wHeroPriorSkill = CharacterAttribute->Skill[Hero->CurrentSkill];

			Hero->CurrentSkill = i;
			m_bSkillList = false;

			PlayBuffer(SOUND_CLICK01);
			return false;
		}
	}
#elif CLIENT_VERSION >= 1001

	if (m_bSkillList == false)
		return true;

	if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHATINPUTBOX))
	{
		m_bSkillList = !m_bSkillList;
		m_SkillPage = 0;
		m_SkillListButtonUp.ForceButtonDown(true);
		m_SkillListButtonUp.Lock();

		m_SkillListButtonDown.ForceButtonDown(true);
		m_SkillListButtonDown.Lock();
		return true;
	}

	std::vector<int> m_skillList;

	m_skillList.clear();

	int skillOffset = 0;

	int skillCount = 0;

	bool bMouseOnSkillList = false;

	//newhudvictoriuri - V12: hitbox da lista usa exatamente a mesma escala do fundo/slots.
	const float popupWidth = NewHudEditorW(HUD_EDITOR_SKILL_POPUP); //newhudvictoriuri
	const float popupHeight = NewHudEditorH(HUD_EDITOR_SKILL_POPUP); //newhudvictoriuri
	const float popupScale = min(popupWidth / 700.0f, popupHeight / 162.0f); //newhudvictoriuri
	x = NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_SKILL_POPUP));
	y = NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_SKILL_POPUP));
	width = NewHudVictoriuriSize(70.0f * popupScale);
	height = NewHudVictoriuriSize(80.0f * popupScale); //newhudvictoriuri
	if (!NewHudEditorVisible(HUD_EDITOR_SKILL_POPUP))
		width = height = 0.0f; //newhudvictoriuri

	float originalX = x + NewHudVictoriuriSize(14.0f * popupScale); //newhudvictoriuri - mesma margem usada no desenho.

	if (Hero->m_pPet == NULL) goto RENDERSKILL;

	for (int i = AT_PET_COMMAND_DEFAULT; i < AT_PET_COMMAND_END; ++i)
	{
		m_skillList.push_back(i);
	}

RENDERSKILL:
	for (int i = 0; i < MAX_MAGIC; ++i)
	{
		int iSkillType = CharacterAttribute->Skill[i];

		if (iSkillType == 0 || (iSkillType >= AT_SKILL_STUN && iSkillType <= AT_SKILL_REMOVAL_BUFF)) continue;

		BYTE bySkillUseType = SkillAttribute[iSkillType].SkillUseType;

		if (bySkillUseType == SKILL_USE_TYPE_MASTER || bySkillUseType == SKILL_USE_TYPE_MASTERLEVEL) continue;

		m_skillList.push_back(i);
	}

	skillOffset += m_SkillPage * 10;

	for (std::vector<int>::iterator it = m_skillList.begin() + skillOffset; it != m_skillList.end(); ++it)
	{
		float tempX = originalX;

		if (skillCount < 10) {
			tempX += (skillCount * NewHudVictoriuriSize(70.0f * popupScale));
		}
		else {
			tempX += ((skillCount - 10) * NewHudVictoriuriSize(70.0f * popupScale));
		}

		float tempY = y + (skillCount > 9 ? NewHudVictoriuriSize(80.0f * popupScale) : 0.0f);

		if (SEASON3B::CheckMouseIn(tempX, tempY, width, height) == true)
		{
			bMouseOnSkillList = true;
			if (m_EventState == EVENT_NONE && MouseLButtonPush == false)
			{
				m_EventState = EVENT_BTN_HOVER_SKILLLIST;
				break;
			}
		}

		if (m_EventState == EVENT_BTN_HOVER_SKILLLIST && MouseLButtonPush == true
			&& SEASON3B::CheckMouseIn(tempX, tempY, width, height) == true)
		{
			if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHATINPUTBOX))
				m_EventState = EVENT_BTN_DOWN_SKILLLIST;
			break;
		}

		if (m_EventState == EVENT_BTN_HOVER_SKILLLIST && MouseLButtonPush == false
			&& SEASON3B::CheckMouseIn(tempX, tempY, width, height) == true)
		{
			m_bRenderSkillInfo = true;
			m_iRenderSkillInfoType = *it;
			m_iRenderSkillInfoPosX = tempX;
			m_iRenderSkillInfoPosY = tempY;
		}

		if (m_EventState == EVENT_BTN_DOWN_SKILLLIST && MouseLButtonPush == false
			&& m_iRenderSkillInfoType == *it && SEASON3B::CheckMouseIn(tempX, tempY, width, height) == true)
		{
			m_EventState = EVENT_NONE;

			m_wHeroPriorSkill = CharacterAttribute->Skill[Hero->CurrentSkill];

			Hero->CurrentSkill = *it;
			m_bSkillList = false;

			PlayBuffer(SOUND_CLICK01);
			return false;
		}

		skillCount++;
		if (skillCount == 20) break;
	}

#endif

	if (bMouseOnSkillList == false && m_EventState == EVENT_BTN_HOVER_SKILLLIST)
	{
		m_EventState = EVENT_NONE;
		return true;
	}
	if (bMouseOnSkillList == false && MouseLButtonPush == false
		&& m_EventState == EVENT_BTN_DOWN_SKILLLIST)
	{
		m_EventState = EVENT_NONE;
		return false;
	}
	if (m_EventState == EVENT_BTN_DOWN_SKILLLIST)
	{
		if (MouseLButtonPush == false)
		{
			m_EventState = EVENT_NONE;
			return true;
		}
		return false;
	}
#if CLIENT_VERSION >= 1001

	if (m_SkillPage == 0) {
		m_SkillListButtonUp.Lock();
		m_SkillListButtonUp.ForceButtonDown(true);
	}
	else {
		m_SkillListButtonUp.UnLock();
		m_SkillListButtonUp.ForceButtonDown(false);
	}

	if (m_skillList.size() < 20 || m_SkillPage == 1) {
		m_SkillListButtonDown.Lock();
		m_SkillListButtonDown.ForceButtonDown(true);
	}
	else {
		m_SkillListButtonDown.UnLock();
		m_SkillListButtonDown.ForceButtonDown(false);
	}

	if (m_SkillListButtonUp.UpdateMouseEvent() == true && m_SkillPage > 0)
	{

		m_SkillPage--;
		PlayBuffer(SOUND_CLICK01);
		m_EventState = EVENT_NONE;
		return false;

	}

	if (m_SkillListButtonDown.UpdateMouseEvent() == true && m_skillList.size() > 20)
	{

		if (m_SkillPage < 1) {
			m_SkillPage++;
			PlayBuffer(SOUND_CLICK01);
			m_EventState = EVENT_NONE;
			return false;
		}
	}


	if (m_bSkillList && NewHudEditorVisible(HUD_EDITOR_SKILL_POPUP)
		&& SEASON3B::CheckMouseIn(NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_SKILL_POPUP)),
			NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_SKILL_POPUP)),
			NewHudVictoriuriSize(popupWidth), NewHudVictoriuriSize(popupHeight))) //newhudvictoriuri - V13
	{
		return false;
	}

#endif

#if CLIENT_VERSION >= 1001
	if (SEASON3B::CheckMouseIn(x + NewHudVictoriuriSize(355.0f * popupScale),
		y + NewHudVictoriuriSize(22.5f * popupScale),
		NewHudVictoriuriSize(18.0f * popupScale), NewHudVictoriuriSize(36.0f * popupScale)) == true) //newhudvictoriuri
	{
		m_EventState = EVENT_NONE;
		return false;
	}
#endif

	return true;
}

bool SEASON3B::CNewUISkillList::UpdateKeyEvent()
{
	for (int i = 0; i < 9; ++i)
	{
		if (SEASON3B::IsPress('1' + i))
		{
			UseHotKey(i + 1);
		}
	}

	if (SEASON3B::IsPress('0'))
	{
		UseHotKey(0);
	}

	if (m_EventState == EVENT_BTN_HOVER_SKILLLIST)
	{
		if (SEASON3B::IsRepeat(VK_CONTROL))
		{
			for (int i = 0; i < 9; ++i)
			{
				if (SEASON3B::IsPress('1' + i))
				{
					SetHotKey(i + 1, m_iRenderSkillInfoType);

					return false;
				}
			}

			if (SEASON3B::IsPress('0'))
			{
				SetHotKey(0, m_iRenderSkillInfoType);

				return false;
			}
		}
	}

	if (SEASON3B::IsRepeat(VK_SHIFT))
	{
		for (int i = 0; i < 4; ++i)
		{
			if (SEASON3B::IsPress('1' + i))
			{
				Hero->CurrentSkill = AT_PET_COMMAND_DEFAULT + i;
				return false;
			}
		}
	}

	return true;
}

bool SEASON3B::CNewUISkillList::IsArrayUp(BYTE bySkill)
{
	for (int i = 0; i < SKILLHOTKEY_COUNT; ++i)
	{
		if (m_iHotKeySkillType[i] == bySkill)
		{
			if (i == 0 || i > 5)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}

	return false;
}

bool SEASON3B::CNewUISkillList::IsArrayIn(BYTE bySkill)
{
	for (int i = 0; i < SKILLHOTKEY_COUNT; ++i)
	{
		if (m_iHotKeySkillType[i] == bySkill)
		{
			return true;
		}
	}

	return false;
}

void SEASON3B::CNewUISkillList::SetHotKey(int iHotKey, int iSkillType)
{
	for (int i = 0; i < SKILLHOTKEY_COUNT; ++i)
	{
		if (m_iHotKeySkillType[i] == iSkillType)
		{
			m_iHotKeySkillType[i] = -1;
			break;
		}
	}

	m_iHotKeySkillType[iHotKey] = iSkillType;
}

int SEASON3B::CNewUISkillList::GetHotKey(int iHotKey)
{
	return m_iHotKeySkillType[iHotKey];
}

int SEASON3B::CNewUISkillList::GetSkillIndex(int iSkillType)
{
	int iReturn = -1;
	for (int i = 0; i < MAX_MAGIC; ++i)
	{
		if (CharacterAttribute->Skill[i] == iSkillType)
		{
			iReturn = i;
			break;
		}
	}

	return iReturn;
}

void SEASON3B::CNewUISkillList::UseHotKey(int iHotKey)
{
	if (m_iHotKeySkillType[iHotKey] != -1)
	{
		if (m_iHotKeySkillType[iHotKey] >= AT_PET_COMMAND_DEFAULT && m_iHotKeySkillType[iHotKey] < AT_PET_COMMAND_END)
		{
			if (Hero->m_pPet == NULL)
			{
				return;
			}
		}

		WORD wHotKeySkill = CharacterAttribute->Skill[m_iHotKeySkillType[iHotKey]];

		if (wHotKeySkill == 0)
		{
			return;
		}

		m_wHeroPriorSkill = CharacterAttribute->Skill[Hero->CurrentSkill];

		Hero->CurrentSkill = m_iHotKeySkillType[iHotKey];

		WORD bySkill = CharacterAttribute->Skill[Hero->CurrentSkill];


		if (
			g_pOption->IsAutoAttack() == true
			&& gMapManager.WorldActive != WD_6STADIUM
			&& gMapManager.InChaosCastle() == false
			&& (bySkill == AT_SKILL_TELEPORT || bySkill == AT_SKILL_TELEPORT_B))
		{
			SelectedCharacter = -1;
			Attacking = -1;
		}
	}
}

bool SEASON3B::CNewUISkillList::Update()
{
#if CLIENT_VERSION >= 1001
	//newhudvictoriuri
	// Skill-page controls also cache X/Y. Re-anchor on maximize/restore so every
	// skill control stays attached to the HUD.
	static int s_LastSkillWideWidth = -1;
	static int s_LastSkillWideHeight = -1;
	const int currentSkillWideWidth = (int)gWideScreen.g_WideWindowWidth;
	const int currentSkillWideHeight = (int)gWideScreen.g_WideWindowHeight;
	if (currentSkillWideWidth != s_LastSkillWideWidth || currentSkillWideHeight != s_LastSkillWideHeight)
	{
		SetButtonInfo();
		s_LastSkillWideWidth = currentSkillWideWidth;
		s_LastSkillWideHeight = currentSkillWideHeight;
	}
#endif

	if (IsArrayIn(Hero->CurrentSkill) == true)
	{
		if (IsArrayUp(Hero->CurrentSkill) == true)
		{
			m_bHotKeySkillListUp = true;
		}
		else
		{
			m_bHotKeySkillListUp = false;
		}
	}

	if (Hero->m_pPet == NULL)
	{
		if (Hero->CurrentSkill >= AT_PET_COMMAND_DEFAULT && Hero->CurrentSkill < AT_PET_COMMAND_END)
		{
			Hero->CurrentSkill = 0;
		}
	}

	return true;
}

// SKILL ATUAL
void SEASON3B::CNewUISkillList::RenderCurrentSkillAndHotSkillList()
{
	float x, y, width, height;

	BYTE bySkillNumber = CharacterAttribute->SkillNumber;

	if (bySkillNumber > 0)
	{
#if CLIENT_VERSION >= 402 && CLIENT_VERSION <= 502
		int i;
		int iStartSkillIndex = 1;
		if (m_bHotKeySkillListUp)
		{
			iStartSkillIndex = 6;
		}

		x = gWideScreen.GetCenterPosition(190) - 0.5f; y = 431.5f; width = 32; height = 38;
		for (i = 0; i < 5; ++i)
		{
			x += width;

			int iIndex = iStartSkillIndex + i;
			if (iIndex == 10)
			{
				iIndex = 0;
			}

			if (m_iHotKeySkillType[iIndex] == -1)
			{
				continue;
			}

			if (m_iHotKeySkillType[iIndex] >= AT_PET_COMMAND_DEFAULT && m_iHotKeySkillType[iIndex] < AT_PET_COMMAND_END)
			{
				if (Hero->m_pPet == NULL)
				{
					continue;
				}
			}

			if (Hero->CurrentSkill == m_iHotKeySkillType[iIndex])
			{
				SEASON3B::RenderImage(IMAGE_SKILLBOX_USE, x, y, width, height);
			}
			RenderSkillIcon(m_iHotKeySkillType[iIndex], x + 6, y + 6, 20, 28);
		}
		x = gWideScreen.GetCenterPosition(392); y = 437; width = 20; height = 28;
#endif
#if CLIENT_VERSION >= 1001

		//newhudvictoriuri - Skill atual reancorada no slot correto da HUD MuAway.
		float slotX = NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_SKILL_CURRENT)); //newhudvictoriuri - V24 PSD
		float slotY = NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_SKILL_CURRENT)); //newhudvictoriuri - V24 PSD
		x = slotX; y = slotY;
		width = NewHudVictoriuriSize(NewHudEditorW(HUD_EDITOR_SKILL_CURRENT));
		height = NewHudVictoriuriSize(NewHudEditorH(HUD_EDITOR_SKILL_CURRENT)); //newhudvictoriuri
		if (NewHudEditorVisible(HUD_EDITOR_SKILL_CURRENT)) //newhudvictoriuri
			RenderSkillIcon(Hero->CurrentSkill, x, y, width, height);
#else
		RenderSkillIcon(Hero->CurrentSkill, x, y, width, height);
#endif
	}
}

// RENDERIZA LISTA SKILLS
bool SEASON3B::CNewUISkillList::Render()
{
#if CLIENT_VERSION >= 402 && CLIENT_VERSION <= 502
	int i;
	float x, y, width, height;

	BYTE bySkillNumber = CharacterAttribute->SkillNumber;

	if (bySkillNumber > 0)
	{
		if (m_bSkillList == true)
		{
			x = gWideScreen.GetCenterPosition(385); y = 390; width = 32; height = 38;
			float fOrigX = gWideScreen.GetCenterPosition(385);
			int iSkillType = 0;
			int iSkillCount = 0;

			for (i = 0; i < MAX_MAGIC; ++i)
			{
				iSkillType = CharacterAttribute->Skill[i];

				if (iSkillType != 0 && (iSkillType < AT_SKILL_STUN || iSkillType > AT_SKILL_REMOVAL_BUFF))
				{
					BYTE bySkillUseType = SkillAttribute[iSkillType].SkillUseType;

					if (bySkillUseType == SKILL_USE_TYPE_MASTER || bySkillUseType == SKILL_USE_TYPE_MASTERLEVEL)
					{
						continue;
					}

					if (iSkillCount == 18)
					{
						y -= height;
					}

					if (iSkillCount < 14)
					{
						int iRemainder = iSkillCount % 2;
						int iQuotient = iSkillCount / 2;

						if (iRemainder == 0)
						{
							x = fOrigX + iQuotient * width;
						}
						else
						{
							x = fOrigX - (iQuotient + 1) * width;
						}
					}
					else if (iSkillCount >= 14 && iSkillCount < 18)
					{
						x = fOrigX - (8 * width) - ((iSkillCount - 14) * width);
					}
					else
					{
						x = fOrigX - (12 * width) + ((iSkillCount - 17) * width);
					}

					iSkillCount++;

					if (i == Hero->CurrentSkill)
					{
						SEASON3B::RenderImage(IMAGE_SKILLBOX_USE, x, y, width, height);
					}
					else
					{
						SEASON3B::RenderImage(IMAGE_SKILLBOX, x, y, width, height);
					}

					RenderSkillIcon(i, x + 6, y + 6, 20, 28);
				}
			}
			RenderPetSkill();
		}
	}

#elif CLIENT_VERSION >= 1001 //newhudvictoriuri - custom MuAway skill list branch
	float x, y, width, height, boxWidth, boxHeight, originalX;

	BYTE bySkillNumber = CharacterAttribute->SkillNumber;

	std::vector<int> m_skillList;

	m_skillList.clear();

	int skillOffset = 0;

	int skillCount = 0;

	if (bySkillNumber < 1 || m_bSkillList == false) goto ENDSKILLRENDER;

	//newhudvictoriuri - V12: popup inteiro (fundo, celulas e pagina) escala como a HUD.
	const float popupWidth = NewHudEditorW(HUD_EDITOR_SKILL_POPUP); //newhudvictoriuri
	const float popupHeight = NewHudEditorH(HUD_EDITOR_SKILL_POPUP); //newhudvictoriuri
	const float popupScale = min(popupWidth / 700.0f, popupHeight / 162.0f); //newhudvictoriuri
	x = NewHudVictoriuriX(NewHudEditorX(HUD_EDITOR_SKILL_POPUP));
	y = NewHudVictoriuriY(NewHudEditorY(HUD_EDITOR_SKILL_POPUP));
	width = NewHudVictoriuriSize(popupWidth); height = NewHudVictoriuriSize(popupHeight);
	boxWidth = NewHudVictoriuriSize(70.0f * popupScale); boxHeight = NewHudVictoriuriSize(80.0f * popupScale); //newhudvictoriuri

	originalX = x + NewHudVictoriuriSize(14.0f * popupScale); //newhudvictoriuri
	if (!NewHudEditorVisible(HUD_EDITOR_SKILL_POPUP))
		goto ENDSKILLRENDER; //newhudvictoriuri

	SEASON3B::RenderImageScale(IMAGE_SKILLBOX_LARGE, x, y, width, height, 0.f, 0.f, 701.f, 163.f);
	SEASON3B::RenderImageScale(IMAGE_SKILL_PAGE, x + NewHudVictoriuriSize(710.0f * popupScale), y + NewHudVictoriuriSize(45.0f * popupScale),
		NewHudVictoriuriSize(36.0f * popupScale), NewHudVictoriuriSize(72.0f * popupScale), 0.f, 0.f, 72.f, 144.f);

	m_SkillListButtonUp.Render();
	m_SkillListButtonDown.Render();

	if (Hero->m_pPet == NULL) goto RENDERSKILL;

	for (int i = AT_PET_COMMAND_DEFAULT; i < AT_PET_COMMAND_END; ++i)
	{
		m_skillList.push_back(i);
	}

RENDERSKILL:
	for (int i = 0; i < MAX_MAGIC; ++i)
	{
		int iSkillType = CharacterAttribute->Skill[i];

		if (iSkillType == 0 || (iSkillType >= AT_SKILL_STUN && iSkillType <= AT_SKILL_REMOVAL_BUFF)) continue;

		BYTE bySkillUseType = SkillAttribute[iSkillType].SkillUseType;

		if (bySkillUseType == SKILL_USE_TYPE_MASTER || bySkillUseType == SKILL_USE_TYPE_MASTERLEVEL) continue;

		m_skillList.push_back(i);
	}

	skillOffset += m_SkillPage * 10;

	for (std::vector<int>::iterator it = m_skillList.begin() + skillOffset; it != m_skillList.end(); ++it)
	{
			float tempX = originalX;

			if (skillCount < 10) {
				tempX += (skillCount * NewHudVictoriuriSize(70.0f * popupScale));
			}
			else {
				tempX += ((skillCount - 10) * NewHudVictoriuriSize(70.0f * popupScale));
			}

			float tempY = y + NewHudVictoriuriSize(12.0f * popupScale) + (skillCount > 9 ? NewHudVictoriuriSize(80.0f * popupScale) : 0.0f); //newhudvictoriuri

			if (*it == Hero->CurrentSkill)
				SEASON3B::RenderImageScale(IMAGE_SKILL_USE, tempX - NewHudVictoriuriSize(6.0f * popupScale), tempY - NewHudVictoriuriSize(6.0f * popupScale), boxWidth, boxHeight, 0.f, 0.f, 69.f, 80.f); //newhudvictoriuri

			RenderSkillIcon(*it, tempX + NewHudVictoriuriSize(10.0f * popupScale), tempY + NewHudVictoriuriSize(6.0f * popupScale), NewHudVictoriuriSize(50.0f * popupScale), NewHudVictoriuriSize(56.0f * popupScale)); //newhudvictoriuri

		skillCount++;
		if (skillCount == 20) break;
	}

ENDSKILLRENDER:
#endif

	if (m_bRenderSkillInfo == true && m_pNewUI3DRenderMng)
	{
		m_pNewUI3DRenderMng->RenderUI2DEffect(INVENTORY_CAMERA_Z_ORDER, UI2DEffectCallback, this, 0, 0);

		m_bRenderSkillInfo = false;
	}

	return true;
}

void SEASON3B::CNewUISkillList::RenderSkillInfo()
{
#if CLIENT_VERSION >= 402 && CLIENT_VERSION <= 502
	::RenderSkillInfo(m_iRenderSkillInfoPosX + 15, m_iRenderSkillInfoPosY - 10, m_iRenderSkillInfoType);
#elif CLIENT_VERSION >= 1001
	::RenderSkillInfo(m_iRenderSkillInfoPosX + 15, m_iRenderSkillInfoPosY - 25, m_iRenderSkillInfoType);
#endif
}

float SEASON3B::CNewUISkillList::GetLayerDepth()
{
	return 2.2f;
}

WORD SEASON3B::CNewUISkillList::GetHeroPriorSkill()
{
	return m_wHeroPriorSkill;
}

void SEASON3B::CNewUISkillList::SetHeroPriorSkill(BYTE bySkill)
{
	m_wHeroPriorSkill = bySkill;
}

// RENDERIZA SKILL DE PET
void SEASON3B::CNewUISkillList::RenderPetSkill()
{
	if (Hero->m_pPet == NULL)
	{
		return;
	}

	float x, y, width, height;

	x = gWideScreen.GetCenterPosition(353); y = gWideScreen.GetBottomPosition(352); width = 32; height = 38;
	for (int i = AT_PET_COMMAND_DEFAULT; i < AT_PET_COMMAND_END; ++i)
	{
		if (i == Hero->CurrentSkill)
		{
			SEASON3B::RenderImage(IMAGE_SKILLBOX_USE, x, y, width, height);
		}
		else
		{
			SEASON3B::RenderImage(IMAGE_SKILLBOX, x, y, width, height);
		}

		RenderSkillIcon(i, x + 6, y + 6, 20, 28);
		x += width;
	}
}

void SEASON3B::CNewUISkillList::RenderSkillIcon(int iIndex, float x, float y, float width, float height)
{
	WORD bySkillType = CharacterAttribute->Skill[iIndex];

	if (bySkillType == 0)
	{
		return;
	}

	if (iIndex >= AT_PET_COMMAND_DEFAULT)
	{
		bySkillType = iIndex;
	}

	bool bCantSkill = false;

	BYTE bySkillUseType = SkillAttribute[bySkillType].SkillUseType;
	int Skill_Icon = SkillAttribute[bySkillType].Magic_Icon;

	if (!gSkillManager.DemendConditionCheckSkill(bySkillType))
	{
		bCantSkill = true;
	}

	if (IsCanBCSkill(bySkillType) == false)
	{
		bCantSkill = true;
	}
	if (g_isCharacterBuff((&Hero->Object), eBuff_AddSkill) && bySkillUseType == SKILL_USE_TYPE_BRAND)
	{
		bCantSkill = true;
	}

	if (bySkillType == AT_SKILL_SPEAR && (Hero->Helper.Type<MODEL_HELPER + 2 || Hero->Helper.Type>MODEL_HELPER + 3) && Hero->Helper.Type != MODEL_HELPER + 37)
	{
		bCantSkill = true;
	}

	if (bySkillType == AT_SKILL_SPEAR && (Hero->Helper.Type == MODEL_HELPER + 2 || Hero->Helper.Type == MODEL_HELPER + 3 || Hero->Helper.Type == MODEL_HELPER + 37))
	{
		int iTypeL = CharacterMachine->Equipment[EQUIPMENT_WEAPON_LEFT].Type;
		int iTypeR = CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT].Type;
		if ((iTypeL < ITEM_SPEAR || iTypeL >= ITEM_BOW) && (iTypeR < ITEM_SPEAR || iTypeR >= ITEM_BOW))
		{
			bCantSkill = true;
		}
	}

	if (bySkillType >= AT_SKILL_BLOCKING && bySkillType <= AT_SKILL_SWORD5 && (Hero->Helper.Type == MODEL_HELPER + 2 || Hero->Helper.Type == MODEL_HELPER + 3 || Hero->Helper.Type == MODEL_HELPER + 37))
	{
		bCantSkill = true;
	}

	if ((bySkillType == AT_SKILL_ICE_BLADE || (AT_SKILL_POWER_SLASH_UP <= bySkillType && AT_SKILL_POWER_SLASH_UP + 4 >= bySkillType)) && (Hero->Helper.Type == MODEL_HELPER + 2 || Hero->Helper.Type == MODEL_HELPER + 3 || Hero->Helper.Type == MODEL_HELPER + 37))
	{
		bCantSkill = true;
	}

	int iEnergy = CharacterAttribute->Energy + CharacterAttribute->AddEnergy;

	if (g_csItemOption.IsDisableSkill(bySkillType, iEnergy))
	{
		bCantSkill = true;
	}

	if (bySkillType == AT_SKILL_PARTY_TELEPORT && PartyNumber <= 0)
	{
		bCantSkill = true;
	}

	if (bySkillType == AT_SKILL_PARTY_TELEPORT && (IsDoppelGanger1() || IsDoppelGanger2() || IsDoppelGanger3() || IsDoppelGanger4()))
	{
		bCantSkill = true;
	}

	if (bySkillType == AT_SKILL_DARK_HORSE || (AT_SKILL_ASHAKE_UP <= bySkillType && bySkillType <= AT_SKILL_ASHAKE_UP + 4))
	{
		BYTE byDarkHorseLife = 0;
		byDarkHorseLife = CharacterMachine->Equipment[EQUIPMENT_HELPER].Durability;
		if (byDarkHorseLife == 0)
		{
			if (Hero->Helper.Type != MODEL_HELPER + 4 && !gHelperSystem.CheckHelperType(Hero->Helper.Type, 16))
			{
				bCantSkill = true;
			}
		}
	}
#ifdef PJH_FIX_SPRIT
	/**/
	if (bySkillType >= AT_PET_COMMAND_DEFAULT && bySkillType < AT_PET_COMMAND_END)
	{
		int iCharisma = CharacterAttribute->Charisma + CharacterAttribute->AddCharisma;
		PET_INFO PetInfo;
		giPetManager::GetPetInfo(PetInfo, 421 - PET_TYPE_DARK_SPIRIT);
		int RequireCharisma = (185 + (PetInfo.m_wLevel * 15));
		if (RequireCharisma > iCharisma)
		{
			bCantSkill = true;
		}
	}
#endif //PJH_FIX_SPRIT
	if ((bySkillType == AT_SKILL_INFINITY_ARROW) || (bySkillType == AT_SKILL_SWELL_OF_MAGICPOWER))
	{
		if (g_csItemOption.IsDisableSkill(bySkillType, iEnergy))
		{
			bCantSkill = true;
		}
		if ((g_isCharacterBuff((&Hero->Object), eBuff_InfinityArrow)) || (g_isCharacterBuff((&Hero->Object), eBuff_SwellOfMagicPower)))
		{
			bCantSkill = true;
		}
	}

	if (bySkillType == AT_SKILL_REDUCEDEFENSE || (AT_SKILL_BLOOD_ATT_UP <= bySkillType && bySkillType <= AT_SKILL_BLOOD_ATT_UP + 4))
	{
		WORD Strength;
		const WORD wRequireStrength = 596;
		Strength = CharacterAttribute->Strength + CharacterAttribute->AddStrength;
		if (Strength < wRequireStrength)
		{
			bCantSkill = true;
		}
		int iTypeL = CharacterMachine->Equipment[EQUIPMENT_WEAPON_LEFT].Type;
		int iTypeR = CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT].Type;

		if (!(iTypeR != -1 && (iTypeR < ITEM_STAFF || iTypeR >= ITEM_STAFF + MAX_ITEM_INDEX) && (iTypeL < ITEM_STAFF || iTypeL >= ITEM_STAFF + MAX_ITEM_INDEX)))
		{
			bCantSkill = true;
		}
	}

	switch (bySkillType)
	{
		//case AT_SKILL_PIERCING:
	case AT_SKILL_PARALYZE:
	{
		WORD  Dexterity;
		const WORD wRequireDexterity = 646;
		Dexterity = CharacterAttribute->Dexterity + CharacterAttribute->AddDexterity;
		if (Dexterity < wRequireDexterity)
		{
			bCantSkill = true;
		}
	}break;
	}

	if (bySkillType == AT_SKILL_WHEEL || (AT_SKILL_TORNADO_SWORDA_UP <= bySkillType && bySkillType <= AT_SKILL_TORNADO_SWORDA_UP + 4) || (AT_SKILL_TORNADO_SWORDB_UP <= bySkillType && bySkillType <= AT_SKILL_TORNADO_SWORDB_UP + 4)
		)
	{
		int iTypeL = CharacterMachine->Equipment[EQUIPMENT_WEAPON_LEFT].Type;
		int iTypeR = CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT].Type;


		if (!(iTypeR != -1 && (iTypeR < ITEM_STAFF || iTypeR >= ITEM_STAFF + MAX_ITEM_INDEX) && (iTypeL < ITEM_STAFF || iTypeL >= ITEM_STAFF + MAX_ITEM_INDEX)))
		{
			bCantSkill = true;
		}
	}

	if (gMapManager.InChaosCastle() == true)
	{
		if (bySkillType == AT_SKILL_DARK_HORSE || bySkillType == AT_SKILL_RIDER || (bySkillType >= AT_PET_COMMAND_DEFAULT && bySkillType <= AT_PET_COMMAND_TARGET) || (AT_SKILL_ASHAKE_UP <= bySkillType && bySkillType <= AT_SKILL_ASHAKE_UP + 4))
		{
			bCantSkill = true;
		}
	}
	else
	{
		if (bySkillType == AT_SKILL_DARK_HORSE || (AT_SKILL_ASHAKE_UP <= bySkillType && bySkillType <= AT_SKILL_ASHAKE_UP + 4))
		{
			BYTE byDarkHorseLife = 0;
			byDarkHorseLife = CharacterMachine->Equipment[EQUIPMENT_HELPER].Durability;
			if (byDarkHorseLife == 0)
			{
				bCantSkill = true;
			}
		}
	}

	int iCharisma = CharacterAttribute->Charisma + CharacterAttribute->AddCharisma;

	if (g_csItemOption.IsDisableSkill(bySkillType, iEnergy, iCharisma))
	{
		bCantSkill = true;
	}

#ifdef PBG_ADD_NEWCHAR_MONK_SKILL
	if (!g_CMonkSystem.IsSwordformGlovesUseSkill(bySkillType))
	{
		bCantSkill = true;
	}
	if (g_CMonkSystem.IsRideNotUseSkill(bySkillType, Hero->Helper.Type))
	{
		bCantSkill = true;
	}

	ITEM* pLeftRing = &CharacterMachine->Equipment[EQUIPMENT_RING_LEFT];
	ITEM* pRightRing = &CharacterMachine->Equipment[EQUIPMENT_RING_RIGHT];

	if (g_CMonkSystem.IsChangeringNotUseSkill(pLeftRing->Type, pRightRing->Type, pLeftRing->Level, pRightRing->Level)
		&& (GetBaseClass(Hero->Class) == CLASS_RAGEFIGHTER))
	{
		bCantSkill = true;
	}
#endif //PBG_ADD_NEWCHAR_MONK_SKILL

	float fU, fV;
	const float srcSkillW = 32.0f; //newhudvictoriuri - V13: celula real do PSD/atlas
	const float srcSkillH = 36.0f; //newhudvictoriuri - V13: celula real do PSD/atlas
	int iKindofSkill = 0;

	if (g_csItemOption.Special_Option_Check() == false && (bySkillType == AT_SKILL_ICE_BLADE || (AT_SKILL_POWER_SLASH_UP <= bySkillType && AT_SKILL_POWER_SLASH_UP + 4 >= bySkillType)))
	{
		bCantSkill = true;
	}


	if (g_csItemOption.Special_Option_Check(1) == false && (bySkillType == AT_SKILL_CROSSBOW || (AT_SKILL_MANY_ARROW_UP <= bySkillType && AT_SKILL_MANY_ARROW_UP + 4 >= bySkillType)))
		bCantSkill = true;

	if (bySkillType >= AT_PET_COMMAND_DEFAULT && bySkillType <= AT_PET_COMMAND_END)
	{
		fU = ((bySkillType - AT_PET_COMMAND_DEFAULT) % 8) * srcSkillW / 256.f;
		fV = ((bySkillType - AT_PET_COMMAND_DEFAULT) / 8) * srcSkillH / 256.f;
		iKindofSkill = KOS_COMMAND;
	}
	else if (bySkillType == AT_SKILL_PLASMA_STORM_FENRIR)
	{
		fU = 4 * srcSkillW / 256.f;
		fV = 0.f;
		iKindofSkill = KOS_COMMAND;
	}
	else if ((bySkillType >= AT_SKILL_ALICE_DRAINLIFE && bySkillType <= AT_SKILL_ALICE_THORNS))
	{
		fU = ((bySkillType - AT_SKILL_ALICE_DRAINLIFE) % 8) * srcSkillW / 256.f;
		fV = 3 * srcSkillH / 256.f;
		iKindofSkill = KOS_SKILL2;
	}
	else if (bySkillType >= AT_SKILL_ALICE_SLEEP && bySkillType <= AT_SKILL_ALICE_BLIND)
	{
		fU = ((bySkillType - AT_SKILL_ALICE_SLEEP + 4) % 8) * srcSkillW / 256.f;
		fV = 3 * srcSkillH / 256.f;
		iKindofSkill = KOS_SKILL2;
	}
	else if (bySkillType == AT_SKILL_ALICE_BERSERKER)
	{
		fU = 10 * srcSkillW / 256.f;
		fV = 3 * srcSkillH / 256.f;
		iKindofSkill = KOS_SKILL2;
	}
	else if (bySkillType >= AT_SKILL_ALICE_WEAKNESS && bySkillType <= AT_SKILL_ALICE_ENERVATION)
	{
		fU = (bySkillType - AT_SKILL_ALICE_WEAKNESS + 8) * srcSkillW / 256.f;
		fV = 3 * srcSkillH / 256.f;
		iKindofSkill = KOS_SKILL2;
	}
	else if (bySkillType >= AT_SKILL_SUMMON_EXPLOSION && bySkillType <= AT_SKILL_SUMMON_REQUIEM)
	{
		fU = ((bySkillType - AT_SKILL_SUMMON_EXPLOSION + 6) % 8) * srcSkillW / 256.f;
		fV = 3 * srcSkillH / 256.f;
		iKindofSkill = KOS_SKILL2;
	}
	else if (bySkillType == AT_SKILL_SUMMON_POLLUTION)
	{
		fU = 11 * srcSkillW / 256.f;
		fV = 3 * srcSkillH / 256.f;
		iKindofSkill = KOS_SKILL2;
	}
	else if (bySkillType == AT_SKILL_BLOW_OF_DESTRUCTION)
	{
		fU = 7 * srcSkillW / 256.f;
		fV = 2 * srcSkillH / 256.f;
		iKindofSkill = KOS_SKILL2;
	}
	else if (bySkillType == AT_SKILL_GAOTIC)
	{
		fU = 3 * srcSkillW / 256.f;
		fV = 8 * srcSkillH / 256.f;
		iKindofSkill = KOS_SKILL2;
	}
	else if (bySkillType == AT_SKILL_RECOVER)
	{
		fU = 9 * srcSkillW / 256.f;
		fV = 2 * srcSkillH / 256.f;
		iKindofSkill = KOS_SKILL2;
	}
	else if (bySkillType == AT_SKILL_MULTI_SHOT)
	{
		if (gCharacterManager.GetEquipedBowType_Skill() == BOWTYPE_NONE)
		{
			bCantSkill = true;
		}

		fU = 0 * srcSkillW / 256.f;
		fV = 8 * srcSkillH / 256.f;
		iKindofSkill = KOS_SKILL2;
	}
	else if (bySkillType == AT_SKILL_FLAME_STRIKE)
	{
		int iTypeL = CharacterMachine->Equipment[EQUIPMENT_WEAPON_LEFT].Type;
		int iTypeR = CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT].Type;

		if (!(iTypeR != -1 && (iTypeR < ITEM_STAFF || iTypeR >= ITEM_STAFF + MAX_ITEM_INDEX) && (iTypeL < ITEM_STAFF || iTypeL >= ITEM_STAFF + MAX_ITEM_INDEX)))
		{
			bCantSkill = true;
		}

		fU = 1 * srcSkillW / 256.f;
		fV = 8 * srcSkillH / 256.f;
		iKindofSkill = KOS_SKILL2;
	}
	else if (bySkillType == AT_SKILL_GIGANTIC_STORM)
	{
		fU = 2 * srcSkillW / 256.f;
		fV = 8 * srcSkillH / 256.f;
		iKindofSkill = KOS_SKILL2;
	}
	else if (bySkillType == AT_SKILL_LIGHTNING_SHOCK)
	{
		fU = 2 * srcSkillW / 256.f;
		fV = 3 * srcSkillH / 256.f;
		iKindofSkill = KOS_SKILL2;
	}
	else if (AT_SKILL_LIGHTNING_SHOCK_UP <= bySkillType && bySkillType <= AT_SKILL_LIGHTNING_SHOCK_UP + 4)
	{
		fU = 6 * srcSkillW / 256.f;
		fV = 8 * srcSkillH / 256.f;
		iKindofSkill = KOS_SKILL2;
	}
	else if (bySkillType == AT_SKILL_SWELL_OF_MAGICPOWER)
	{
		fU = 8 * srcSkillW / 256.f;
		fV = 2 * srcSkillH / 256.f;
		iKindofSkill = KOS_SKILL2;
	}
	else if (bySkillUseType == 4)
	{
		fU = (srcSkillW / 256.f) * (Skill_Icon % 12);
		fV = (srcSkillH / 256.f) * ((Skill_Icon / 12) + 4);
		iKindofSkill = KOS_SKILL2;
	}
#ifdef PBG_ADD_NEWCHAR_MONK_SKILL
	else if (bySkillType >= AT_SKILL_THRUST)
	{
		fU = ((bySkillType - 260) % 12) * srcSkillW / 256.f;
		fV = ((bySkillType - 260) / 12) * srcSkillH / 256.f;
		iKindofSkill = KOS_SKILL3;
	}
#endif //PBG_ADD_NEWCHAR_MONK_SKILL
	else if (bySkillType >= 57)
	{
		fU = ((bySkillType - 57) % 8) * srcSkillW / 256.f;
		fV = ((bySkillType - 57) / 8) * srcSkillH / 256.f;
		iKindofSkill = KOS_SKILL2;
	}
	else
	{
		fU = ((bySkillType - 1) % 8) * srcSkillW / 256.f;
		fV = ((bySkillType - 1) / 8) * srcSkillH / 256.f;
		iKindofSkill = KOS_SKILL1;
	}
	int iSkillIndex = 0;
	switch (iKindofSkill)
	{
	case KOS_COMMAND:
	{
		iSkillIndex = IMAGE_COMMAND;
	}break;
	case KOS_SKILL1:
	{
		iSkillIndex = IMAGE_SKILL1;
	}break;
	case KOS_SKILL2:
	{
		iSkillIndex = IMAGE_SKILL2;
	}break;
	}

	if (iSkillIndex != 0)
	{
		//newhudvictoriuri - o pack MuAway nao possui os antigos atlas
		//IMAGE_NON_SKILL*. Antes eles eram acessados por +5 e viravam caixas
		//brancas. Mantemos o icone real da Elf e apenas o escurecemos quando
		//o requisito da skill ainda nao foi atendido.
		if (bCantSkill)
			glColor4f(0.58f, 0.58f, 0.58f, 1.0f);
		RenderBitmap(iSkillIndex, x, y, width, height, fU, fV, srcSkillW / 256.f, srcSkillH / 256.f); //newhudvictoriuri - V13: UV independente do tamanho renderizado.
		if (bCantSkill)
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}

	int iHotKey = -1;
	for (int i = 0; i < SKILLHOTKEY_COUNT; ++i)
	{
		if (m_iHotKeySkillType[i] == iIndex)
		{
			iHotKey = i;
			break;
		}
	}

	if (iHotKey != -1)
	{
		glColor3f(1.f, 0.9f, 0.8f);
		SEASON3B::RenderNumber(x + 20, y + 20, iHotKey);
	}

#ifdef PBG_ADD_NEWCHAR_MONK_SKILL
	if ((bySkillType == AT_SKILL_GIANTSWING || bySkillType == AT_SKILL_DRAGON_KICK
		|| bySkillType == AT_SKILL_DRAGON_LOWER) && (bCantSkill))
		return;
#endif //PBG_ADD_NEWCHAR_MONK_SKILL

	if ((bySkillType != AT_SKILL_INFINITY_ARROW) && (bySkillType != AT_SKILL_SWELL_OF_MAGICPOWER))

	{
		RenderSkillDelay(iIndex, x, y, width, height);
	}
}

void SEASON3B::CNewUISkillList::RenderSkillDelay(int iIndex, float x, float y, float width, float height)
{
	int iSkillDelay = CharacterAttribute->SkillDelay[iIndex];
	if (iSkillDelay > 0)
	{
		int iSkillType = CharacterAttribute->Skill[iIndex];

		if (iSkillType == AT_SKILL_PLASMA_STORM_FENRIR)
		{
			if (!CheckAttack())
			{
				return;
			}
		}

		int iSkillMaxDelay = SkillAttribute[iSkillType].Delay;

		float fPersent = (float)(iSkillDelay / (float)iSkillMaxDelay);

		EnableAlphaTest();
		glColor4f(1.f, 0.5f, 0.5f, 0.5f);
		float fdeltaH = height * fPersent;
		RenderColor(x, y + height - fdeltaH, width, fdeltaH);
		EndRenderColor();
	}
}

bool SEASON3B::CNewUISkillList::IsSkillListUp()
{
	return m_bHotKeySkillListUp;
}

void SEASON3B::CNewUISkillList::ResetMouseLButton()
{
	MouseLButton = false;
	MouseLButtonPop = false;
	MouseLButtonPush = false;
}

void SEASON3B::CNewUISkillList::UI2DEffectCallback(LPVOID pClass, DWORD dwParamA, DWORD dwParamB)
{
	if (pClass)
	{
		CNewUISkillList* pSkillList = (CNewUISkillList*)(pClass);

#if CLIENT_VERSION >= 402 && CLIENT_VERSION <= 502
		pSkillList->RenderSkillInfo();
#elif CLIENT_VERSION >= 1001
		if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHATINPUTBOX))
			pSkillList->RenderSkillInfo();
#endif
	}
}

void SEASON3B::CNewUIMainFrameWindow::SetPreExp_Wide(__int64 dwPreExp)
{
	m_loPreExp = dwPreExp;
}

void SEASON3B::CNewUIMainFrameWindow::SetGetExp_Wide(__int64 dwGetExp)
{
	m_loGetExp = dwGetExp;

	if (m_loGetExp > 0)
	{
		m_bExpEffect = true;
		m_dwExpEffectTime = timeGetTime();
	}
}

void SEASON3B::CNewUIMainFrameWindow::SetPreExp(DWORD dwPreExp)
{
	m_dwPreExp = dwPreExp;
}

void SEASON3B::CNewUIMainFrameWindow::SetGetExp(DWORD dwGetExp)
{
	m_dwGetExp = dwGetExp;

	if (m_dwGetExp > 0)
	{
		m_bExpEffect = true;
		m_dwExpEffectTime = timeGetTime();
	}
}

void SEASON3B::CNewUIMainFrameWindow::SetBtnState(int iBtnType, bool bStateDown)
{
#if CLIENT_VERSION >= 1001
	//newhudvictoriuri - o estado continua sincronizado com a janela, mas a
	// CCustomButtons usa o icone normal para nao desenhar o quadro V24 quebrado.
	CCustomButtons* pButton = NULL;
	int image = -1;
	int pressedImage = -1;
	if (iBtnType == MAINFRAME_BTN_CHAINFO)
	{
		pButton = &m_BtnChaInfo;
		image = IMAGE_MENU_BTN_CHAINFO;
		pressedImage = IMAGE_MENU_BTN_CHAINFO; //newhudvictoriuri
	}
	else if (iBtnType == MAINFRAME_BTN_MYINVEN)
	{
		pButton = &m_BtnMyInven;
		image = IMAGE_MENU_BTN_MYINVEN;
		pressedImage = IMAGE_MENU_BTN_MYINVEN; //newhudvictoriuri
	}
	else if (iBtnType == MAINFRAME_BTN_PARTY)
	{
		pButton = &m_BtnParty;
		image = IMAGE_MENU_BTN_WINDOW;
		pressedImage = IMAGE_MENU_BTN_WINDOW; //newhudvictoriuri
	}
	if (pButton != NULL)
	{
		const int selectedImage = bStateDown ? pressedImage : image;
		pButton->UnRegisterButtonState();
		pButton->RegisterButtonState(BUTTON_STATE_UP, selectedImage, 0);
		pButton->RegisterButtonState(BUTTON_STATE_OVER, selectedImage, 0);
		pButton->RegisterButtonState(BUTTON_STATE_DOWN, bStateDown ? selectedImage : pressedImage, 0);
		pButton->ChangeImgIndex(selectedImage, 0);
		return;
	}
#endif
	switch (iBtnType)
	{
#ifdef PBG_ADD_INGAMESHOP_UI_MAINFRAME 
#if CLIENT_VERSION >= 302 && CLIENT VERSION <= 502
	case MAINFRAME_BTN_PARTCHARGE:
	{
		if (bStateDown)
		{
			m_BtnCShop.UnRegisterButtonState();
			m_BtnCShop.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_CSHOP, 2);
			m_BtnCShop.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_CSHOP, 3);
			m_BtnCShop.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_CSHOP, 2);
			m_BtnCShop.ChangeImgIndex(IMAGE_MENU_BTN_CSHOP, 2);
		}
		else
		{
			m_BtnCShop.UnRegisterButtonState();
			m_BtnCShop.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_CSHOP, 0);
			m_BtnCShop.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_CSHOP, 1);
			m_BtnCShop.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_CSHOP, 2);
			m_BtnCShop.ChangeImgIndex(IMAGE_MENU_BTN_CSHOP, 0);
		}
	}
	break;
#endif
#endif //defined defined PBG_ADD_INGAMESHOP_UI_MAINFRAME
	case MAINFRAME_BTN_CHAINFO:
	{
		if (bStateDown)
		{
			m_BtnChaInfo.UnRegisterButtonState();
			m_BtnChaInfo.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_CHAINFO, 2);
			m_BtnChaInfo.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_CHAINFO, 3);
			m_BtnChaInfo.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_CHAINFO, 2);
			m_BtnChaInfo.ChangeImgIndex(IMAGE_MENU_BTN_CHAINFO, 2);

		}
		else
		{
			m_BtnChaInfo.UnRegisterButtonState();
			m_BtnChaInfo.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_CHAINFO, 0);
			m_BtnChaInfo.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_CHAINFO, 1);
			m_BtnChaInfo.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_CHAINFO, 2);
			m_BtnChaInfo.ChangeImgIndex(IMAGE_MENU_BTN_CHAINFO, 0);
		}
	}
	break;
	case MAINFRAME_BTN_MYINVEN:
	{
		if (bStateDown)
		{
			m_BtnMyInven.UnRegisterButtonState();
			m_BtnMyInven.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_MYINVEN, 2);
			m_BtnMyInven.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_MYINVEN, 3);
			m_BtnMyInven.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_MYINVEN, 2);
			m_BtnMyInven.ChangeImgIndex(IMAGE_MENU_BTN_MYINVEN, 2);
		}
		else
		{
			m_BtnMyInven.UnRegisterButtonState();
			m_BtnMyInven.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_MYINVEN, 0);
			m_BtnMyInven.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_MYINVEN, 1);
			m_BtnMyInven.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_MYINVEN, 2);
			m_BtnMyInven.ChangeImgIndex(IMAGE_MENU_BTN_MYINVEN, 0);
		}
	}
	break;
#if CLIENT_VERSION <= 502
	case MAINFRAME_BTN_FRIEND:
	{
		if (bStateDown)
		{
			m_BtnFriend.UnRegisterButtonState();
			m_BtnFriend.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_FRIEND, 2);
			m_BtnFriend.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_FRIEND, 3);
			m_BtnFriend.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_FRIEND, 2);
			m_BtnFriend.ChangeImgIndex(IMAGE_MENU_BTN_FRIEND, 2);
		}
		else
		{
			m_BtnFriend.UnRegisterButtonState();
			m_BtnFriend.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_FRIEND, 0);
			m_BtnFriend.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_FRIEND, 1);
			m_BtnFriend.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_FRIEND, 2);
			m_BtnFriend.ChangeImgIndex(IMAGE_MENU_BTN_FRIEND, 0);
		}
	}
	break;
#endif
#if CLIENT_VERSION >= 401 && CLIENT_VERSION <= 502
	case MAINFRAME_BTN_WINDOW:
	{
		if (bStateDown)
		{
			m_BtnWindow.UnRegisterButtonState();
			m_BtnWindow.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_WINDOW, 2);
			m_BtnWindow.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_WINDOW, 3);
			m_BtnWindow.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_WINDOW, 2);
			m_BtnWindow.ChangeImgIndex(IMAGE_MENU_BTN_WINDOW, 2);
		}
		else
		{
			m_BtnWindow.UnRegisterButtonState();
			m_BtnWindow.RegisterButtonState(BUTTON_STATE_UP, IMAGE_MENU_BTN_WINDOW, 0);
			m_BtnWindow.RegisterButtonState(BUTTON_STATE_OVER, IMAGE_MENU_BTN_WINDOW, 1);
			m_BtnWindow.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_MENU_BTN_WINDOW, 2);
			m_BtnWindow.ChangeImgIndex(IMAGE_MENU_BTN_WINDOW, 0);
		}
	}
	break;
#endif
	}
}
