#pragma once

#include <vector>
#include "User.h"

class CCustomPick
{
public:
	CCustomPick();
	virtual ~CCustomPick();
	void Init();
	void Load(char* path);
	void LoadConfig(char* section, char* path);	
	bool IsPickItem(LPOBJ lpObj, int ItemIndex);
	void ToggleUserPick(LPOBJ lpObj);
	
public:
	std::vector<int> m_PickItemList;
	int CommandEnabled;
	int CommandLevel[4];
};

extern CCustomPick gCustomPick;
