// Ponto de entrada sem janela do GameServer (Linux).
// Replica o WinMain/WndProc de GameServer.cpp: mesmos timers e as mesmas conexoes,
// mas os comandos do menu viram comandos digitados no terminal.
#ifndef _WIN32

#include "stdafx.h"
#include "GameServer.h"
#include "GameMain.h"
#include "JSProtocol.h"
#include "Message.h"
#include "Notice.h"
#include "QueueTimer.h"
#include "ServerDisplayer.h"
#include "ServerInfo.h"
#include "SocketManager.h"
#include "SocketManagerUdp.h"
#include "Util.h"
#include "LuaGameServer.h"
#include "Timer.h"
#include <signal.h>
#include <locale.h>
#include <string>
#include <sstream>

#define WM_CONSOLE_COMMAND (WM_USER+10)

HWND hWnd = (HWND)1; // identificador logico: nao existe janela nesta versao

char LuaDecryptKey[50];
char CustomerName[50];
char CustomerHardwareId[45];
int ClientId;

static std::mutex g_cmdMutex;
static std::deque<std::string> g_cmds;

static void OnSignal(int) { PostQuitMessage(0); }

static void StdinThread()
{
	std::string line;
	while (std::getline(std::cin, line))
	{
		{
			std::lock_guard<std::mutex> l(g_cmdMutex);
			g_cmds.push_back(line);
		}
		PostMessage(hWnd, WM_CONSOLE_COMMAND, 0, 0);
	}
}

static void ScheduleClose(int seconds, int messageId)
{
	if (gCloseMsg == 0)
	{
		gCloseMsg = 1;
		gCloseMsgTime = seconds;
		gNotice.GCNoticeSendToAll(0, 0, 0, 0, 0, 0, gMessage.GetMessage(messageId));
	}
}

static void HandleCommand(const std::string& line)
{
	std::istringstream in(line);
	std::string cmd, arg;
	in >> cmd >> arg;

	if (cmd == "help")
	{
		puts("Comandos: reload <cashshop|chaosmix|character|command|common|custom|event|eventitembag|hack|item|monster|move|quest|shop|skill|util|lua|terrain>");
		puts("          close <1|3|5>   alluserlogout   alluserdisconnect   quit");
	}
	else if (cmd == "quit" || cmd == "exit") PostQuitMessage(0);
	else if (cmd == "alluserlogout") gObjAllLogOut();
	else if (cmd == "alluserdisconnect") gObjAllDisconnect();
	else if (cmd == "close")
	{
		if (arg == "1") ScheduleClose(60, 487);
		else if (arg == "3") ScheduleClose(180, 488);
		else if (arg == "5") ScheduleClose(300, 489);
		else puts("Use: close 1 | close 3 | close 5");
	}
	else if (cmd == "reload")
	{
		if (arg == "cashshop") gServerInfo.ReadCashShopInfo();
		else if (arg == "chaosmix") gServerInfo.ReadChaosMixInfo();
		else if (arg == "character") gServerInfo.ReadCharacterInfo();
		else if (arg == "command") gServerInfo.ReadCommandInfo();
		else if (arg == "common") gServerInfo.ReadCommonInfo();
		else if (arg == "custom") gServerInfo.ReadCustomInfo();
		else if (arg == "event") gServerInfo.ReadEventInfo();
		else if (arg == "eventitembag") gServerInfo.ReadEventItemBagInfo();
		else if (arg == "hack") gServerInfo.ReadHackInfo();
		else if (arg == "item") gServerInfo.ReadItemInfo();
		else if (arg == "monster") gServerInfo.ReloadMonsterInfo();
		else if (arg == "move") gServerInfo.ReadMoveInfo();
		else if (arg == "quest") gServerInfo.ReadQuestInfo();
		else if (arg == "shop") gServerInfo.ReadShopInfo();
		else if (arg == "skill") gServerInfo.ReadSkillInfo();
		else if (arg == "util") gServerInfo.ReadUtilInfo();
		else if (arg == "lua") gLuaGameServer.Reload();
		else if (arg == "terrain") gServerInfo.ReadMapInfo();
		else puts("Reload desconhecido. Digite 'help'.");
	}
	else if (!cmd.empty()) puts("Comando desconhecido. Digite 'help'.");
}

