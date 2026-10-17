#pragma once
#include <vector>

class CImmortalPet
{
public:

	CImmortalPet();
	virtual ~CImmortalPet();

	void Clear();
	void Load(char* path);
	bool CheckIsItem(int ItemIndex);

private:
	std::vector<int> m_ImmortalPets;
};

extern CImmortalPet gImmortalPet;