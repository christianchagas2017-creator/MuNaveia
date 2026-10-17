#pragma once

#include "User.h"
#include <unordered_map>
#include <vector>

class CServerCache {
public:
	CServerCache();
	~CServerCache();

	int GetCacheInt(std::string key);
	std::string GetCacheStr(std::string key);
	std::string GetCacheTableStr(std::string key, int innerKey);
	void SetCacheInt(std::string key, int value);
	void SetCacheStr(std::string key, std::string value);
	int SetCacheTableStr(std::string key, int innerKey, std::string value);


private:
	std::unordered_map<std::string, int> m_cacheInt;
	std::unordered_map<std::string, std::string> m_cacheStr;
	std::unordered_map<std::string, std::vector<std::string>> m_cacheTableStr;


};

extern CServerCache gServerCache;