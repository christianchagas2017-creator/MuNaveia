#pragma once
#include <map>

class CPatenteInfo
{
public:

	CPatenteInfo();
	virtual ~CPatenteInfo();

	void Load(char* path);
	int GetPatentType(int index);
	

private:
	std::map<int, int> m_PatenteInfo;
};

extern CPatenteInfo gPatenteInfo;