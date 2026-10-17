#pragma once
#include "Protocol.h"

struct PMSG_CLIENT_INFO_RECV
{
	PSBMSG_HEAD header;

	char m_MacAddress[40];
	BYTE lang;
};

class ClientInfo
{
public:
	ClientInfo() = default;
	~ClientInfo() = default;

	void ReceiveInfo(PMSG_CLIENT_INFO_RECV *lpMsg, int aIndex);

};
extern ClientInfo gClientInfo;