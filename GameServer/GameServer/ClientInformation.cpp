#include "stdafx.h"
#include "ClientInformation.h"
#include "User.h"
#include "LuaGameServer.h"

ClientInfo gClientInfo;

void ClientInfo::ReceiveInfo(PMSG_CLIENT_INFO_RECV* lpMsg, int aIndex)
{
	if (OBJECT_RANGE(aIndex) == 0) {
		return;
	}

	LPOBJ lpObj = &gObj[aIndex];


	switch (lpMsg->lang)
	{
	case 0:
		strcpy_s(lpObj->m_Language, "Eng");
		break;
	case 1:
		strcpy_s(lpObj->m_Language, "Por");
		break;
	case 2:
		strcpy_s(lpObj->m_Language, "Spn");
		break;
	default:
		strcpy_s(lpObj->m_Language, "Por");
		break;
	}

	gLuaGameServer.MacAddressPlayer(aIndex, lpMsg->m_MacAddress);

	strcpy_s(gObj[aIndex].m_MacAddress, lpMsg->m_MacAddress);
}