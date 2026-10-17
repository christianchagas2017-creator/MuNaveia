#include "stdafx.h"
#include "CustomPick.h"
#include "Util.h"
#include "MemScript.h"
#include "ServerInfo.h"
#include "ItemManager.h"
#include "Message.h"
#include "Notice.h"
#include "User.h"

CCustomPick gCustomPick;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCustomPick::CCustomPick() // OK
{
	this->Init();
}

CCustomPick::~CCustomPick() // OK
{

}

void CCustomPick::Init() // OK
{
	this->m_PickItemList.clear();
	this->CommandEnabled = false;
	this->CommandLevel[0] = false;
	this->CommandLevel[1] = false;
	this->CommandLevel[2] = false;
	this->CommandLevel[3] = false;
}

void CCustomPick::Load(char* path) // OK
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

	this->m_PickItemList.clear();

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

					int itemSection = lpMemScript->GetNumber();

					int itemIndex = lpMemScript->GetAsNumber();

					this->m_PickItemList.push_back(GET_ITEM(itemSection, itemIndex));

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

void CCustomPick::LoadConfig(char* section, char* path) // OK
{
	this->CommandEnabled = GetPrivateProfileInt(section, "CustomPickSwitch", 0, path);
	this->CommandLevel[0] = GetPrivateProfileInt(section, "CustomPick_FREE", 0, path);
	this->CommandLevel[1] = GetPrivateProfileInt(section, "CustomPick_VIP1", 0, path);
	this->CommandLevel[2] = GetPrivateProfileInt(section, "CustomPick_VIP2", 0, path);
	this->CommandLevel[3] = GetPrivateProfileInt(section, "CustomPick_VIP3", 0, path);
}

bool CCustomPick::IsPickItem(LPOBJ lpObj, int ItemIndex)
{
	if (!lpObj->m_CustomPickEnabled) return false;

	for (const int& item: this->m_PickItemList) {
		if (item == ItemIndex) return true;
	}

	return false;
}

void CCustomPick::ToggleUserPick(LPOBJ lpObj)
{
	if (this->CommandEnabled == 0) 
	{
		return;
	}

	if (this->CommandLevel[lpObj->AccountLevel] == 0) {
		return;
	}

	if (lpObj->m_CustomPickEnabled)
	{
		lpObj->m_CustomPickEnabled = false;
		gNotice.GCNoticeSend(lpObj->Index, 1, 0, 0, 0, 0, 0, gMessage.GetMessage(501));
	}
	else
	{
		lpObj->m_CustomPickEnabled = true;
		gNotice.GCNoticeSend(lpObj->Index, 1, 0, 0, 0, 0, 0, gMessage.GetMessage(500));
	}
}