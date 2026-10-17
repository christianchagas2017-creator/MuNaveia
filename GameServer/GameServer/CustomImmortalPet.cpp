#include "stdafx.h"
#include "CustomImmortalPet.h"
#include "Util.h"
#include "MemScript.h"
#include "ItemManager.h"

CImmortalPet gImmortalPet;

CImmortalPet::CImmortalPet()
{
	this->Clear();
}

CImmortalPet::~CImmortalPet()
{

}

void CImmortalPet::Clear()
{
	this->m_ImmortalPets.clear();
}

void CImmortalPet::Load(char* path)
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

	this->Clear();

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
					
					int section = lpMemScript->GetNumber();

					int index = lpMemScript->GetAsNumber();

					this->m_ImmortalPets.push_back(GET_ITEM(section, index));
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
		printf(lpMemScript->GetLastError());
	}

	delete lpMemScript;
}

bool CImmortalPet::CheckIsItem(int ItemIndex)
{
	for (int pet : m_ImmortalPets)
	{
		if (pet == ItemIndex)
		{
			return true;
		}
	}

	return false;
}
