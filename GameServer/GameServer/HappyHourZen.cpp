#include "stdafx.h"
#include "HappyHourZen.h"
#include "MemScript.h"
#include "Notice.h"
#include "ScheduleManager.h"
#include "Util.h"

#ifdef FAIRPLAY

HappyHourZen gHappyHourZen;

HappyHourZen::HappyHourZen()
{
	auto lpInfo = &this->m_HappyHourZenInfo;

	lpInfo->State = HAPPY_HOUR_ZEN_STATE_BLANK;
	lpInfo->RemainTime = 0;
	lpInfo->TargetTime = 0;
	lpInfo->TickCount = GetTickCount();
	lpInfo->AlarmMinSave = -1;
	lpInfo->AlarmMinLeft = -1;
}

HappyHourZen::~HappyHourZen()
{
}

void HappyHourZen::Init()
{
	this->SetState(&this->m_HappyHourZenInfo, HAPPY_HOUR_ZEN_STATE_EMPTY);
}

void HappyHourZen::Load(char* path)
{
	CMemScript* lpMemScript = new CMemScript;

	if (lpMemScript == 0)
	{
		ErrorMessageBox(MEM_SCRIPT_ALLOC_ERROR, path);
		return;
	}

	if (lpMemScript->SetBuffer(path) == 0)
	{
		ErrorMessageBox(lpMemScript->GetLastError());
		delete lpMemScript;
		return;
	}

	this->m_HappyHourZenInfo.StartTime.clear();

	try
	{
		while (true)
		{
			if (lpMemScript->GetToken() == TOKEN_END)
			{
				break;
			}

			int section = lpMemScript->GetNumber();

			while (true)
			{
				if (section == 0)
				{
					if (strcmp("end", lpMemScript->GetAsString()) == 0)
					{
						break;
					}

					HAPPY_HOUR_ZEN_START_TIME info;

					info.Year = lpMemScript->GetNumber();

					info.Month = lpMemScript->GetAsNumber();

					info.Day = lpMemScript->GetAsNumber();

					info.DayOfWeek = lpMemScript->GetAsNumber();

					info.Hour = lpMemScript->GetAsNumber();

					info.Minute = lpMemScript->GetAsNumber();

					info.Second = lpMemScript->GetAsNumber();

					this->m_HappyHourZenInfo.StartTime.push_back(info);
				}
				else if (section == 1)
				{
					if (strcmp("end", lpMemScript->GetAsString()) == 0)
					{
						break;
					}

					this->m_HappyHourZenInfo.AlarmTime = lpMemScript->GetNumber();

					this->m_HappyHourZenInfo.EventTime = lpMemScript->GetAsNumber();

					this->m_HappyHourZenInfo.RateZen = lpMemScript->GetAsNumber();
				}
				else
				{
					break;
				}
			}
		}
	}
	catch (...)
	{
		ErrorMessageBox(lpMemScript->GetLastError());
	}

	delete lpMemScript;
}

void HappyHourZen::MainProc()
{
	auto lpInfo = &this->m_HappyHourZenInfo;

	if ((GetTickCount() - lpInfo->TickCount) >= 1000)
	{
		lpInfo->TickCount = GetTickCount();

		lpInfo->RemainTime = (int)difftime(lpInfo->TargetTime, time(0));

		switch (lpInfo->State)
		{
		case HAPPY_HOUR_ZEN_STATE_BLANK:
			this->ProcState_BLANK(lpInfo);
			break;
		case HAPPY_HOUR_ZEN_STATE_EMPTY:
			this->ProcState_EMPTY(lpInfo);
			break;
		case HAPPY_HOUR_ZEN_STATE_START:
			this->ProcState_START(lpInfo);
			break;
		}
	}
}

void HappyHourZen::ProcState_BLANK(HAPPY_HOUR_ZEN_INFO* lpInfo)
{
}

