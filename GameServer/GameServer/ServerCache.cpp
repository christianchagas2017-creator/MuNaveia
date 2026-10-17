#include "stdafx.h"
#include "ServerCache.h"
#include <vector>

CServerCache gServerCache;

CServerCache::CServerCache()
{
	this->m_cacheInt.clear();
	this->m_cacheStr.clear();
}

CServerCache::~CServerCache()
{
	this->m_cacheInt.clear();
	this->m_cacheStr.clear();
}

int CServerCache::GetCacheInt(std::string key) {
	auto itr = m_cacheInt.find(key);
	if (itr == m_cacheInt.end())
	{
		m_cacheInt[key] = 0;

		return 0;
	}

	return m_cacheInt[key];
}

void CServerCache::SetCacheInt(std::string key, int value) {
	m_cacheInt[key] = value;
}

std::string CServerCache::GetCacheTableStr(std::string key, int innerKey) {
	auto itr = m_cacheTableStr.find(key);
	if (itr == m_cacheTableStr.end())
	{
		return "invalid_key";
	}

	if (innerKey >= itr->second.size()) {
		return "out_of_range";
	}

	return itr->second.at(innerKey);
}

int CServerCache::SetCacheTableStr(std::string key, int innerKey, std::string value) {
	auto itr = m_cacheTableStr.find(key);
	if (itr == m_cacheTableStr.end())
	{
		std::vector<std::string> a;
		m_cacheTableStr.insert(std::pair<std::string, std::vector<std::string>>(key, a));
		itr = m_cacheTableStr.find(key);
	}

	for (int i = 0; i < itr->second.size(); i++) {
		if(itr->second.at(i) == value) {
			return i;
		}
	}

	itr->second.push_back(value);
	
	return (itr->second.size() - 1);
}

std::string CServerCache::GetCacheStr(std::string key) {
	auto itr = m_cacheStr.find(key);
	if (itr == m_cacheStr.end())
	{
		m_cacheStr[key] = "nil";

		return "nil";
	}

	return m_cacheStr[key];
}

void CServerCache::SetCacheStr(std::string key, std::string value) {
	m_cacheStr[key] = value;
}