// ServerDisplayer.cpp: implementation of the CServerDisplayer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ServerDisplayer.h"
#include "GameMain.h"
#include "Log.h"
#include "ServerInfo.h"
#include "SocketManager.h"
#include "User.h"
#include "LuaManager.h"
#include "QueryAsync.h"

CServerDisplayer gServerDisplayer;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CServerDisplayer::CServerDisplayer() // OK
{
	for(int n=0;n < MAX_LOG_TEXT_LINE;n++)
	{
		memset(&this->m_log[n],0,sizeof(this->m_log[n]));
	}

	this->m_font[0] = CreateFont(50, 0, 0, 0, FW_THIN, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Times");
	this->m_font[1] = CreateFont(18, 0, 0, 0, FW_THIN, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Times");
	this->m_brush[0] = CreateSolidBrush(RGB(105, 105, 105));
	this->m_brush[1] = CreateSolidBrush(RGB(59, 217, 59));
	this->m_brush[2] = CreateSolidBrush(RGB(50, 50, 50));

	strcpy_s(this->m_DisplayerText[0],"STANDBY MODE");
	strcpy_s(this->m_DisplayerText[1],"ACTIVE MODE");
}

CServerDisplayer::~CServerDisplayer() // OK
{
	DeleteObject(this->m_font[0]);
	DeleteObject(this->m_font[1]);
	DeleteObject(this->m_brush[0]);
	DeleteObject(this->m_brush[1]);
	DeleteObject(this->m_brush[2]);
}

void CServerDisplayer::Init(HWND hWnd) // OK
{
	this->m_hwnd = hWnd;

	CreateDirectory("LOGS", 0);

	gLog.AddLog(1, "LOGS\\LOG_GAMESERVER");

	gLog.AddLog(gServerInfo.m_WriteChatLog,"LOGS\\CHAT_LOG");

	gLog.AddLog(gServerInfo.m_WriteCommandLog,"LOGS\\COMMAND_LOG");

	gLog.AddLog(gServerInfo.m_WriteTradeLog,"LOGS\\TRADE_LOG");

	gLog.AddLog(gServerInfo.m_WriteConnectLog,"LOGS\\CONNECT_LOG");

	gLog.AddLog(gServerInfo.m_WriteHackLog,"LOGS\\HACK_LOG");

	gLog.AddLog(gServerInfo.m_WriteCashShopLog,"LOGS\\CASH_SHOP_LOG");

	gLog.AddLog(gServerInfo.m_WriteChaosMixLog,"LOGS\\CHAOS_MIX_LOG");

	gLog.AddLog(1,"LOGS\\LUA_LOG");

	gLog.AddLog(1,"LOGS\\EVENT_LOG");

	gLog.AddLog(1, "LOGS\\TRADEX_LOG");
}

void CServerDisplayer::Run() // OK
{
	this->SetWindowName();
	this->PaintAllInfo();
	this->LogTextPaint();
	this->PaintInfos();
}

void CServerDisplayer::SetWindowName() // OK
{
	char buff[256];

	wsprintf(buff,"[%s] %s (PlayerCount : %d/%d) (MonsterCount : %d/%d)",GAMESERVER_VERSION,gServerInfo.m_ServerName,gObjTotalUser,gServerInfo.m_ServerMaxUserNumber,gObjTotalMonster,MAX_OBJECT_MONSTER);

	SetWindowText(this->m_hwnd,buff);
}

void CServerDisplayer::PaintAllInfo() // OK
{
	RECT rect;

	GetClientRect(this->m_hwnd, &rect);

	rect.top = 0;
	rect.bottom = 80;

	HDC hdc = GetDC(this->m_hwnd);

	int OldBkMode = SetBkMode(hdc, TRANSPARENT);

	HFONT OldFont = (HFONT)SelectObject(hdc, this->m_font[0]);

	if (gJoinServerConnection.CheckState() == 0 || gDataServerConnection.CheckState() == 0)
	{
		SetTextColor(hdc, RGB(200, 200, 200));
		FillRect(hdc, &rect, this->m_brush[0]);
		TextOut(hdc, 120, 20, this->m_DisplayerText[0], strlen(this->m_DisplayerText[0]));
	}
	else
	{
		gGameServerDisconnect = 0;
		SetTextColor(hdc, RGB(250, 250, 250));
		FillRect(hdc, &rect, this->m_brush[1]);
		TextOut(hdc, 120, 20, this->m_DisplayerText[1], strlen(this->m_DisplayerText[1]));
	}

	SelectObject(hdc, OldFont);
	SetBkMode(hdc, OldBkMode);
	ReleaseDC(this->m_hwnd, hdc);
}

static const char* ConvertMemory(uint64_t bytes)
{
	char* suffix[] = { "B", "KB", "MB", "GB", "TB" };
	char length = sizeof(suffix) / sizeof(suffix[0]);
	int i = 0;
	double dblBytes = (double)bytes;
	if (bytes > 1024) {
		for (i = 0; (bytes / 1024) > 0 && i < length - 1; i++, bytes /= 1024) {
			dblBytes = bytes / 1024.0;
		}
	}
	static char output[200];
	sprintf_s(output, "%.02lf %s", dblBytes, suffix[i]);
	return output;
}

void CServerDisplayer::PaintInfos()
{
	RECT rect;

	GetClientRect(this->m_hwnd, &rect);

	rect.top = 520;

	rect.bottom = 545;

	HDC hdc = GetDC(this->m_hwnd);

	int OldBkMode = SetBkMode(hdc, TRANSPARENT);

	HFONT OldFont = (HFONT)SelectObject(hdc, this->m_font[1]);

	SetTextColor(hdc, RGB(255, 255, 255));

	FillRect(hdc, &rect, this->m_brush[2]);

	char buffer[256];

	sprintf_s(buffer, "Lua Memory: %s | Async Queue: %d", ConvertMemory(LuaManager.getMemoryUsed()), gQueryAsync.getSize());

	TextOut(hdc, 0, 523, buffer, strlen(buffer));

	SelectObject(hdc, OldFont);
	SetBkMode(hdc, OldBkMode);
	ReleaseDC(this->m_hwnd, hdc);
}


void CServerDisplayer::LogTextPaint() // OK
{
	RECT rect;

	GetClientRect(this->m_hwnd, &rect);

	rect.top = 80;

	HDC hdc = GetDC(this->m_hwnd);

	FillRect(hdc, &rect, (HBRUSH)GetStockObject(0));

	int line = MAX_LOG_TEXT_LINE;

	int count = (((this->m_count - 1) >= 0) ? (this->m_count - 1) : (MAX_LOG_TEXT_LINE - 1));

	for (int n = 0; n < MAX_LOG_TEXT_LINE; n++)
	{
		switch(this->m_log[count].color)
		{
			case LOG_BLACK:
				SetTextColor(hdc,RGB(0,0,0));
				break;
			case LOG_RED:
				SetTextColor(hdc,RGB(255,0,0));
				break;
			case LOG_GREEN:
				SetTextColor(hdc,RGB(0,255,0));
				break;
			case LOG_BLUE:
				SetTextColor(hdc,RGB(0,0,255));
				break;
			case LOG_CYAN:
				SetTextColor(hdc, RGB(0, 230, 218));
				break; 
			case LOG_DARK_GREEN:
				SetTextColor(hdc, RGB(0, 159, 0));
				break;
			case LOG_DARK_RED:
				SetTextColor(hdc, RGB(190, 0, 0));
				break;
			case LOG_MAGENTA:
				SetTextColor(hdc, RGB(255, 0, 255));
				break;
			case LOG_PURPLE:
				SetTextColor(hdc, RGB(155, 0, 255));
				break;
			case LOG_DARK_BLUE:
				SetTextColor(hdc, RGB(0, 0, 170));
				break;
			case LOG_ORANGE:
				SetTextColor(hdc, RGB(255, 98, 0));
				break;
			case LOG_YELLOW:
				SetTextColor(hdc, RGB(255, 208, 0));
				break;
			case LOG_BROWN:
				SetTextColor(hdc, RGB(120, 57, 6));
				break;
			case LOG_GRAY:
				SetTextColor(hdc, RGB(100, 100, 100));
				break;
			default:
				SetTextColor(hdc, RGB(0, 0, 0));
				break;
		}

		int size = strlen(this->m_log[count].text);

		if (size > 1)
		{
			TextOut(hdc, 0, (65 + (line * 15)), this->m_log[count].text, size);
			line--;
		}

		count = (((--count) >= 0) ? count : (MAX_LOG_TEXT_LINE - 1));
	}

	ReleaseDC(this->m_hwnd, hdc);
}

void CServerDisplayer::LogAddText(eLogColor color,char* text,int size) // OK
{
	size = ((size>=MAX_LOG_TEXT_SIZE)?(MAX_LOG_TEXT_SIZE-1):size);

	memset(&this->m_log[this->m_count].text,0,sizeof(this->m_log[this->m_count].text));

	memcpy(&this->m_log[this->m_count].text,text,size);

	this->m_log[this->m_count].color = color;

	this->m_count = (((++this->m_count)>=MAX_LOG_TEXT_LINE)?0:this->m_count);

	gLog.Output(LOG_GENERAL,"%s",&text[9]);
}
