#include "StdAfx.h"
#include "PatenteInfo.h"
#include "MemScript.h"
#include "Util.h"

CPatenteInfo gPatenteInfo;

CPatenteInfo::CPatenteInfo()
{
	this->m_PatenteInfo.clear();
}

CPatenteInfo::~CPatenteInfo()
{
	this->m_PatenteInfo.clear();
}


void CPatenteInfo::Load(char* path)
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

	this->m_PatenteInfo.clear();
	int count = 0;
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

					int patenteIndex = lpMemScript->GetNumber();

					int patentType = lpMemScript->GetAsNumber();
					count++;

					this->m_PatenteInfo[patenteIndex] = patentType;

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

int CPatenteInfo::GetPatentType(int index)
{
	auto it = this->m_PatenteInfo.find(index);

	if (it == this->m_PatenteInfo.end()) {
		return -1;
	}

	return it->second;
}