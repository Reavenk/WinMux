#pragma once
#include "Defines.h"
#include <wx/wx.h>

namespace WinMux
{
	struct OverlapDropDst
	{
		bool active;
		WinInst* win;
		Node* node;
		DockDir dockDir;
		wxRect previewLoc;
	public:
		OverlapDropDst();
		OverlapDropDst(WinInst* win, Node* node, DockDir dockDir, wxRect previewLoc);
		static OverlapDropDst Inactive(); //!TODO: const static
	};
}