#include "OverlapDropDst.h"

namespace WinMux
{
	OverlapDropDst::OverlapDropDst()
		: active(false)
	{}

	OverlapDropDst::OverlapDropDst(WinInst* win, Node* node, DockDir dockDir, wxRect previewLoc)
		: active(true),
		win(win),
		node(node),
		dockDir(dockDir),
		previewLoc(previewLoc)
	{
	}

	OverlapDropDst OverlapDropDst::Inactive()
	{
		return OverlapDropDst();
	}
}