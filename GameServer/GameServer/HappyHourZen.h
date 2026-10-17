#pragma once

#ifdef FAIRPLAY

enum eHappyHourZenState
{
	HAPPY_HOUR_ZEN_STATE_BLANK = 0,
	HAPPY_HOUR_ZEN_STATE_EMPTY = 1,
	HAPPY_HOUR_ZEN_STATE_START = 2,
};

struct HAPPY_HOUR_ZEN_START_TIME
{
	int Year;
	int Month;
	int Day;
	int DayOfWeek;
	int Hour;
	int Minute;
	int Second;
};

struct HAPPY_HOUR_ZEN_INFO
{
	int State;
	int RemainTime;
	int TargetTime;
	int TickCount;
	int AlarmMinSave;
	int AlarmMinLeft;
	int AlarmTime;
	int EventTime;
	int RateZen;
	std::vector<HAPPY_HOUR_ZEN_START_TIME> StartTime;
};

class HappyHourZen
{
public:
	HappyHourZen();
	~HappyHourZen();

	void Init();
	void Load(char* path);
	void MainProc();
	void ProcState_BLANK(HAPPY_HOUR_ZEN_INFO* lpInfo);
	void ProcState_EMPTY(HAPPY_HOUR_ZEN_INFO* lpInfo);
	void ProcState_START(HAPPY_HOUR_ZEN_INFO* lpInfo);
	void SetState(HAPPY_HOUR_ZEN_INFO* lpInfo, int state);
	void SetState_BLANK(HAPPY_HOUR_ZEN_INFO* lpInfo);
	void SetState_EMPTY(HAPPY_HOUR_ZEN_INFO* lpInfo);
	void SetState_START(HAPPY_HOUR_ZEN_INFO* lpInfo);
	void CheckSync(HAPPY_HOUR_ZEN_INFO* lpInfo);

	int GetZenRate();
	int GetExpRate();

private:
	HAPPY_HOUR_ZEN_INFO m_HappyHourZenInfo;
};

extern HappyHourZen gHappyHourZen;

#endif