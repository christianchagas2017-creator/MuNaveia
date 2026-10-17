#include "stdafx.h"
#include "Pets.h"
#include "MemScript.h"
#include "Util.h"

CCustomPet gCustomPets;

CCustomPet::CCustomPet()
{
	this->Clear();
}

CCustomPet::~CCustomPet()
{
}

void CCustomPet::Clear()
{
	for (int i = 0; i < MAX_CUSTOM_PET; i++) {
		this->m_CustomItemInfo[i].ItemIndex = -1;
	}

	this->m_MaxCustomItemLoad = 0;
}

void CCustomPet::Load(char* path)
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
	this->m_MaxCustomItemLoad = 0;

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

					CUSTOM_PET_INFO info;

					memset(&info, 0, sizeof(info));

					info.ItemIndex = lpMemScript->GetNumber();

					info.Movement = lpMemScript->GetAsNumber();

					lpMemScript->GetAsNumber();

					lpMemScript->GetAsNumber();

					lpMemScript->GetAsNumber();

					lpMemScript->GetAsString();

					lpMemScript->GetAsString();

					this->SetInfo(info, this->m_MaxCustomItemLoad);

					this->m_MaxCustomItemLoad++;
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

void CCustomPet::SetInfo(CUSTOM_PET_INFO lpInfo, int index)
{
	if (index < 0 || index >= MAX_CUSTOM_PET)
	{
		return;
	}

	this->m_CustomItemInfo[index] = lpInfo;
}

bool CCustomPet::CheckCustomPet(WORD ItemIndex)
{
	for (int i = 0; i < this->m_MaxCustomItemLoad; i++)
	{
		if (this->m_CustomItemInfo[i].ItemIndex == ItemIndex)
		{
			return 1;
		}
	}

	return 0;
}

int CCustomPet::GetCustomPetType(WORD ItemIndex)
{
	for (int i = 0; i < this->m_MaxCustomItemLoad; i++)
	{
		if (this->m_CustomItemInfo[i].ItemIndex == ItemIndex)
		{
			return this->m_CustomItemInfo[i].Movement;
		}
	}

	return -1;
}
