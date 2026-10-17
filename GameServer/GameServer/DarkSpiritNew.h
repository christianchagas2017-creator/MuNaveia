#pragma once

class CCustomDarkSpirit 
{
public:
	CCustomDarkSpirit();
	~CCustomDarkSpirit();

	void SetDarkSpirit(int ItemIndex);
	bool checkIsDarkSpirit(int ItemIndex);
	int checkDarkSpirit(int ItemIndex);

	std::map<int, int> m_DarkSpiritInfo;
};

extern CCustomDarkSpirit gCustomDarkSpirit;