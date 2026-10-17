#pragma once

#define MAX_CUSTOM_PET 300

struct CUSTOM_PET_INFO
{
	int ItemIndex;
	int Movement;
};

class CCustomPet
{
public:
	CCustomPet();
	virtual ~CCustomPet();

	void Clear();
	void Load(char* path);
	void SetInfo(CUSTOM_PET_INFO lpInfo, int index);

	bool CheckCustomPet(WORD ItemIndex);
	int GetCustomPetType(WORD ItemIndex);

public:
	CUSTOM_PET_INFO m_CustomItemInfo[MAX_CUSTOM_PET];
	int m_MaxCustomItemLoad;
};
extern CCustomPet gCustomPets;