// Versao sem janela do ServerDisplayer: escreve o log no terminal.
// No Windows continua valendo o ServerDisplayer.cpp original (interface grafica).
#ifndef _WIN32

#include "stdafx.h"
#include "ServerDisplayer.h"
#include "ServerInfo.h"
#include "User.h"

CServerDisplayer gServerDisplayer;

static std::mutex g_logMutex;

static const char* AnsiColor(eLogColor color)
{
	switch (color)
	{
		case LOG_RED: case LOG_DARK_RED: return "\033[31m";
		case LOG_GREEN: case LOG_DARK_GREEN: return "\033[32m";
		case LOG_BLUE: case LOG_DARK_BLUE: return "\033[34m";
		case LOG_CYAN: return "\033[36m";
		case LOG_MAGENTA: case LOG_PURPLE: return "\033[35m";
		case LOG_ORANGE: case LOG_YELLOW: case LOG_BROWN: return "\033[33m";
		case LOG_GRAY: return "\033[90m";
		default: return "\033[0m";
	}
}

CServerDisplayer::CServerDisplayer()
{
	this->m_hwnd = 0;
	this->m_count = 0;
	memset(this->m_font, 0, sizeof(this->m_font));
	memset(this->m_brush, 0, sizeof(this->m_brush));
	memset(this->m_log, 0, sizeof(this->m_log));
	memset(this->m_DisplayerText, 0, sizeof(this->m_DisplayerText));
}

CServerDisplayer::~CServerDisplayer() {}

void CServerDisplayer::Init(HWND hWnd) { this->m_hwnd = hWnd; }

void CServerDisplayer::Run()
{
	// Chamado a cada 2 segundos; imprime um resumo a cada minuto.
	if ((++this->m_count % 30) == 0)
	{
		this->SetWindowName();
	}
}

void CServerDisplayer::SetWindowName()
{
	char text[256];
	snprintf(text, sizeof(text), "[%s] %s (PlayerCount : %d/%d) (MonsterCount : %d/%d)", GAMESERVER_VERSION, gServerInfo.m_ServerName, gObjTotalUser, gServerInfo.m_ServerMaxUserNumber, gObjTotalMonster, MAX_OBJECT_MONSTER);
	this->LogAddText(LOG_GRAY, text, (int)strlen(text));
}

void CServerDisplayer::PaintAllInfo() {}
void CServerDisplayer::PaintInfos() {}
void CServerDisplayer::LogTextPaint() {}

void CServerDisplayer::LogAddText(eLogColor color, char* text, int size)
{
	std::lock_guard<std::mutex> lock(g_logMutex);
	fprintf(stdout, "%s%.*s\033[0m\n", AnsiColor(color), size, text);
	fflush(stdout);
}

#endif // !_WIN32
