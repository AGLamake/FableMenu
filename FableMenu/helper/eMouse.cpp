#include "eMouse.h"
#include <Windows.h>
#include <iostream>
#include "../eDirectX9Hook.h"
#include "../fablemenu.h"
#include "../eSettingsManager.h"

eMouse eMouse::mouse;

eMouse::eMouse()
{
	Clear();
}

void eMouse::Clear()
{
	x = 0;
	y = 0;
	deltaX = 0;
	deltaY = 0;
	gotDelta = false;
}

void eMouse::UpdateMouse()
{
	if (!TheMenu->m_bIsActive && TheMenu->ms_bFreeCam && TheMenu->m_nFreeCameraMode == FREE_CAMERA_CUSTOM && IsWindowFocused())
	{
		mouse.Clear();
		POINT point;
		GetCursorPos(&point);

		if (!mouse.gotDelta)
		{
			eMouse::LockMouse();
			mouse.gotDelta = true;
		}

		POINT center;
		GetCursorPos(&center);

		mouse.deltaX = point.x - center.x;
		mouse.deltaY = point.y - center.y;
		mouse.deltaX *= SettingsMgr->mouse.sens;
		mouse.deltaY *= SettingsMgr->mouse.sens;
		mouse.gotDelta = false;
	}
}


int eMouse::GetDeltaX()
{
	return SettingsMgr->mouse.invert_x ? -mouse.deltaX : mouse.deltaX;
}

int eMouse::GetDeltaY()
{
	return SettingsMgr->mouse.invert_y ? mouse.deltaY : -mouse.deltaY;
}
void eMouse::LockMouse()
{
	if (eDirectX9Hook::ms_hWindow)
	{
		RECT rect;
		GetWindowRect(eDirectX9Hook::ms_hWindow, &rect);

		SetCursorPos(rect.right / 2, rect.bottom / 2);
	}
}
