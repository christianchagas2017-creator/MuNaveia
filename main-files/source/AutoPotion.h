#pragma once
#pragma once
#include <windows.h>

class CAutoPotion
{
public:
    CAutoPotion();
    virtual ~CAutoPotion();
    void CreateAutoPotion();
    void CheckAutoPotion();
    static DWORD WINAPI AutoPotionWorker(CAutoPotion* AutoPotion);
private:
    bool m_Potion;            // Flag
    HANDLE m_Thread;
    void UseItem(int itemType);
};

extern CAutoPotion gAutoPotion;