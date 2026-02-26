#include "Context.h"

namespace WinMux
{
	Context::Context()
		: btncols_WinTitle(
			Color3(100, 100, 255),
			Color3(120,	120, 255),
			Color3(50,	50,  255)),
		  btncols_TitleBtn(
			Color3(200, 200, 200),
			Color3(220, 220, 220),
			Color3(100, 100, 100)),
		  btncols_CloseBtn(
		    Color3(255,   0,   0),
			Color3(255,	100,  100),
			Color3(100,   0,   0)),
		  btncol_Tabs(
			Color3( 90,  90, 255),
			Color3(110, 110, 255),
			Color3( 60,  60, 255)),
		  btncols_TabsUnsel(
			Color3( 70,  60, 220),
		    Color3( 80,  80, 240),
		    Color3( 60,  60, 100)),
		  color_tabsBG		(50,	50,		200),
		  minWindowSize(50, 80),
		barDropPreviewWidth(10),
		intoDropPrevSize(20),
		minDragDist(5.0f),
		previewUnhovered(128,128,128),
		previewHovered(255, 128, 64)
	{}
}