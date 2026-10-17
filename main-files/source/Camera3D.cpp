// Camera.cpp: implementation of the CCamera class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Camera3D.h"
#include "ZzzLodTerrain.h"
#include "NewUISystem.h"

CCamera gCamera;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCamera::CCamera() // OK
{
	this->LastPress = GetTickCount();
	this->m_CursorX = MouseX;
	this->m_CursorY = MouseY;
}

CCamera::~CCamera() // OK
{

}


void CCamera::Toggle() // OK
{
	m_CameraOnOff ^= 1;
	if (m_CameraOnOff)
	{
		g_pChatListBox->AddText("", "Ajuste da câmera (F11) ativado.", SEASON3B::TYPE_SYSTEM_MESSAGE);
	}
	else {
		g_pChatListBox->AddText("", "Ajuste da câmera (F11) desativado.", SEASON3B::TYPE_SYSTEM_MESSAGE);
	}
}

void CCamera::Restore() // OK
{
	CameraZoom = 0;
	CameraAngle[2] = -45.f;
	AngleY3D = 0;
	AngleZ3D = 0;
}

bool wasKeyPressed = false;

void CCamera::Update() // OK
{
	if (SceneFlag != MAIN_SCENE) return;

	SHORT keyState = GetAsyncKeyState(VK_F11);

	if ((keyState & 0x8000) && !wasKeyPressed)
	{
		wasKeyPressed = true;
		this->Toggle();
	}
	else if (!(keyState & 0x8000) && wasKeyPressed)
	{
		wasKeyPressed = false;
	}

	if (!m_CameraOnOff) return;

	if (HIBYTE(GetAsyncKeyState(VK_F10)) == 128)
	{
		this->Restore();
		
		return;
	}

	if (HIBYTE(GetAsyncKeyState(VK_MBUTTON)) == 128)
	{
		if (this->m_CursorX < MouseX)
		{
			CameraAngle[2] -= 4;
		}

		if (this->m_CursorX > MouseX)
		{
			CameraAngle[2] += 4;
		}

		if (this->m_CursorY < MouseY)
		{
			//if (AngleY3D < 10) {
			//	AngleY3D += 1.3f;
			//	AngleZ3D += 24.0f;
			//}
		}

		if (this->m_CursorY > MouseY)
		{
			//if (AngleY3D > -15) {
			//	AngleY3D -= 1.3f;
			//	AngleZ3D -= 24.0f;
			//}
		}
	}

	if (HIBYTE(GetAsyncKeyState(VK_RIGHT)) == 128) // RIGHT ARROW
		CameraAngle[2] -= 4;

	if (HIBYTE(GetAsyncKeyState(VK_LEFT)) == 128) // LEFT ARROW
		CameraAngle[2] += 4;

	if (HIBYTE(GetAsyncKeyState(VK_DOWN)) == 128) // DOWN ARROW
		if (CameraZoom < 30) {
			CameraZoom += 1;
			if (AngleY3D < 5) {
				CameraZoom += 3;
				AngleY3D += 1.5f;
				AngleZ3D += 27.0f;
			}
		}



	//CameraZoom = 0;
	//CameraAngle[2] = -45.f;
	//AngleY3D = 0;
	//AngleZ3D = 0;

	if (HIBYTE(GetAsyncKeyState(VK_UP)) == 128) // UP ARROW
	{
		if (CameraZoom > -19) {
			CameraZoom -= 1;
			if (AngleY3D > -25) {
				AngleY3D -= 1.5f;
				AngleZ3D -= 28.0f;
			}
		}
	}


	//SCROLL
	if (MouseWheel > 0)
		if (CameraZoom > -19) { //25
			CameraZoom -= 1;
			if (AngleY3D > -25) {
				AngleY3D -= 1.5f;
				AngleZ3D -= 27.0f;
			}
		}
	if (MouseWheel < 0)
		if (CameraZoom < 30) {
			CameraZoom += 1;
			if (AngleY3D < 5) {
				CameraZoom += 3;
				AngleY3D += 1.5f;
				AngleZ3D += 27.0f;
			}
		}

	//SCROLL
	//if (MouseWheel > 0)
	//	if (CameraZoom > -15) {
	//		CameraZoom -= 2;
	//		if (AngleY3D > -20) {
	//			AngleY3D -= 1.3f;
	//			AngleZ3D -= 24.0f;
	//		}
	//	}
	//if (MouseWheel < 0)
	//	if (CameraZoom < 5) {
	//		CameraZoom += 2;
	//		if (AngleY3D < 10) {
	//			AngleY3D += 1.3f;
	//			AngleZ3D += 24.0f;
	//		}
	//	}

	if (MouseWheel != 0)
		MouseWheel = 0;

	this->m_CursorX = MouseX;

	this->m_CursorY = MouseY;

	return;
}
