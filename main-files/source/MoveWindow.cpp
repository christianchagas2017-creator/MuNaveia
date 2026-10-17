#include "stdafx.h"
#include "MoveWindow.h"

cMoveWindow::cMoveWindow()
{
    m_PosTemp.x = 0;
    m_PosTemp.y = 0;
    m_OriginalPos.x = 0;  // Nova variável para guardar a posição original
    m_OriginalPos.y = 0;
    //
    Width = 0;
    Height = 0;
    //
    IsMove = false;
    //
    m_PosOffset.x = 0;
    m_PosOffset.y = 0;
}

cMoveWindow::~cMoveWindow()
{

}

void cMoveWindow::Create(int x, int y, int width, int height)
{
    m_PosTemp.x = x;
    m_PosTemp.y = y;
    m_OriginalPos.x = x;  // Guarda a posição original ao criar
    m_OriginalPos.y = y;
    //
    Width = width;
    Height = height;
}

bool cMoveWindow::UpdatePosition(int x, int y, int WindowID)
{
    // Verifica se CTRL + Z foi pressionado
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000 && GetAsyncKeyState('Z') & 0x8000)
    {
        ResetToOriginalPosition();
        return false;
    }

    if (SEASON3B::IsRepeat(VK_LBUTTON) == 1 && SEASON3B::CheckMouseIn(x, y, Width, 30))
    {
        this->IsMove = true;
    }
    else
    {
        if (SEASON3B::IsRepeat(VK_LBUTTON) == 0)
        {
            this->IsMove = false;
            //
            m_PosOffset.x = MouseX - x;
            m_PosOffset.y = MouseY - y;
            //
            m_PosTemp.x = x;
            m_PosTemp.y = y;
        }
    }

    int GetWindowsX = GetSystemMetrics(SM_CXSCREEN);  // Largura total da tela
    int GetWindowsY = GetSystemMetrics(SM_CYSCREEN);  // Altura total da tela

    if (SEASON3B::CheckMouseIn(m_PosTemp.x, m_PosTemp.y, Width, 30) || this->IsMove)
    {
        if (this->IsMove)
        {
            if (MouseX > 0 && MouseX < GetWindowsX)
            {
                m_PosTemp.x = MouseX - m_PosOffset.x;

                if (m_PosTemp.x > GetWindowsX - Width)
                {
                    m_PosTemp.x = GetWindowsX - Width;
                }

                if (m_PosTemp.x < 0)
                {
                    m_PosTemp.x = 0;
                }
            }

            if (MouseY > 0 && MouseY < GetWindowsY)
            {
                m_PosTemp.y = MouseY - m_PosOffset.y;

                if (m_PosTemp.y > GetWindowsY - Height)
                {
                    m_PosTemp.y = GetWindowsY - Height;
                }

                if (m_PosTemp.y < 0)
                {
                    m_PosTemp.y = 0;
                }
            }

            return true;
        }
    }
    else
    {
        if (SEASON3B::IsRepeat(VK_LBUTTON) == 0)
        {
            this->IsMove = false;
        }
    }

    return false;
}

// Novo método para resetar a posição
void cMoveWindow::ResetToOriginalPosition()
{
    m_PosTemp.x = m_OriginalPos.x;
    m_PosTemp.y = m_OriginalPos.y;
    IsMove = false;
}