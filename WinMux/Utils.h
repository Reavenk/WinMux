#pragma once
#include "Defines.h"
#include <wx/wx.h>

namespace WinMux
{ 
	bool IsPointInRect(
		const wxPoint& point, 
		const wxPoint& rectPos, 
		const wxSize& rectSize);

	inline wxColor ToWxColor(const Color3& c)
	{
		return wxColor(c.r, c.g, c.b);
	}
}

