#include "stdafx.h"
#include "DarkSpiritNew.h"
#include "Util.h"

CCustomDarkSpirit gCustomDarkSpirit;

CCustomDarkSpirit::CCustomDarkSpirit()
{
	gCustomDarkSpirit.m_DarkSpiritInfo.clear();
}

CCustomDarkSpirit::~CCustomDarkSpirit()
{
}

void CCustomDarkSpirit::SetDarkSpirit(int ItemIndex)
{
	gCustomDarkSpirit.m_DarkSpiritInfo.insert(std::make_pair(ItemIndex, ItemIndex));
}

bool CCustomDarkSpirit::checkIsDarkSpirit(int ItemIndex)
{
	if (ItemIndex == -1) 
	{
		return 0;
	}

	auto it = this->m_DarkSpiritInfo.find(ItemIndex);

	if (it == this->m_DarkSpiritInfo.end()) 
	{
		return 0;
	}

	return 1;
}


int CCustomDarkSpirit::checkDarkSpirit(int ItemIndex)
{
	if (ItemIndex == -1)
	{
		return 0;
	}

	auto it = this->m_DarkSpiritInfo.find(ItemIndex);

	if (it == this->m_DarkSpiritInfo.end())
	{
		return it->second;
	}

	return -1;
}
