// NewUINameWindow.cpp: implementation of the CNewUINameWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NewUINameWindow.h"
#include "ZzzBmd.h"
#include "ZzzObject.h"
#include "ZzzCharacter.h"
#include "ZzzInterface.h"
#include "ZzzInventory.h"
#include "UIControls.h"
#include "CSChaosCastle.h"
#include "PersonalShopTitleImp.h"
#include "MatchEvent.h"
#include "MapManager.h"
#include "WideScreen.h"
#include "NewUISystem.h"

using namespace SEASON3B;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SEASON3B::CNewUINameWindow::CNewUINameWindow()
{
	m_pNewUIMng = NULL;
	m_Pos.x = m_Pos.y = 0;

	m_bShowItemName = false;
}

SEASON3B::CNewUINameWindow::~CNewUINameWindow()
{
	Release();
}

bool SEASON3B::CNewUINameWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
{
	if (NULL == pNewUIMng)
		return false;

	m_pNewUIMng = pNewUIMng;
	m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_NAME_WINDOW, this);

	SetPos(x, y);

	Show(true);

	return true;
}

void SEASON3B::CNewUINameWindow::Release()
{
	if(m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj( this );
		m_pNewUIMng = NULL;
	}
}
		
void SEASON3B::CNewUINameWindow::SetPos(int x, int y)
{
	m_Pos.x = x;
	m_Pos.y = y;
}

bool SEASON3B::CNewUINameWindow::UpdateMouseEvent()
{
	return true;
}

bool SEASON3B::CNewUINameWindow::UpdateKeyEvent()
{
	if(SEASON3B::IsPress(VK_MENU) == true)
	{
		m_bShowItemName = !m_bShowItemName;
	}

	return true;
}

bool SEASON3B::CNewUINameWindow::Update()
{
	return true;
}

bool SEASON3B::CNewUINameWindow::Render()
{
	EnableAlphaTest();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	RenderName();
    RenderTimes();
    matchEvent::RenderMatchTimes ();
    RenderBooleans();
	DrawPersonalShopTitleImp();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	g_pNewUISystem->m_Lua.Generic_Call("BeforeMainProc", ">");
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	DisableAlphaBlend();
	return true;
}

void SEASON3B::CNewUINameWindow::RenderName()
{
     //Sempre renderizar nomes dos personagens
    for (int i = 0; i < MAX_CHARACTERS_CLIENT; i++)
    {
        CHARACTER* c = &CharactersClient[i];
        OBJECT* o = &c->Object;
        if (o->Live && o->Kind == KIND_PLAYER)
        {
            if (IsShopTitleVisible(c) == false)
            {
                CreateChat(c->ID, "", c); // Renderiza o nome do personagem
            }
        }
    }

     //Mantém a lógica existente para outras funcionalidades (NPCs, itens, etc.)
    if (g_bGMObservation == true)
    {
        for (int i = 0; i < MAX_CHARACTERS_CLIENT; i++)
        {
            CHARACTER* c = &CharactersClient[i];
            OBJECT* o = &c->Object;
            if (o->Live && o->Kind == KIND_PLAYER)
            {
                if (IsShopTitleVisible(c) == false)
                {
                    CreateChat(c->ID, "", c);
                }
            }
        }
    }

#ifndef GUILD_WAR_EVENT
    if (gMapManager.InChaosCastle() == true)
    {
        if (FindText(Hero->ID, "webzen") == false)
        {
            if (SelectedNpc != -1 || SelectedCharacter != -1)
            {
                return;
            }
        }
    }
#endif // GUILD_WAR_EVENT

    if (SelectedItem != -1 || SelectedNpc != -1 || SelectedCharacter != -1)
    {
        if (SelectedNpc != -1)
        {
            CHARACTER* c = &CharactersClient[SelectedNpc];
            OBJECT* o = &c->Object;
            CreateChat(c->ID, "", c);
        }
        else if (SelectedCharacter != -1)
        {
            CHARACTER* c = &CharactersClient[SelectedCharacter];
            OBJECT* o = &c->Object;
            if (o->Kind == KIND_MONSTER)
            {
               //g_pRenderText->SetTextColor(255, 230, 200, 255);
               //g_pRenderText->SetBgColor(100, 0, 0, 255);
               //g_pRenderText->RenderText(setPosCenterX(0), 2, c->ID, 0, 0, RT3_WRITE_CENTER);
            }
            else
#ifdef ASG_ADD_GENS_SYSTEM
#ifndef PBG_MOD_STRIFE_GENSMARKRENDER
                if (!::IsStrifeMap(World) || Hero->m_byGensInfluence == c->m_byGensInfluence)
#endif // PBG_MOD_STRIFE_GENSMARKRENDER
#endif // ASG_ADD_GENS_SYSTEM
                {
                    if (IsShopTitleVisible(c) == false)
                    {
                        CreateChat(c->ID, "", c);
                    }
                }
        }
        else if (SelectedItem != -1)
        {
            RenderItemName(SelectedItem, &Items[SelectedItem].Object, Items[SelectedItem].Item.Level, Items[SelectedItem].Item.Option1, Items[SelectedItem].Item.ExtOption, false);
        }
    }

    if (m_bShowItemName || SEASON3B::IsRepeat(VK_MENU))
    {
        for (int i = 0; i < MAX_ITEMS; i++)
        {
            OBJECT* o = &Items[i].Object;
            if (o->Live)
            {
                if (o->Visible && i != SelectedItem)
                {
                    RenderItemName(i, o, Items[i].Item.Level, Items[i].Item.Option1, Items[i].Item.ExtOption, true);
                }
            }
        }
    }
}

