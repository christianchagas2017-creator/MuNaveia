#include "stdafx.h"
#include "HealthBar.h"
#include "ZzzOpenglUtil.h"
#include "ZzzCharacter.h"
#include "UIControls.h"
#include "WideScreen.h"

CHealthBar gHealthBar;

CHealthBar::CHealthBar()
{
	ClearNewHealthBar();
}

CHealthBar::~CHealthBar()
{

}

void CHealthBar::ClearNewHealthBar() // OK
{
	for (int n = 0; n < MAX_MAIN_VIEWPORT; n++)
	{
		gNewHealthBar[n].index = 0xFFFF;
		gNewHealthBar[n].type = 0;
		gNewHealthBar[n].rate = 0;
	}
}

void CHealthBar::InsertNewHealthBar(WORD index, BYTE type, BYTE rate) // OK
{
	for (int n = 0; n < MAX_MAIN_VIEWPORT; n++)
	{
		if (gNewHealthBar[n].index == 0xFFFF)
		{
			gNewHealthBar[n].index = index;
			gNewHealthBar[n].type = type;
			gNewHealthBar[n].rate = rate;
			return;
		}
	}
}

NEW_HEALTH_BAR* CHealthBar::GetNewHealthBar(WORD index, BYTE type) // OK
{
	for (int n = 0; n < MAX_MAIN_VIEWPORT; n++)
	{
		if (gNewHealthBar[n].index != 0xFFFF)
		{
			if (gNewHealthBar[n].index == index && gNewHealthBar[n].type == type)
			{
				return &gNewHealthBar[n];
			}
		}
	}

	return 0;
}

void CHealthBar::DrawNewHealthBar() // OK
{
    float   Width = 38.f;
    char    Text[100];


    //for (int j = 0; j < MAX_CHARACTERS_CLIENT; ++j)
    if(SelectedCharacter != -1)
    {
        CHARACTER* c = &CharactersClient[SelectedCharacter];
        OBJECT* o = &c->Object;
       //vec3_t      Position;
       //int         ScreenX, ScreenY;


        if (KIND_MONSTER == c->Object.Kind && MODEL_PLAYER != c->Object.Type && c->Object.Kind != KIND_TRAP)
        {
            BYTE LifePercent = 80;



            //Vector(o->Position[0], o->Position[1], o->Position[2] + o->BoundingBoxMax[2] + 100.f, Position);
            //Projection(Position, &ScreenX, &ScreenY);
            //ScreenX -= (int)(Width / 2);
            if (o->Live && KIND_NPC != c->Object.Kind)
            {
                NEW_HEALTH_BAR* lpNewHealthBar = gHealthBar.GetNewHealthBar(c->Key, c->Object.Kind);

                if (lpNewHealthBar == NULL) {
                    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                    DisableAlphaBlend();
                    glColor3f(1.f, 1.f, 1.f);
                    return;
                }

                float posX = gWideScreen.g_WideWindowWidth / 2 - 75;
                float posY = 15;

                int LifePercent = lpNewHealthBar->rate;
                float iHP = (float)(LifePercent * 133.5) / 100;
                SEASON3B::RenderImageScale(BITMAP_NEW_HEALTHBAR_BACK, posX, posY, 150, 15.f, 0.f, 0.f, 300.f, 30.f);
                if (c->Dead != 0)
                {
                    iHP -= c->Dead;
                    if (iHP <= 0)
                    {
                        iHP = 0;
                    }
                }
                SEASON3B::RenderImageScale(BITMAP_NEW_HEALTHBAR, posX+8.f, posY+3.f, iHP, 9.f, 0.f, 0.f, 267.f, 18.f);

                g_pRenderText->SetFont(g_hFontBold);
                if (c->Dead != 0)
                {
                    sprintf(Text, "%s die...", c->ID);
                }
                else
                {
                    sprintf(Text, "%s", c->ID);
                }
                g_pRenderText->SetTextColor(255, 230, 210, 255);
                g_pRenderText->SetBgColor(0,0,0,0);
                g_pRenderText->RenderText(posX, posY+4.f, Text, 150.f, 0, RT3_SORT_CENTER);


            }
        }
    }
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    DisableAlphaBlend();
    glColor3f(1.f, 1.f, 1.f);
}