void HappyHourZen::ProcState_EMPTY(HAPPY_HOUR_ZEN_INFO* lpInfo)
{
	if (lpInfo->RemainTime > 0 && lpInfo->RemainTime <= (lpInfo->AlarmTime * 60))
	{
		if ((lpInfo->AlarmMinSave = (((lpInfo->RemainTime % 60) == 0) ? ((lpInfo->RemainTime / 60) - 1) : (lpInfo->RemainTime / 60))) != lpInfo->AlarmMinLeft)
		{
			lpInfo->AlarmMinLeft = lpInfo->AlarmMinSave;

			gNotice.GCNoticeSendToAll(0, 0, 0, 0, 0, 0, "Happy hour zen and exp will start in %d minute(s)", (lpInfo->AlarmMinLeft + 1));
		}
	}

	if (lpInfo->RemainTime <= 0)
	{
		gNotice.GCNoticeSendToAll(0, 0, 0, 0, 0, 0, "Happy hour zen and exp started!");
		this->SetState(lpInfo, HAPPY_HOUR_ZEN_STATE_START);
	}
}

void HappyHourZen::ProcState_START(HAPPY_HOUR_ZEN_INFO* lpInfo)
{
	if (lpInfo->RemainTime <= 0)
	{
		gNotice.GCNoticeSendToAll(0, 0, 0, 0, 0, 0, "Happy hour zen and exp finished!");
		this->SetState(lpInfo, HAPPY_HOUR_ZEN_STATE_EMPTY);
	}
}

void HappyHourZen::SetState(HAPPY_HOUR_ZEN_INFO* lpInfo, int state)
{
	switch ((lpInfo->State = state))
	{
	case HAPPY_HOUR_ZEN_STATE_BLANK:
		this->SetState_BLANK(lpInfo);
		break;
	case HAPPY_HOUR_ZEN_STATE_EMPTY:
		this->SetState_EMPTY(lpInfo);
		break;
	case HAPPY_HOUR_ZEN_STATE_START:
		this->SetState_START(lpInfo);
		break;
	}
}

void HappyHourZen::SetState_BLANK(HAPPY_HOUR_ZEN_INFO* lpInfo)
{
}

void HappyHourZen::SetState_EMPTY(HAPPY_HOUR_ZEN_INFO* lpInfo)
{
	lpInfo->AlarmMinSave = -1;
	lpInfo->AlarmMinLeft = -1;

	this->CheckSync(lpInfo);
}

void HappyHourZen::SetState_START(HAPPY_HOUR_ZEN_INFO* lpInfo)
{
	lpInfo->AlarmMinSave = -1;
	lpInfo->AlarmMinLeft = -1;

	lpInfo->RemainTime = lpInfo->EventTime * 60;

	lpInfo->TargetTime = (int)(time(0) + lpInfo->RemainTime);
}

void HappyHourZen::CheckSync(HAPPY_HOUR_ZEN_INFO* lpInfo)
{
	if (lpInfo->StartTime.empty() != 0)
	{
		this->SetState(lpInfo, HAPPY_HOUR_ZEN_STATE_BLANK);
		return;
	}

	CTime ScheduleTime;

	CScheduleManager ScheduleManager;

	for (std::vector<HAPPY_HOUR_ZEN_START_TIME>::iterator it = lpInfo->StartTime.begin(); it != lpInfo->StartTime.end(); it++)
	{
		ScheduleManager.AddSchedule(it->Year, it->Month, it->Day, it->Hour, it->Minute, it->Second, it->DayOfWeek);
	}

	if (ScheduleManager.GetSchedule(&ScheduleTime) == 0)
	{
		this->SetState(lpInfo, HAPPY_HOUR_ZEN_STATE_BLANK);
		return;
	}

	lpInfo->RemainTime = (int)difftime(ScheduleTime.GetTime(), time(0));

	lpInfo->TargetTime = (int)ScheduleTime.GetTime();

	LogAdd(LOG_BLUE, "[Happy hour zen] Start in: [%d] min remain", (lpInfo->RemainTime / 60));
}

int HappyHourZen::GetZenRate()
{
	return (this->m_HappyHourZenInfo.State == HAPPY_HOUR_ZEN_STATE_START ? this->m_HappyHourZenInfo.RateZen : 100);
}

int HappyHourZen::GetExpRate()
{
	return (this->m_HappyHourZenInfo.State == HAPPY_HOUR_ZEN_STATE_START ? 200 : 100);
}

#endif