static void DispatchMessage(const MSG& msg)
{
	switch (msg.message)
	{
		case WM_TIMER:
			switch (msg.wParam)
			{
				case WM_TIMER_1000:
					GJServerUserInfoSend();
					ConnectServerInfoSend();
					break;
				case WM_TIMER_2000:
					gObjCountProc();
					gServerDisplayer.Run();
					break;
				case WM_TIMER_10000:
					JoinServerReconnect(hWnd, WM_JOIN_SERVER_MSG_PROC);
					DataServerReconnect(hWnd, WM_DATA_SERVER_MSG_PROC);
					break;
				default:
					break;
			}
			break;
		case WM_JOIN_SERVER_MSG_PROC:
			JoinServerMsgProc(msg.wParam, msg.lParam);
			break;
		case WM_DATA_SERVER_MSG_PROC:
			DataServerMsgProc(msg.wParam, msg.lParam);
			break;
		case WM_CONSOLE_COMMAND:
		{
			std::string line;
			{
				std::lock_guard<std::mutex> l(g_cmdMutex);
				if (g_cmds.empty()) break;
				line = g_cmds.front();
				g_cmds.pop_front();
			}
			HandleCommand(line);
			break;
		}
		default:
			break;
	}
}

int main(int, char**)
{
	signal(SIGINT, OnSignal);
	signal(SIGTERM, OnSignal);

	setlocale(LC_ALL, "pt_BR.UTF-8");

	SetLargeRand();

	gServerInfo.ReadStartupInfo("GameServerInfo", ".\\Data\\GameServerInfo - Common.dat");

	gServerDisplayer.Init(hWnd);

	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) == 0)
	{
		if (gSocketManager.Start((WORD)gServerInfo.m_ServerPort) == 0)
		{
			LogAdd(LOG_RED, "Could not start GameServer");
		}
		else
		{
			GameMainInit(hWnd);

			JoinServerConnect(WM_JOIN_SERVER_MSG_PROC);

			DataServerConnect(WM_DATA_SERVER_MSG_PROC);

			gSocketManagerUdp.Connect(gServerInfo.m_ConnectServerAddress, (WORD)gServerInfo.m_ConnectServerPort);

			SetTimer(hWnd, WM_TIMER_1000, 1000, 0);
			SetTimer(hWnd, WM_TIMER_10000, 10000, 0);

			gTimer.start();

			gQueueTimer.CreateTimer(QUEUE_TIMER_MONSTER, 100, &QueueTimerCallback);
			gQueueTimer.CreateTimer(QUEUE_TIMER_MONSTER_MOVE, 100, &QueueTimerCallback);
			gQueueTimer.CreateTimer(QUEUE_TIMER_MONSTER_AI, 100, &QueueTimerCallback);
			gQueueTimer.CreateTimer(QUEUE_TIMER_MONSTER_AI_MOVE, 100, &QueueTimerCallback);
			gQueueTimer.CreateTimer(QUEUE_TIMER_EVENT, 100, &QueueTimerCallback);
			gQueueTimer.CreateTimer(QUEUE_TIMER_VIEWPORT, 1000, &QueueTimerCallback);
			gQueueTimer.CreateTimer(QUEUE_TIMER_FIRST, 1000, &QueueTimerCallback);
			gQueueTimer.CreateTimer(QUEUE_TIMER_CLOSE, 1000, &QueueTimerCallback);
			gQueueTimer.CreateTimer(QUEUE_TIMER_MATH_AUTHENTICATOR, 10000, &QueueTimerCallback);
			gQueueTimer.CreateTimer(QUEUE_TIMER_ACCOUNT_LEVEL, 60000, &QueueTimerCallback);
			gQueueTimer.CreateTimer(QUEUE_TIMER_UPDATE_PARTY_BUFF, 100, &QueueTimerCallback);
		}
	}
	else
	{
		LogAdd(LOG_RED, "WSAStartup() failed with error: %d", WSAGetLastError());
	}

	SetTimer(hWnd, WM_TIMER_2000, 2000, 0);

	std::thread(StdinThread).detach();

	LogAdd(LOG_GREEN, "GameServer em execucao. Digite 'help' para ver os comandos.");

	MSG msg;

	while (GetMessage(&msg, 0, 0, 0) != 0)
	{
		DispatchMessage(msg);
	}

	return (int)msg.wParam;
}

#endif // !_WIN32
