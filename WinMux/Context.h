#pragma once
#include "Defines.h"
#include <wx/wx.h>

namespace WinMux
{ 
	struct Context
	{
		//	Basic Colors
		// 
		//////////////////////////////////////////////////
		ButtonColors btncols_WinTitle;
		ButtonColors btncols_TitleBtn;
		ButtonColors btncols_CloseBtn;
		ButtonColors btncol_Tabs;
		ButtonColors btncols_TabsUnsel;

		Color3 color_tabsBG;

		//	Layout settings
		// 
		//////////////////////////////////////////////////
		int vertWinPadding = 2;
		int horizWinPadding = 2;

		int sashWidth = 5;

		wxSize minWindowSize;

		//	Capture settings
		// 
		//////////////////////////////////////////////////
		int millisecondsAwaitGUI = 100;

		//	Tabs settings
		//
		//////////////////////////////////////////////////
		int tabHeight = 30;

		//	Title bar Settings
		//	
		//////////////////////////////////////////////////
		int titleHeight = 20;
		int titleBoxRegionWidth = 20;
		int titleSysBoxIconDim = 16;
		int titleBoxButtonIconDim = 16;
		int titleFontSize = 10;
		int titleTextLeftPadding = 10;

		//	Drag Settings
		// 
		//////////////////////////////////////////////////
		int barDropPreviewWidth = 10;
		int intoDropPrevSize = 20;
		float minDragDist = 5.0f;
		Color3 previewUnhovered;
		Color3 previewHovered;

	public:
		Context();
	};
}