float SEASON3B::CNewUINameWindow::GetLayerDepth()
{
	return 1.0f;
}


void RenderFlameEdgesAdvanced(float iPos_x, float fsy, float fWidth, float fHeight, SIZE TextSize)
{
	extern double WorldTime;

	float lineH = (float)TextSize.cy / g_fScreenRate_y;
	float flareH = lineH * 0.9f;
	float flareW = lineH * 0.9f;

	float perimeter = 2.0f * (fWidth + fHeight);
	float travelSpeed = 0.2f;

	float head = fmod(WorldTime * travelSpeed, perimeter);
	int snakeLength = 12;
	float spacing = flareW * 3.5f;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	for (int s = 0; s < snakeLength; s++)
	{
		float pos = fmod(head - s * spacing + perimeter, perimeter);
		float fx = iPos_x, fy = fsy;
		float w = flareW, h = flareH;

		// Determinar el lado
		if (pos < fWidth)
		{
			fx = iPos_x + pos; fy = fsy; w = flareW * 5.0f;
		}
		else if (pos < fWidth + fHeight) { fx = iPos_x + fWidth; fy = fsy + (pos - fWidth); h = flareH * 5.0f; }
		else if (pos < fWidth + fHeight + fWidth) { fx = iPos_x + (fWidth - (pos - (fWidth + fHeight))); fy = fsy + fHeight; w = flareW * 5.0f; }
		else { fx = iPos_x; fy = fsy + (fHeight - (pos - (fWidth + fHeight + fWidth))); h = flareH * 5.0f; }

		float alpha = 1.0f - (float)s / (float)snakeLength;

		// Capa 1: rojo-anaranjado
		glColor4f(1.0f, 0.3f, 0.0f, alpha * 0.8f);
		RenderBitmap(BITMAP_FLARE, fx - w * 0.5f, fy - h * 0.5f, w, h, 0.f, 0.f, 1.f, 1.f);

		// Capa 2: amarillo-blanco encima
		glColor4f(1.0f, 0.9f, 0.3f, alpha * 0.6f);
		RenderBitmap(BITMAP_FLARE, fx - w * 0.3f, fy - h * 0.3f, w * 0.6f, h * 0.6f, 0.f, 0.f, 1.f, 1.f);

		// Chispa random
		float flick = 0.5f + 0.5f * sinf(WorldTime * 20.0f + s * 2.0f);
		if (flick > 0.8f)
		{
			glColor4f(1.0f, 1.0f, 0.6f, flick);
			RenderBitmap(BITMAP_FLARE, fx + sinf(s) * 5.0f, fy + cosf(s) * 5.0f, flareW * 0.7f, flareH * 0.7f, 0.f, 0.f, 1.f, 1.f);
		}
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
	glColor4f(1, 1, 1, 1);
}


void RenderFlameEdges(float iPos_x, float fsy, float fWidth, float fHeight, SIZE TextSize, float r, float g, float b)
{

	extern double WorldTime;

	// Tamaño base
	float lineH = (float)TextSize.cy / g_fScreenRate_y;
	float flareH = lineH * 0.9f;
	float flareW = lineH * 0.9f;

	// Perimetro total
	float perimeter = 2.0f * (fWidth + fHeight);

	// Velocidad
	float travelSpeed = 0.2f;

	// Posicion de la cabeza de la linea
	float head = fmod(WorldTime * travelSpeed, perimeter);

	// Cantidad de segmentos que forman la "linea"
	int snakeLength = 10;
	float spacing = (flareW * 3.0f); // mas grande para que se solapen y se vea continuo

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	for (int s = 0; s < snakeLength; s++)
	{
		float pos = fmod(head - s * spacing + perimeter, perimeter);

		float fx = iPos_x;
		float fy = fsy;
		float w = flareW, h = flareH;

		// Decidir en que lado del rectangulo esta
		if (pos < fWidth) {
			// arriba
			fx = iPos_x + pos;
			fy = fsy;
			w = flareW * 5.0f; // linea horizontal
			h = flareH;
		}
		else if (pos < fWidth + fHeight)
		{
			// derecha
			fx = iPos_x + fWidth;
			fy = fsy + (pos - fWidth);
			w = flareW;
			h = flareH * 5.0f; // linea vertical
		}
		else if (pos < fWidth + fHeight + fWidth)
		{
			// abajo
			fx = iPos_x + (fWidth - (pos - (fWidth + fHeight)));
			fy = fsy + fHeight;
			w = flareW * 5.0f;
			h = flareH;
		}
		else
		{
			// izquierda
			fx = iPos_x;
			fy = fsy + (fHeight - (pos - (fWidth + fHeight + fWidth)));
			w = flareW;
			h = flareH * 5.0f;
		}

		// Alpha en degradado
		float alpha = 1.0f - (float)s / (float)snakeLength;

		glColor4f(r, g, b, alpha);
		RenderBitmap(BITMAP_FLARE, fx - w * 0.5f, fy - h * 0.5f, w, h, 0.f, 0.f, 1.f, 1.f);

	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	DisableAlphaBlend();
}

