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

	/// <summary>
	/// Sets the Window icon set for a Window.
	/// </summary>
	/// <param name="win">The window to set the icons for.</param>
	/// <param name="resourceID">The icon's resource ID.</param>
	/// <param name="large">Specify whether to use the big or small icon in the set.</param>
	void SetIcon(wxWindow* win, int resourceID, bool large);

	/// <summary>
	/// The function is used to set Window icons to the same set across
	/// the entire application.
	/// </summary>
	/// <param name="win">The window to apply the icon set for.</param>
	void SetDefaultIcons(wxWindow* win);

	void SetWindowTransparency(wxWindow* win, int alpha);

	void SetWindowTransparency(HWND hwnd, int alpha);
}

