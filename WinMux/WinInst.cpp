#include <queue>
#include "WinInst.h"
#include "App.h"
#include "Node.h"
#include "Context.h"
#include "Utils.h"
#include "SubTitlebar.h"
#include "SubTabs.h"
#include <Windows.h>
#include "resource.h"

namespace WinMux
{
    bool GetDockDirGrainStats(DockDir dir, OUT NodeType& type, OUT bool& lower)
    {
        switch(dir)
        {
        case DockDir::Left:
            type = NodeType::SplitHoriz;
            lower = true;
            break;
        case DockDir::Top:
            type = NodeType::SplitVert;
            lower = true;
            break;
        case DockDir::Right:
            type = NodeType::SplitHoriz;
            lower = false;
            break;
        case DockDir::Bottom:
            type = NodeType::SplitVert;
            lower = false;
            break;

        default:
            return false;
        }
        return true;
    }

	BEGIN_EVENT_TABLE(WinInst, wxFrame)
		EVT_PAINT                   (WinInst::OnPaintEvent)
        EVT_KEY_DOWN                (WinInst::OnKeyDownEvent)
        EVT_MOTION                  (WinInst::OnMouseMotionEvent)
        EVT_LEFT_DOWN               (WinInst::OnMouseLeftDownEvent)
        EVT_LEFT_UP                 (WinInst::OnMouseLeftUpEvent)
        EVT_MOUSE_CAPTURE_LOST      (WinInst::OnMouseCaptureLostEvent)
        EVT_MOUSE_CAPTURE_CHANGED   (WinInst::OnMouseCaptureChangedEvent)
        EVT_ENTER_WINDOW            (WinInst::OnMouseEnterEvent)
        EVT_LEAVE_WINDOW            (WinInst::OnMouseLeaveEvent)
        EVT_SIZE                    (WinInst::OnSizeEvent)
        EVT_CLOSE                   (WinInst::OnClose)
        EVT_WINDOW_DESTROY          (WinInst::OnDestroy)

        EVT_TIMER                   (WinInst::TIMER_WaitForGUI,             WinInst::OnTimerEvent_WaitGUIProcess)
        EVT_TIMER                   (WinInst::TIMER_RefreshLayout,          WinInst::OnTimerEvent_RefreshLayout)

        EVT_MENU                    (WinInst::MENU_TestAddTop,              WinInst::OnMenu_TestAddTop)
        EVT_MENU                    (WinInst::MENU_TestAddLeft,             WinInst::OnMenu_TestAddLeft)
        EVT_MENU                    (WinInst::MENU_TestAddBottom,           WinInst::OnMenu_TestAddBottom)
        EVT_MENU                    (WinInst::MENU_TestAddRight,            WinInst::OnMenu_TestAddRight)
        EVT_MENU                    (WinInst::MENU_CreateDebugSetup,        WinInst::OnMenu_CreateDebugSetup)
        EVT_MENU                    (WinInst::MENU_CreateDebugTabSetup,     WinInst::OnMenu_CreateDebugTabSetup)

        EVT_MENU                    (WinInst::MENU_CloseMode_DestroyAll,    WinInst::OnMenu_CloseMode_Destroy)
        EVT_MENU                    (WinInst::MENU_Close_ReleaseAll,        WinInst::OnMenu_CloseMode_Release)
        EVT_MENU                    (WinInst::MENU_Close_BlockingCloseAll,  WinInst::OnMenu_CloseMode_Close)

		EVT_MENU                    (WinInst::MENU_ReleaseAll,              WinInst::OnMenu_ReleaseAll)
		EVT_MENU                    (WinInst::MENU_DetachAll,               WinInst::OnMenu_DetachAll)
		EVT_MENU                    (WinInst::MENU_CloseAll,                WinInst::OnMenu_CloseAll)
    END_EVENT_TABLE()

	WinInst::WinInst(App* appOwner, CloseMode closeMode)
		: wxFrame(nullptr, wxID_ANY, "WinMux Window", wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE|wxFULL_REPAINT_ON_RESIZE|wxCLIP_CHILDREN),
          appOwner(appOwner),
          awaitGUITimer(this, TIMER_WaitForGUI),
          refreshLayoutTimer(this, TIMER_RefreshLayout),
            closeMode(closeMode)
	{
        // Unused, but set as filler to override default behavior
        this->SetSizer(new wxBoxSizer(wxVERTICAL));

		root = nullptr;

        wxMenu* menuFile = new wxMenu;
        menuFile->AppendSeparator();
        menuFile->Append(wxID_EXIT);

        wxMenu* menuDebug = new wxMenu;
        menuDebug->Append(MENU_TestAddTop,      "Add Top");
        menuDebug->Append(MENU_TestAddLeft,     "Add Left");
        menuDebug->Append(MENU_TestAddBottom,   "Add Bottom");
        menuDebug->Append(MENU_TestAddRight,    "Add Right");
        menuDebug->AppendSeparator();
        menuDebug->Append(MENU_CreateDebugSetup,"Create Debug Setup");
        menuDebug->Append(MENU_CreateDebugTabSetup, "Create Debug Tab Setup");

        wxMenu* menuView = new wxMenu;
        this->menuClose_Destroy = menuView->AppendRadioItem(MENU_CloseMode_DestroyAll,      "Close As Destroy All");
        this->menuClose_Release = menuView->AppendRadioItem(MENU_Close_ReleaseAll,          "Close As Release All");
        this->menuClose_Close   = menuView->AppendRadioItem(MENU_Close_BlockingCloseAll,    "Close As Close All");

        wxMenu* menuCommands = new wxMenu;

        wxMenu* menuHelp = new wxMenu;
        menuHelp->Append(wxID_ABOUT);

        wxMenuBar* menuBar = new wxMenuBar;
        menuBar->Append(menuFile,       "&File");
        menuBar->Append(menuDebug,      "Debug");
        menuBar->Append(menuView,       "View");
        menuBar->Append(menuCommands,   "Commands");
        menuBar->Append(menuHelp,       "&Help");

        this->SetMenuBar(menuBar);

        this->CreateStatusBar();
        this->SetStatusText("Welcome to wxWidgets!");

        this->Bind(wxEVT_MENU, &WinInst::OnAbout, this, wxID_ABOUT);
        this->Bind(wxEVT_MENU, &WinInst::OnExit, this, wxID_EXIT);

        // SETUP SYSTEM MENU
        //////////////////////////////////////////////////
        HMENU sysMenu = (HMENU)::GetSystemMenu(this->GetHWND(), FALSE);
        if(sysMenu != nullptr)
        {
			// Counter to insert entries at the top. To append to the bottom use:
			// InsertMenu(..., -1, MF_BYPOSITION | MF_POPUP, ...,  TEXT("..."));
            int menuPosIns = 0;

			// Create, populate and add the "Manage All" submenu
			HMENU subMenuManip = ::CreatePopupMenu();
			if (subMenuManip != NULL)
			{
				// Add the submenu to the System Menu
				::InsertMenu(sysMenu, menuPosIns, MF_BYPOSITION | MF_POPUP, (UINT_PTR)subMenuManip, TEXT("Manage All"));
				++menuPosIns;
				//
				// Add submenu entries
				::InsertMenu(subMenuManip, -1, MF_BYPOSITION, MENU_ReleaseAll, TEXT("Release All"));
				::InsertMenu(subMenuManip, -1, MF_BYPOSITION, MENU_DetachAll, TEXT("Detach All All"));
				::InsertMenu(subMenuManip, -1, MF_BYPOSITION, MENU_CloseAll, TEXT("Close All"));
				::InsertMenu(subMenuManip, -1, MF_BYPOSITION, wxID_EXIT, TEXT("Force Close All"));
			}
        }
        this->RefreshCloseModeMenus();

        SetDefaultIcons(this);
	}

    WinInst::~WinInst()
    {
        this->DeleteAllSashes();
    }

    Node* WinInst::Dock(HWND hwnd, HANDLE process, Node* where, DockDir dir, int idx)
    {
        Node* ret = this->_innerDock(hwnd, process, where, dir, idx);
        assert(this->VALI());

        if(ret != nullptr)
            this->FlagDirty();
            
        return ret;
    }

    Node* WinInst::_innerDock(HWND hwnd, HANDLE process, Node* where, DockDir dir, int idx)
    {
        // TODO: Uncomment this when windows are supported
        //assert(this->hwndToNodes.find(hwnd) == this->hwndToNodes.end());

        if (this->root == nullptr)
        {
            this->root = Node::MakeWinNode(nullptr, 1.0f, hwnd, process);
            this->RegisterNode(this->root);
            this->Refresh(); // TODO: Remove and handle refresh correctly
            return this->root;
        }

		if (where->type == NodeType::Tabs)
		{
			if(dir == DockDir::Into)
            { 
			    Node* newWinNode = Node::MakeWinNode(where, 1.0f, hwnd, process);
			    where->children.push_back(newWinNode);
			    this->RegisterNode(newWinNode);
			    this->Refresh();
			    return newWinNode;
            }
		}

        if(where->type == NodeType::Window || where->type == NodeType::Tabs)
        {
            if(dir == DockDir::Into) // Tabs:Into already handled above
            {
                // If the window is in a tab, put the dropped thing into that tab.
                if(where->parent != nullptr && where->parent->type == NodeType::Tabs)
                    return Dock(hwnd, process, where->parent, DockDir::Into, -1);

                // Swap out the windows position for a tabbed node, and 
                // populate it.
                Node* tabsNode = new Node(NodeType::Tabs, nullptr, where->prop);
                where->prop = 1.0f;
                if(where->parent == nullptr)
                {
                    assert(this->root == where);
                    this->root = tabsNode;
                }
                else
                {
                    where->parent->SwapChild(where, tabsNode);
                }

                Node* newWinNode = Node::MakeWinNode(tabsNode, 1.0f, hwnd, process);
                //
                tabsNode->children.push_back(where);
                where->parent = tabsNode;
                tabsNode->children.push_back(newWinNode);
                newWinNode->parent = tabsNode;
                // The newest item goes on top for now, may change mind about this later.
                tabsNode->activeTab = 1;
                //
                this->RegisterNode(tabsNode);
                this->RegisterNode(newWinNode);
                return newWinNode;
            }
            // Else, we're doing a Top/Left/Bottom/Right - and we'll need to convert that
            // to a parent insertion.
            NodeType dockGrainType;
            bool lowerSide;
            bool grainStat = GetDockDirGrainStats(dir, dockGrainType, lowerSide);
            assert(grainStat);

            // Is `where` the root? 
            // Then it needs to be split into a grained container?
            if(root == where)
            {
                assert(where->parent == nullptr);

                Node* newGrainNode = new Node(dockGrainType, nullptr, 1.0f);
                this->root = newGrainNode;
                where->parent = newGrainNode;
                Node* newWinNode = Node::MakeWinNode(newGrainNode, 0.5f, hwnd, process);
                where->prop = 0.5f;

                if(lowerSide)
                {
                    newGrainNode->children.push_back(newWinNode);
                    newGrainNode->children.push_back(where);
                }
                else
                {
                    newGrainNode->children.push_back(where);
                    newGrainNode->children.push_back(newWinNode);
                }
                this->RegisterNode(newGrainNode);
                this->RegisterNode(newWinNode);
                return newWinNode;
            }

            assert(where->parent != nullptr);
            assert(where->parent->type == NodeType::SplitHoriz || where->parent->type == NodeType::SplitVert);
            if (where->parent->type == NodeType::SplitHoriz)
            {
                if(dockGrainType == NodeType::SplitHoriz)
                {
                    Node* newWinNode = Node::MakeWinNode(where->parent, where->parent->AvgChildProp(), hwnd, process);
                    where->parent->InsertAsChild(newWinNode, where, lowerSide ? Insertion::Before : Insertion::After);
                    where->parent->NormalizeChildProportions();
                    RegisterNode(newWinNode);
                    return newWinNode;
                }
                else // Make a vertical split in the horizontal parent
                {
                    Node* oldParent = where->parent;
                    Node* newWinNode = Node::MakeWinNode(nullptr, 0.5f, hwnd, process);
                    where->prop = 0.5f;
                    Node* newVertInner = 
                        Node::MakeSplitContainerNode(
                            NodeType::SplitVert, 
                            where->parent,
                            where->prop, 
                            where,
                            newWinNode,
                            lowerSide);

                    oldParent->SwapChild(where, newVertInner, false);
                    where->prop = 0.5f;
                    this->RegisterNode(newVertInner);
                    this->RegisterNode(newWinNode);
                    return newWinNode;
                }
            }
            else //if(where->parent->type == Node::Type::SplitVert)
            {
                // The inverse of the if statement
                if(dockGrainType == NodeType::SplitVert)
                {
                    Node* newWinNode = Node::MakeWinNode(where->parent, where->parent->AvgChildProp(), hwnd, process);
                    where->parent->InsertAsChild(newWinNode, where, lowerSide ? Insertion::Before : Insertion::After);
                    where->parent->NormalizeChildProportions();
                    this->RegisterNode(newWinNode);
                    return newWinNode;
                }
                else // Make a horizontal split in the vertical parent
                {
                    Node* oldParent = where->parent;
                    Node* newWinNode = Node::MakeWinNode(nullptr, 0.5f, hwnd, process);
                    Node* newHorizInner = 
                        Node::MakeSplitContainerNode(
                        NodeType::SplitHoriz,
                            where->parent,
                            where->prop,
                            where,
                            newWinNode,
                            lowerSide);

                    where->prop = 0.5f;
                    oldParent->SwapChild(where, newHorizInner, false);
                    this->RegisterNode(newHorizInner);
                    this->RegisterNode(newWinNode);
                    return newWinNode;
                }
            }
        }

        // TODO: This should be used more often through recursion on some of the cases above.
		if (dir == DockDir::Into)
		{
			assert(where->type != NodeType::Window);
            assert(where->children.size() > 1);
			assert(idx >= 0);

			Node* newWinNode = Node::MakeWinNode(nullptr, 1.0f, hwnd, process);
			where->InsertAsChild(newWinNode, idx);
            if(where->type != NodeType::Tabs)
            {
                newWinNode->prop = 1.0f / (where->children.size() - 1);
                where->NormalizeChildProportions();
            }
			this->RegisterNode(newWinNode);
			return newWinNode;
		}

        NodeType dockGrainType = {};
        bool lowerSide = {};
		bool grainStat = GetDockDirGrainStats(dir, dockGrainType, lowerSide);
        assert(grainStat);

        NodeType typeToUse = {};
        NodeType oppositeType = {};
        if(where->type == NodeType::SplitHoriz)
        {
            typeToUse = NodeType::SplitHoriz;
            oppositeType = NodeType::SplitVert;
        }
        else
        {
            assert(where->type == NodeType::SplitVert);
            typeToUse = NodeType::SplitVert;
            oppositeType = NodeType::SplitHoriz;
        }

        // Adding to end of the grained container
        if(dockGrainType == typeToUse)
        {
            size_t insertIndex = lowerSide ? 0 : where->children.size();
            Node* newWinNode = Node::MakeWinNode(nullptr, where->AvgChildProp(), hwnd, process);
            where->InsertAsChild(newWinNode, insertIndex);
            where->NormalizeChildProportions();
            this->RegisterNode(newWinNode);
            return newWinNode;
        }
        // Root vertical split into horizontal, or vice versa
        if(where == this->root)
        {
            assert(dockGrainType == oppositeType);
            Node* newWinNode = Node::MakeWinNode(nullptr, 0.5f, hwnd, process);
            Node* newVertRoot = 
                Node::MakeSplitContainerNode(
                    oppositeType, 
                    nullptr,
                    1.0f,
                    where,
                    newWinNode,
                    lowerSide);

            where->prop = 0.5f;
            this->root = newVertRoot;
            this->RegisterNode(newVertRoot);
            this->RegisterNode(newWinNode);
            return newWinNode;
        }
        // Non-root case of splitting vertical into horizontal, or vice versa
        {
            assert(dockGrainType == oppositeType);
            assert(where->parent != nullptr);
            assert(where->parent->type == typeToUse);
            Node* newWinNode = Node::MakeWinNode(nullptr, where->parent->AvgChildProp(), hwnd, process);
            int idx = where->parent->GetChildIndex(where);
            assert(idx != -1);
            if(!lowerSide)
                ++idx;
            where->parent->InsertAsChild(newWinNode, idx);
            where->parent->NormalizeChildProportions();
            this->RegisterNode(newWinNode);
            return newWinNode;
        }   
    }

    Node* WinInst::Dock(const wchar_t* exePath, Node* where, DockDir dir, int idx)
    {
		SHELLEXECUTEINFO sei = { sizeof(sei) };
		sei.fMask = SEE_MASK_NOCLOSEPROCESS;
		sei.lpFile = exePath;
		sei.nShow = SW_SHOWNORMAL;

		if (!::ShellExecuteEx(&sei))
			return nullptr;

        Node* ret = Dock((HWND)NULL, sei.hProcess, where, dir, idx);
        if(ret == nullptr)
        {
            //assert(false, "Could not Dock booted EXE path");
            ::TerminateProcess(sei.hProcess, 1);
            ::CloseHandle(sei.hProcess);
        }
        return ret;
    }

    void WinInst::RefreshLayout()
    {
        bool layout     = (this->dirtyFlags & DirtyFlags::Layout) != 0;
        bool sashes     = (this->dirtyFlags & DirtyFlags::Sashes) != 0;
        bool freshMax   = (this->dirtyFlags & DirtyFlags::FreshMaxChange) != 0;
        this->RefreshLayout(layout, sashes, freshMax);
    }

    void WinInst::RefreshLayout(bool layout, bool sashes, bool freshMax)
    {
        // Regardless of what happens, a whole refresh of any part is
        // assumed (for now) to make sash dragging unsafe
        if(layout || sashes)
        { 
		    this->draggedSash = nullptr;
            this->DeleteAllSashes();
        }

        if(freshMax)
        {
            if(this->maximizedWinNode == nullptr)
            {
                // No maximized window node
                for(Node* n : this->nodes)
                {
                    if (n->type == NodeType::Window)
                    { 
                        n->titlebar->Show();
                        if(n->winHandle != nullptr)
                            ::ShowWindow(n->winHandle, SW_SHOW);
                    }
                    else if(n->type == NodeType::Tabs)
                    {
                        // TODO: VALI for tabs
                        n->tabsBar->Show();
                    }

                }
            }
            else
            {
                this->SetDragCursor(DragCursor::None);
                // A maximized window node
                for(Node* n : this->nodes)
                {
                    if(n->type == NodeType::Window)
                    {
                        if(n == this->maximizedWinNode)
                        {
						    n->titlebar->Show();
                            if(n->winHandle != nullptr)
                                ::ShowWindow(n->winHandle, SW_SHOW);
                        }
                        else
                        {
						    n->titlebar->Hide();
                            if(n->winHandle != nullptr)
                                ::ShowWindow(n->winHandle, SW_HIDE);
                        }
                    }
                    else if(n->type == NodeType::Tabs)
                    {
                        // TODO: VALI for tabs
                        n->tabsBar->Hide();
                    }

                }
            }
        }
        this->dirtyFlags &= ~this->dirtyFlags & DirtyFlags::FreshMaxChange;

        if(layout)
        { 
		    const Context& ctx = this->appOwner->GetLayoutContext();
		    wxPoint rootPos(0, 0);
		    wxSize rootSize = GetClientSize();
		    rootPos.x += ctx.horizWinPadding;
		    rootPos.y += ctx.vertWinPadding;
		    rootSize.x -= ctx.horizWinPadding * 2;
		    rootSize.y -= ctx.vertWinPadding * 2;

            if(this->HasMaximizedWindow())
            {
                // TODO: VALI is a window
                this->maximizedWinNode->ApplyNodeLayout(rootPos, rootSize, ctx);
            }
            else
            {
		        if (this->root != nullptr)
		        {
			        this->root->CalculateMinSizeRecursive(ctx);
			        this->root->CacheCalculatedSizeRecursive(rootPos, rootSize, ctx);
			        root->ApplyCachedlayout(ctx);
		        }
            }
            this->dirtyFlags &= ~this->dirtyFlags & DirtyFlags::Layout;
        }

        if(sashes)
        {
            if(this->root != nullptr)
            {
                this->RegenerateSashes(this->root, this->sashes);
            }
            this->dirtyFlags &= ~this->dirtyFlags & DirtyFlags::Sashes;
        }
    }

    void WinInst::RegenerateSashes(Node* node, std::map<SashKey, Sash*>& sashes)
    {
        // TODO: Just touch this->sashes directly?
        if(
            node->type != NodeType::SplitHoriz && 
            node->type != NodeType::SplitVert)
        {
            return;
        }

        size_t itCt = node->children.size() - 1;
        for(size_t i = 0; i < itCt; ++i)
        {
            Node* a = node->children[i + 0];
            Node* b = node->children[i + 1];

            SashKey key(a, b);
            if(node->type == NodeType::SplitHoriz)
            {
                int aRight = a->pos.x + a->size.x;
                assert(!sashes.contains(key));
                sashes[key] = new Sash(node, i, aRight, a->pos.y, b->pos.x - aRight, a->size.y);
            }
            else
            { 
                int aBot = a->pos.y + a->size.y;
                assert(!sashes.contains(key));
                sashes[key] = new Sash(node, i, a->pos.x, aBot, a->size.x, b->pos.y - aBot);
            }
        }

        for(Node* child : node->children)
            RegenerateSashes(child, sashes);
    }

    void WinInst::DeleteAllSashes()
    {
        this->draggedSash = nullptr;
        for(auto kvp : this->sashes)
        {
            delete kvp.second;
        }
        this->sashes.clear();
    }

	bool WinInst::UpdateSash(Node* a, Node* b)
    {
        SashKey key(a, b);
        auto it = this->sashes.find(key);
        if(it == this->sashes.end())
            return false;

        Sash* s = it->second;
        

        if(s->container->type == NodeType::SplitHoriz)
        { 
			Node* left  = s->container->children[s->index + 0];
			Node* right = s->container->children[s->index + 1];

            int tlEnd = left->pos.x + left->size.x;
            s->pos = wxPoint(tlEnd, left->pos.y); 
            s->size = wxSize(right->pos.x - tlEnd, left->size.y);
        }
        else
        {
            assert(s->container->type == NodeType::SplitVert);

			Node* top = s->container->children[s->index + 0]; // top or left
			Node* bot = s->container->children[s->index + 1]; // bottom or right

			int tlEnd = top->pos.y + top->size.y;
			s->pos = wxPoint(top->pos.x, tlEnd);
			s->size = wxSize(top->size.x, bot->pos.y - tlEnd);
        }
        return true;
    }

	void WinInst::UpdateContainerSashes(Node* containerNode, bool recurse)
    {
        if(containerNode->type != NodeType::SplitHoriz &&
            containerNode->type != NodeType::SplitVert)
        {
            return;
        }

        const size_t childCt = containerNode->children.size();
        for(int i = 0; i < childCt - 1; ++i)
        {
            Node* a = containerNode->children[i + 0];
            Node* b = containerNode->children[i + 1];
            bool result = UpdateSash(a, b);
            assert(result);
        }

        if(recurse)
        {
            for(Node* child : containerNode->children)
                this->UpdateContainerSashes(child, true);
        }
    }

    void WinInst::SetDragCursor(DragCursor cursor)
    {
        switch(cursor)
        {
        case DragCursor::Horiz:
            this->SetCursor(wxCursor(wxCURSOR_SIZEWE));
            break;
        case DragCursor::Vert:
            this->SetCursor(wxCursor(wxCURSOR_SIZENS));
            break;
        case DragCursor::None:
            this->SetCursor(wxNullCursor);
            break;
        default:
            assert(!"Unknown drag cursor type");
            break;
        }
    }

    void WinInst::RefreshCloseModeMenus()
    {
        this->menuClose_Destroy->Check( this->closeMode == CloseMode::DestroyAll);
        this->menuClose_Release->Check( this->closeMode == CloseMode::ReleaseAll);
        this->menuClose_Close->Check(   this->closeMode == CloseMode::BlockingCloseAll);
    }

	void WinInst::ReleaseAll()
    {
		std::vector<Node*> winNodes;
		std::vector<HWND> releasedHwnds;
		for (Node* n : this->nodes)
		{
			if (n->type == NodeType::Window)
			{
				winNodes.push_back(n);

				if (n->winHandle != nullptr)
					releasedHwnds.push_back(n->winHandle);
			}
		}
		for (Node* n : winNodes)
			this->ReleaseManagedWindow(n);

		this->FlagDirty();

		wxPoint winPos = this->GetPosition();
		const int cascadex = 10; //TODO: Store as variables in context
		const int cascadey = 10; //TODO: Store as variables in context
		for (HWND hwnd : releasedHwnds)
		{
			winPos.x += cascadex;
			winPos.y += cascadey;
			::SetWindowPos(hwnd, HWND_TOP, winPos.x, winPos.y, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
		}
        // assert(No processes)
        // assert(No windows)
		assert(this->VALI());
    }

	void WinInst::SendCloseAndForgetAll()
    {
		std::vector<Node*> winNodes;
		for (Node* n : this->nodes)
		{
			if (n->type == NodeType::Window)
				winNodes.push_back(n);
		}
		for (Node* n : winNodes)
			this->CloseAndForgetManagedWindow(n);

		this->FlagDirty();

		assert(this->VALI());
    }

	void WinInst::BlockingCloseAll()
    {
		std::vector<Node*> winNodeProcesses;
        std::vector<HWND> hwnds;
		for (Node* n : this->nodes)
		{
			if (n->type == NodeType::Window)
            {
                if(n->process != nullptr)
                    winNodeProcesses.push_back(n);
                else
                {
                    assert(n->winHandle != nullptr);
                    hwnds.push_back(n->winHandle);
                }
            }
		}
		for(Node* n : winNodeProcesses)
			this->CloseManagedWindow(n);

        for(HWND hwnd : hwnds)
            SendMessage(hwnd, WM_CLOSE, 0, 0);

		this->FlagDirty();

		assert(this->VALI());
    }

    void WinInst::NonblockingCloseAll()
    {
		std::vector<Node*> winNodes;
		for (Node* n : this->nodes)
		{
			if (n->type == NodeType::Window)
				winNodes.push_back(n);
		}
		for (Node* n : winNodes)
			this->CloseManagedWindow(n);

		this->FlagDirty();

		assert(this->VALI());
    }

    void WinInst::DetachAll()
    {
		std::vector<Node*> winNodes;
		for (Node* n : this->nodes)
		{
			if (n->type == NodeType::Window)
				winNodes.push_back(n);
		}
		// TODO: Cascaded layout
		for (Node* n : winNodes)
			this->DetachManagedWindow(n);

		this->FlagDirty();

		// assert(No processes)
		// assert(No windows)
		assert(this->VALI());
    }

    void WinInst::FlagDirty(bool layout, bool sashes, bool freshMax)
    {
        if(layout)
            this->dirtyFlags |= DirtyFlags::Layout;

        if(sashes)
            this->dirtyFlags |= DirtyFlags::Sashes;

        if(freshMax)
            this->dirtyFlags |= DirtyFlags::FreshMaxChange;

        if(!this->refreshLayoutTimer.IsRunning())
            this->refreshLayoutTimer.Start(0, true);
    }

    const Context& WinInst::GetAppContext()
    {
        return this->appOwner->GetLayoutContext();
    }

    App* WinInst::GetAppOwner()
    {
        return this->appOwner;
    }

    bool WinInst::HasNode(Node* node)
    {
        return this->nodes.find(node) != this->nodes.end();
    }

    bool WinInst::IsManagingHwnd(HWND hwnd)
    {
        return this->hwndToNodes.find(hwnd) != this->hwndToNodes.end();
    }

    Node* WinInst::FindNodeForHWND(HWND hwnd)
    {
        auto find = hwndToNodes.find(hwnd);
        return find != hwndToNodes.end() ? (*find).second : nullptr;
    }

    void WinInst::ApplyCachedLayout(Node* node)
    {
        if(node->winHandle != NULL)
        {
            ::SetWindowPos(
                node->winHandle, 
                NULL, 
                node->pos.x, 
                node->pos.y, 
                node->size.x, 
                node->size.y, 
                0);
        }
    }

    Sash* WinInst::ProcessMousePointForSash(const wxPoint& targetPos)
    {
		for (auto& kvp : this->sashes)
		{
            Sash* s = kvp.second;

			bool assertTest = 
                s->container->type == NodeType::SplitHoriz ||
				s->container->type == NodeType::SplitVert;

            if(assertTest == false)
            {
                int y = 5;
            }
			assert(assertTest);

			//assert(
			//	s.container->type == NodeType::SplitHoriz ||
			//	s.container->type == NodeType::SplitVert);

			if (IsPointInRect(targetPos, s->pos, s->size))
			{
				if (s->container->type == NodeType::SplitHoriz)
				{
                    this->SetDragCursor(DragCursor::Horiz);
					return s;
				}
				else if (s->container->type == NodeType::SplitVert)
				{
                    this->SetDragCursor(DragCursor::Vert);
					return s;
				}
			}
		}	
        this->SetDragCursor(DragCursor::None);
        return nullptr;
    }

    void WinInst::StopDraggingSash()
    {
		this->dragState = MouseDragState::Void;
		this->draggedSash = nullptr;
		this->SetDragCursor(DragCursor::None);
    }

    bool WinInst::RemoveWinNode(Node* node, bool deleteNode)
    {
        assert(node->type == NodeType::Window);

        if(node == this->maximizedWinNode)
            this->SetWinNodeMaximized(nullptr);

        // Erase mapping records
		if(node->winHandle != nullptr)
		{
            int rmCt = this->hwndToNodes.erase(node->winHandle);
            assert(rmCt == 1);
		}
        if(node->process != nullptr)
        {
            int rmCt = this->processToNodes.erase(node->process);
            assert(rmCt == 1);
        }

        // Do removal if root - means it's the only node, which is the
        // simplest case.
        if(node == this->root)
        {
            if(deleteNode)
                delete node;

            this->root = nullptr;
            this->nodes.erase(node);
            this->FlagDirty(true, true);
            // TODO: Set layout dirty

            assert(this->VALI());
            return true;
        }

        // Do removal if in a complex hierarchy.
        Node* oldParent = node->parent;
        assert(oldParent != nullptr);
        auto childIt = std::find(oldParent->children.begin(), oldParent->children.end(), node);
        assert(childIt != oldParent->children.end());
        oldParent->children.erase(childIt);
        if(deleteNode)
            delete node;

        this->nodes.erase(node);
        FlagDirty(true, true);

        // Parent might be deleted if there was only 1 child left - but 
        // easier to updated contents left here than remembering until later.
        switch(oldParent->type)
        {
        case NodeType::Tabs:
            oldParent->ClampTabId();
            break;
        case NodeType::SplitHoriz:
        case NodeType::SplitVert:
            oldParent->NormalizeChildProportions();
            break;
        }

        // Fixup hierarchy if needed
        this->CleanContainerNodeIfEmpty(oldParent);
        this->draggedSash = nullptr;

        assert(this->VALI());
        return true;
    }

    void WinInst::CleanContainerNodeIfEmpty(Node* containerNode)
    {
        assert(
            containerNode->type == NodeType::Tabs ||
            containerNode->type == NodeType::SplitHoriz ||
            containerNode->type == NodeType::SplitVert);

        // Sanity checks - but shouldn't really happen, or else
        // would be replace by the "container.children.size == 1"
        // case before this.
        if(containerNode->children.size() == 0)
        {
            if(containerNode == this->root)
            {
                delete containerNode;
                this->root = nullptr;
                this->nodes.erase(containerNode);
            }
            else
            { 
                Node* oldParent = containerNode->parent;
                assert(oldParent != nullptr);
			    auto childIt = std::find(oldParent->children.begin(), oldParent->children.end(), containerNode);
			    assert(childIt != oldParent->children.end());
			    oldParent->children.erase(childIt);
			    delete containerNode;
			    this->nodes.erase(containerNode);

                CleanContainerNodeIfEmpty(oldParent);
            }
            assert(this->VALI());
            return;
        }

        // All the children have been deleted except the last. 
        // At this point, the container has no more use and should
        // just be replaced by the child - (which should always be a 
        // Window type if things were deleted correctly).
        if(containerNode->children.size() == 1)
        {
            Node* lastWinLeft = containerNode->children[0];

            if(containerNode == this->root)
            {
                containerNode->children.clear();
                this->root = lastWinLeft;
                lastWinLeft->prop = 1.0f;
                delete containerNode;
                this->nodes.erase(containerNode);
                lastWinLeft->parent = nullptr;
            }
            else
            { 
			    Node* oldParent = containerNode->parent;
                assert(oldParent != nullptr);
			    auto childIt = std::find(oldParent->children.begin(), oldParent->children.end(), containerNode);
			    assert(childIt != oldParent->children.end());
			    *childIt = lastWinLeft;
                lastWinLeft->prop = containerNode->prop;
                lastWinLeft->parent = oldParent;
                containerNode->children.clear();
			    delete containerNode;
			    this->nodes.erase(containerNode);
                this->CleanContainerNodeIfEmpty(oldParent);
            }
            assert(this->VALI());
            return;
        }

        if(containerNode->type == NodeType::SplitHoriz || containerNode->type == NodeType::SplitVert)
        {
            NodeType splitType = containerNode->type;
            for(size_t i = 0; i < containerNode->children.size(); )
            {
                if(containerNode->children[i]->type != splitType)
                {
                    ++i;
                    continue;
                }
                //assert(containerNode->children[i]->type == splitType);
                Node* collidingTypeChild = containerNode->children[i];
                int transferCt = collidingTypeChild->children.size();

                for(Node* childToTransfer : collidingTypeChild->children)
                    childToTransfer->parent = containerNode;

                containerNode->children.erase(containerNode->children.begin() + i);

                containerNode->children.insert(
                    containerNode->children.begin() + i,
                    collidingTypeChild->children.begin(),
                    collidingTypeChild->children.end());

                // This isn't quite correct, but we'll let this approximation pass for now.
                // Biggest issue is we don't account for the slightly less space because
                // of the extra sashes for what used to be two nodes.
                for(Node* insertedNode : collidingTypeChild->children)
                    insertedNode->prop *= collidingTypeChild->prop;
                containerNode->NormalizeChildProportions();

                i += transferCt;
                
                this->nodes.erase(collidingTypeChild);
                collidingTypeChild->children.clear();
                delete collidingTypeChild;
            }
            assert(this->VALI());
            return; // Not needed, but to match the conventions of returning everywhere else.
        }
    }

    bool WinInst::MSWTranslateMessage(WXMSG *msg)
    {
		if (msg->message == WM_SYSCOMMAND)
		{
			switch (msg->wParam)
			{
				// For every wparam documented for WM_SYSCOMMAND, we leave alone and 
				// let the default implementation do whatever with it.
				// https://learn.microsoft.com/en-us/windows/win32/menurc/wm-syscommand
			case SC_CLOSE:
			case SC_CONTEXTHELP:
			case SC_DEFAULT:
			case SC_HOTKEY:
			case SC_HSCROLL:
				// case SCF_ISSECURE: 
				//  Except this, t'is a troublemaker!
			case SC_KEYMENU:
			case SC_MAXIMIZE:
			case SC_MINIMIZE:
			case SC_MONITORPOWER:
			case SC_MOUSEMENU:
			case SC_MOVE:
			case SC_NEXTWINDOW:
			case SC_PREVWINDOW:
			case SC_RESTORE:
			case SC_SCREENSAVE:
			case SC_SIZE:
			case SC_TASKLIST:
			case SC_VSCROLL:
				break;

			default:
				// Everything else, we're claiming as ID space we're processing ourselves.
				// We convert to a menu command
				wxCommandEvent evt(wxEVT_MENU, msg->wParam);
				this->ProcessEvent(evt);
				return true;
			}

		} // if (msg->message == WM_SYSCOMMAND)

		  // Delegate everything we didn't handle to parent implementation.
		return wxFrame::MSWTranslateMessage(msg);
    }

	void WinInst::CloseManagedWindow(Node* winNode)
    {
        assert(winNode->type == NodeType::Window);

        if(winNode->winHandle != nullptr)
        {
            // It may be a bit-rigmarolley to get the handle, only for that
            // overload of CloseManagedWindow() to re-find the Node, but
            // the overhead is negligible, and it also forces to check the
            // validity of things that are more appropriately validated in
            // that overload.
            this->CloseManagedWindow(winNode->winHandle);
        }
        else if(winNode->process != nullptr)
        {
            // Same notes as above comment.
            this->CancelManagedWindow(winNode->process);
        }
        else
        {
            ASSERT_REACHED_ILLEGAL_POINT();
        }
    }

    void WinInst::CloseAndForgetManagedWindow(Node* winNode)
    {
		assert(winNode->type == NodeType::Window);

		if (winNode->winHandle != nullptr)
		{
            ::SendMessage(winNode->winHandle , WM_CLOSE, 0, 0);
            this->RemoveWinNode(winNode);
		}
		else if (winNode->process != nullptr)
		{
			// Same notes as above comment.
			this->CancelManagedWindow(winNode->process);
		}
		else
		{
			ASSERT_REACHED_ILLEGAL_POINT();
		}
    }

    void WinInst::DestroyManagedWindow(Node* winNode)
    {
        if(winNode->winHandle != nullptr)
        {
            HWND oldHwnd = winNode->winHandle;

            this->RemoveWinNode(winNode);
            assert(this->hwndToNodes.find(oldHwnd) == this->hwndToNodes.end());

            DWORD processId;
            DWORD threadId = ::GetWindowThreadProcessId(oldHwnd, &processId);
            if(threadId != 0)
            {
                assert(processId != 0);
				HANDLE hProcess = ::OpenProcess(PROCESS_TERMINATE, FALSE, processId);
				if (hProcess == NULL) 
                {
                    // !TODO: This the correct error output we're going with?
					std::cerr << "Failed to open process or process not found. Error code: " << GetLastError() << std::endl;
					return;
				}
                // This is overkill in that it destroys the entire process, even if the
                // window is a child window of a bigger app (e.g., killing all of Steam
                // if just a chat window was being managed).
				::TerminateProcess(hProcess, 0);
				::CloseHandle(hProcess);
            }
        }
        else if(winNode->process != nullptr)
        {
            this->CancelManagedWindow(winNode->process);
        }
        else
        {
            ASSERT_REACHED_ILLEGAL_POINT();
        }
    }

	void WinInst::CloseManagedWindow(HWND hwnd)
    {
        auto it = this->hwndToNodes.find(hwnd);
        assert(it != this->hwndToNodes.end());

		// Send a message for it to close, and we'll get rid of the window
		// node when we intercept the EVENT_OBJECT_DESTROY hook event.
		::PostMessage(hwnd, WM_CLOSE, 0, 0);
    }

	void WinInst::CancelManagedWindow(HANDLE process)
    {
        auto it = this->processToNodes.find(process);
        assert(it != this->processToNodes.end());

		::TerminateProcess(process, 0);
		this->RemoveWinNode(it->second.node);

        assert(this->processToNodes.find(process) == this->processToNodes.end());
    }

    WinInst* WinInst::DetachManagedWindow(Node* winNode)
    {
        this->RemoveWinNode(winNode, false);
        winNode->CleanSupportingUI();

        WinInst* newWinInst = this->appOwner->SpawnInst();
        newWinInst->InjectExistingWinNodeAsRoot(winNode);
        return newWinInst;
    }

    void WinInst::ReleaseManagedWindow(Node* winNode)
    {
        assert(winNode->type == NodeType::Window);

        HWND oldHwnd = winNode->winHandle;
        LONG origStyle = winNode->origStyle;
        LONG origExStyle = winNode->origExStyle;
        wxSize origSize = winNode->origSize;

        this->RemoveWinNode(winNode, true);

        if(oldHwnd != nullptr)
        { 
            ::SetParent(oldHwnd, nullptr);
            // TODO: Make sure the filter stuff out like "Minimized" and "Maximized"
		    ::SetWindowLong(oldHwnd, GWL_STYLE, origStyle);
		    ::SetWindowLong(oldHwnd, GWL_EXSTYLE, origExStyle);

            wxPoint mousePos = wxGetMousePosition();

            ::SetWindowPos(
                oldHwnd, 
                HWND_TOP, 
                mousePos.x, 
                mousePos.y, 
                origSize.x, 
                origSize.y,
                SWP_SHOWWINDOW);
        }
    }

    void WinInst::InjectExistingWinNodeAsRoot(Node* winNode)
    {
        // Assert empty
        assert(this->root == nullptr);
        assert(this->processToNodes.size() == 0);
        assert(this->hwndToNodes.size() == 0);
        // winNode validity
        assert(winNode->type == NodeType::Window);
        // Check whoever handed us this node didn't leak stuff behind
        assert(winNode->titlebar == nullptr);
        assert(winNode->tabsBar == nullptr);

		winNode->parent = nullptr;
        this->root = winNode;
		this->RegisterNode(winNode, false);
        this->FlagDirty(true, true);

        assert(this->VALI());
    }

    OverlapDropDst WinInst::GetPreviewDropOverlayDst(const wxPoint& pt)
    {
        const Context& ctx = this->GetAppContext();
        if(this->HasMaximizedWindow())
        {
            assert(this->root != nullptr);
			wxRect rclient = this->GetClientRect();
			return OverlapDropDst(
				this,
				this->maximizedWinNode,
				DockDir::Into,
				rclient);
        }

        const int dropRad = ctx.intoDropPrevSize / 2;
        if(this->root == nullptr)
        {
            wxRect rclient = this->GetClientRect();
            return OverlapDropDst(
                this, 
                nullptr, 
                DockDir::Into, 
                rclient);
        }
        std::queue<Node*> toSearch;
        toSearch.push(this->root);
        Node* toDrop = nullptr;
        while(!toSearch.empty())
        {
            Node* n = toSearch.front();
            toSearch.pop();

            wxRect rgn(n->pos, n->size);
            if(!rgn.Contains(pt))
                continue;
            
            if(n->type == NodeType::Window || n->type == NodeType::Tabs)
            {
                toDrop = n;
                break;
            }
            for(Node* c : n->children)
                toSearch.push(c);
        }

        if(toDrop != nullptr)    
        {
            enum RectCmp
            {
                Center,
                Top,
                Left,
                Bottom,
                Right
            };
            // Match distance from dockable features to an array where
            // it's the RectCmp-th entry.
            std::vector<float> canDists; // Candidates
            wxPoint center = wxPoint(toDrop->pos.x + toDrop->size.x / 2, toDrop->pos.y + toDrop->size.y / 2);
            wxPoint cenOffs = (center - pt);
            canDists.push_back(sqrt(cenOffs.x * cenOffs.x + cenOffs.y * cenOffs.y));
            canDists.push_back(pt.y - toDrop->pos.y);
            canDists.push_back(pt.x - toDrop->pos.x);
            canDists.push_back(toDrop->pos.y + toDrop->size.y - pt.y);
            canDists.push_back(toDrop->pos.x + toDrop->size.x - pt.x);
            // Find closest feature to mouse.
            int lowestIdx = 0;
            float lowest = canDists[0];
            for(int i = 0; i < canDists.size(); ++i)
            {
                if(canDists[i] < lowest)
                {
                    lowestIdx = i;
                    lowest = canDists[i];
                }
            }
            // Process
            switch(lowestIdx)
            {
            case RectCmp::Center:
				return OverlapDropDst(
					this,
                    toDrop,
					DockDir::Into,
					wxRect(
						toDrop->pos.x + (toDrop->size.x / 2) - dropRad,
						toDrop->pos.y + (toDrop->size.y / 2) - dropRad,
						ctx.intoDropPrevSize,
						ctx.intoDropPrevSize));
                break;
            case RectCmp::Top:
				return OverlapDropDst(
					this,
					toDrop,
					DockDir::Top,
					wxRect(toDrop->pos.x, toDrop->pos.y, toDrop->size.x, ctx.barDropPreviewWidth)); 
                break;
            case RectCmp::Left:
				return OverlapDropDst(
					this,
					toDrop,
					DockDir::Left,
					wxRect(toDrop->pos.x, toDrop->pos.y, ctx.barDropPreviewWidth, toDrop->size.y));
            case RectCmp::Bottom:
				return OverlapDropDst(
					this,
					toDrop,
					DockDir::Bottom,
					wxRect(toDrop->pos.x, toDrop->pos.y + toDrop->size.y - ctx.barDropPreviewWidth, toDrop->size.x, ctx.barDropPreviewWidth));
            case RectCmp::Right:
				return OverlapDropDst(
					this,
					toDrop,
					DockDir::Right,
					wxRect(toDrop->pos.x + toDrop->size.x - ctx.barDropPreviewWidth, toDrop->pos.y, ctx.barDropPreviewWidth, toDrop->size.y));

                // TODO: Support docking into multiple levels
            }
        }

        return OverlapDropDst::Inactive();
    }

    void WinInst::SetWinNodeMaximized(Node* winNode)
    {
        if(this->maximizedWinNode == winNode)
        {
            this->maximizedWinNode = nullptr;
            this->FlagDirty(true, true, true);
            return;
        }

        this->maximizedWinNode = winNode;
        this->FlagDirty(true, true, true);
        return;
    }

	void WinInst::DirectDestroy()
	{
		this->directDestroy = true;
		this->Close(true);
	}

    bool WinInst::IsEmptyLayout()
    {
        if(this->root == nullptr)
        {
            assert(this->hwndToNodes.size() == 0 && this->processToNodes.size() == 0);
            return true;
        }

        assert(this->hwndToNodes.size() > 0 || this->processToNodes.size() > 0);
        return false;
    }

    void WinInst::RegisterNode(Node* node, bool cacheOrigWinProperties)
    {
        assert(!nodes.contains(node));
        nodes.insert(node);

        if(node->type == NodeType::Window)
        {
            //assert(node->winHandle != NULL);
            //assert(!hwndToNodes.contains(node->winHandle));
            node->titlebar = new SubTitlebar(this, node);

            assert((node->winHandle != nullptr) != (node->process != nullptr));

            if(node->winHandle != nullptr)
            { 
                if(cacheOrigWinProperties)
                    this->CacheOrigHWNDProperties(node);

                this->ReparentNodeHWNDIntoLayout(node);
                this->hwndToNodes[node->winHandle] = node;
            }
            else if(node->process != nullptr)
            { 
                this->processToNodes[node->process] = AwaitingHWNDData(node, wxGetUTCTimeMillis());

				if (!this->awaitGUITimer.IsRunning())
				{
					const Context& ctx = this->appOwner->GetLayoutContext();
					this->awaitGUITimer.Start(ctx.millisecondsAwaitGUI, true);
				}
            }

			if (this->HasMaximizedWindow())
			{
                node->titlebar->Show(false);
				if (node->winHandle != nullptr)
					::ShowWindow(node->winHandle, SW_HIDE);
			}
        }
        else if(node->type == NodeType::Tabs)
        {
            node->tabsBar = new SubTabs(this, node);

            if(this->HasMaximizedWindow())
                node->tabsBar->Hide();
        }
    }

    void WinInst::CacheOrigHWNDProperties(Node* node)
    {
        assert(node->VALI_IsFilledWinNode());
        assert(node->cachedHWNDProperties == false);
		node->cachedHWNDProperties = true;
		
        // !TODO: Distribute some of this responsibility to Node
		RECT windowSize;
		::GetWindowRect(node->winHandle, &windowSize);
		node->origSize =
			wxSize(
				windowSize.right - windowSize.left,
				windowSize.bottom - windowSize.top);

		node->origStyle     = ::GetWindowLong(node->winHandle, GWL_STYLE);
		node->origExStyle   = ::GetWindowLong(node->winHandle, GWL_EXSTYLE);

        assert(!this->hwndToNodes.contains(node->winHandle));
        this->hwndToNodes[node->winHandle] = node;
    }

    void WinInst::ReparentNodeHWNDIntoLayout(Node* node)
    {
        assert(node->VALI_IsFilledWinNode());

		::SetParent(node->winHandle, this->GetHWND());
		//
		LONG newStyle = node->origStyle;
		newStyle &= ~(WS_TILEDWINDOW | WS_OVERLAPPEDWINDOW | WS_POPUPWINDOW | WS_MINIMIZE | WS_MAXIMIZE | WS_DLGFRAME);
		newStyle |= WS_SYSMENU;
		//
		LONG newExStyle = node->origExStyle;
		newExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE);
		//
		::SetWindowLong(node->winHandle, GWL_STYLE, newStyle);
		::SetWindowLong(node->winHandle, GWL_EXSTYLE, newExStyle);
    }

	void WinInst::OnWinEvent_TitleChanged(HWND hwnd)
    {
		Node* winNode = this->FindNodeForHWND(hwnd);
        assert(winNode != nullptr);
		assert(winNode->VALI_IsFilledWinNode());
        assert(winNode->winHandle == hwnd);

        // !TODO: re-cache name and refresh display
		winNode->titlebar->Refresh();

        if(winNode->parent != nullptr && winNode->parent->type == NodeType::Tabs)
        {
            winNode->parent->tabsBar->Refresh();
        }
    }

	void WinInst::OnWinEvent_WindowClosed(HWND hwnd)
    {
        Node* winNode = this->FindNodeForHWND(hwnd);
        assert(winNode != nullptr);
        assert(winNode->VALI_IsFilledWinNode());
        assert(winNode->winHandle == hwnd);

        this->RemoveWinNode(winNode);
    }

    void WinInst::OnClose(wxCloseEvent & event)
    {
        if(!directDestroy)
        {
            switch(this->closeMode)
            {
            case CloseMode::ReleaseAll:
                this->ReleaseAll();
                break;
            case CloseMode::DestroyAll:
                // We don't actually "destroy", we just close out its Node (but send a 
                // "courtesy close" message) and then just let it die with the window
                // once we destroy ourselves.
                this->SendCloseAndForgetAll();
                break;
            case CloseMode::BlockingCloseAll:
                if(this->root != nullptr)
                {
                    this->BlockingCloseAll();
                    return;
                }
                break;
            }
            assert(this->root == nullptr);
            assert(this->processToNodes.empty());
            assert(this->hwndToNodes.empty());
        }
        this->Destroy();

        this->appOwner->OnWinInstClose(this);
        //event.Skip(false);
    }

    void WinInst::OnDestroy(wxWindowDestroyEvent& event)
    {
        // TODO: NOP - remove event
    }

    void WinInst::OnSizeEvent(wxSizeEvent& event)
    {
        event.Skip(true);
        this->RefreshLayout(true, true, false);
    }

    void WinInst::OnPaintEvent(wxPaintEvent& event)
    {
        wxPaintDC dc(this);
	}

    void WinInst::OnKeyDownEvent(wxKeyEvent& event)
    {}

	void WinInst::OnMouseMotionEvent(wxMouseEvent& event)
    {
        wxPoint mousePt = event.GetPosition();
        wxString status = wxString::Format("%i, %i", mousePt.x, mousePt.y);
        this->GetStatusBar()->SetStatusText(status);

        wxPoint mousePos = event.GetPosition();
        if(dragState != MouseDragState::DraggingSash)
            this->ProcessMousePointForSash(mousePos);

        if(this->HasCapture() && this->draggedSash != nullptr)
        {
            int contIdx = this->draggedSash->index;
            Node* container = this->draggedSash->container;
            assert(container->type == NodeType::SplitHoriz || container->type == NodeType::SplitVert);
            const Context& ctx = this->appOwner->GetLayoutContext();

            if(container->type == NodeType::SplitHoriz)
            {
				Node* leftSide = container->children[contIdx + 0];
				Node* rightSide = container->children[contIdx + 1];

                float propLeft = 1.0f - container->ChildPropTakenUpByAllExcept(leftSide, rightSide);
                int minLeft = leftSide->pos.x + leftSide->minSize.x;
                int maxRight = rightSide->pos.x + rightSide->size.x;
                int minRight = maxRight - rightSide->minSize.x - ctx.sashWidth;
                int workableX = minRight - minLeft;
                if(workableX > 0)
                {
                    int spotX = mousePos.x - this->dragSashOffset.x;
                    float tl = (float)(spotX - minLeft)/(float)(workableX);
                    tl = std::clamp(tl, 0.0f, 1.0f);
                    float tr = 1.0f - tl;
                    // Note that tr and tl are the proportions between each other,
                    // with no regard to any other siblings, while their `prop` does
                    // regard siblings.
                    leftSide->prop = propLeft * tl;
                    rightSide->prop = propLeft * tr;
                    leftSide->size.x = leftSide->minSize.x + workableX * tl;
                    rightSide->pos.x = leftSide->pos.x + leftSide->size.x + ctx.sashWidth;
                    rightSide->size.x = maxRight - rightSide->pos.x;
                    leftSide->CacheCalculatedSizeRecursive(leftSide->pos, leftSide->size, ctx);
                    leftSide->ApplyCachedlayout(ctx);
                    rightSide->CacheCalculatedSizeRecursive(rightSide->pos, rightSide->size, ctx);
                    rightSide->ApplyCachedlayout(ctx);

                    this->UpdateSash(leftSide, rightSide);
					this->UpdateContainerSashes(leftSide);
					this->UpdateContainerSashes(rightSide);
                    //Refresh();
                }
            }
            else //if(container->type == NodeType::SplitVert)
            {
				Node* topSide = container->children[contIdx + 0];
				Node* botSide = container->children[contIdx + 1];

				float propLeft = 1.0f - container->ChildPropTakenUpByAllExcept(topSide, botSide);
				int minTop = topSide->pos.y + topSide->minSize.y;
				int maxBot = botSide->pos.y + botSide->size.y;
				int minBot = maxBot - botSide->minSize.y - ctx.sashWidth;
				int workableY = minBot - minTop;
				if (workableY > 0)
				{
					int spotY = mousePos.y - this->dragSashOffset.y;
					float tt = (float)(spotY - minTop) / (float)(workableY);
					tt = std::clamp(tt, 0.0f, 1.0f);
                    float tb = 1.0f - tt;
					topSide->prop = propLeft * tt;
					botSide->prop = propLeft * tb;
					topSide->size.y = topSide->minSize.y + workableY * tt;
					botSide->pos.y = topSide->pos.y + topSide->size.y + ctx.sashWidth;
					botSide->size.y = maxBot - botSide->pos.y;
                    topSide->CacheCalculatedSizeRecursive(topSide->pos, topSide->size, ctx);
					topSide->ApplyCachedlayout(ctx);
                    botSide->CacheCalculatedSizeRecursive(botSide->pos, botSide->size, ctx);
					botSide->ApplyCachedlayout(ctx);

                    this->UpdateSash(topSide, botSide);
                    this->UpdateContainerSashes(topSide);
                    this->UpdateContainerSashes(botSide);
                    Refresh();
                }
            }
        }
    }

	void WinInst::OnMouseLeftDownEvent(wxMouseEvent& event)
    {
        wxPoint mousePos = event.GetPosition();
        Sash* s = ProcessMousePointForSash(mousePos);
        if (s != nullptr)
        {
            this->draggedSash = s;
            this->dragState = MouseDragState::DraggingSash;
            this->dragSashOffset = mousePos - s->pos;
            CaptureMouse();
        }
    }

	void WinInst::OnMouseLeftUpEvent(wxMouseEvent& /*event*/ )
    {
        if(this->HasCapture())
        {
            this->ReleaseMouse();
            this->StopDraggingSash();
        }
    }

	void WinInst::OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& /*event*/)
    {
        this->StopDraggingSash();

    }

	void WinInst::OnMouseCaptureChangedEvent(wxMouseCaptureChangedEvent& /*event*/ )
    {
		this->dragState = MouseDragState::Void;
		this->draggedSash = nullptr;
		this->SetDragCursor(DragCursor::None);
    }

	void WinInst::OnMouseEnterEvent(wxMouseEvent& event)
    {
        // When deleting stuff, we can hit this even before we do a normal
        // dirty fix, which can cause memory-access issue with old sashes.
        // So we short-circuit it. In other cases, there should be little overhead
        // if the dirty flags aren't set.
        this->RefreshLayout();

        if(dragState != MouseDragState::DraggingSash)
            this->ProcessMousePointForSash(event.GetPosition());
    }

	void WinInst::OnMouseLeaveEvent(wxMouseEvent& /*event*/)
    {
        if(dragState != MouseDragState::DraggingSash)
            this->SetDragCursor(DragCursor::None);
    }

    void WinInst::OnTimerEvent_WaitGUIProcess(wxTimerEvent& event)
    {
        std::set<HANDLE> processesToRm;
        for(auto kvp : processToNodes)
        {
            DWORD waitRet = ::WaitForInputIdle(kvp.first, 0);
            if(waitRet == WAIT_FAILED)
            { 
                processesToRm.insert(kvp.first);
                kvp.second.invalid = true;
            }
            else if(waitRet == 0)
                kvp.second.awaitedForIdleInput = true;

            if(kvp.second.awaitedForIdleInput && !kvp.second.invalid)
            {
                DWORD processId = ::GetProcessId(kvp.first);
                HWND hwnd = nullptr;

                Node* n = kvp.second.node;
                auto passInParams = std::make_pair(processId, n);
				::EnumWindows(
                    [](HWND hwnd, LPARAM lParam) -> BOOL 
                    {
                        auto* data = (std::pair<DWORD, Node*>*)lParam;
					    DWORD pid = 0;
					    ::GetWindowThreadProcessId(hwnd, &pid);

						if (pid == data->first && ::IsWindowVisible(hwnd) && ::GetParent(hwnd) == nullptr)
						{
                            data->second->winHandle = hwnd;
							return FALSE;
						}
					    return TRUE;
					}, 
                    (LPARAM)&passInParams);

                if(n->winHandle != nullptr)
                {
                    processesToRm.insert(n->process);
                    ::CloseHandle(n->process);
                    kvp.second.node->process = nullptr;

                    this->CacheOrigHWNDProperties(n);
                    this->ReparentNodeHWNDIntoLayout(n);
                    const Context& ctx = this->GetAppContext();
                    n->ApplyCachedlayout(ctx);
                    n->titlebar->Refresh();
                }
            }
        }
        // TODO: Check for boot timeout - that can last across multiple
        // timers

        for(HANDLE toRm : processesToRm)
            this->processToNodes.erase(toRm);

        if(this->processToNodes.size() > 0)
        {
            const Context& ctx = this->appOwner->GetLayoutContext();
            this->awaitGUITimer.Start(ctx.millisecondsAwaitGUI, true);
        }
    }

    void WinInst::OnTimerEvent_RefreshLayout(wxTimerEvent& event)
    {
		this->RefreshLayout();
        this->Refresh();
    }

    void WinInst::OnExit(wxCommandEvent& /*event*/)
    {
        this->Close(true);
    }

    void WinInst::OnAbout(wxCommandEvent& /*event*/)
    {
        wxMessageBox("This is a wxWidgets Hello World example\nTODO: Redo about page",
            "About WinMux", wxOK | wxICON_INFORMATION);
    }

	void WinInst::OnMenu_TestAddTop(wxCommandEvent& event)
    {
        //if (Dock(L"paint", this->root, DockDir::Top, -1) != nullptr)
        if (this->Dock(L"notepad", this->root, DockDir::Top, -1) != nullptr)
			this->Refresh();

        assert(this->VALI());
    }

	void WinInst::OnMenu_TestAddLeft(wxCommandEvent& event)
    {
		if (this->Dock(L"mspaint", this->root, DockDir::Left, -1) != nullptr)
			Refresh();

        assert(this->VALI());
    }

	void WinInst::OnMenu_TestAddBottom(wxCommandEvent& event)
    {
		if (this->Dock(L"mspaint", this->root, DockDir::Bottom, -1) != nullptr)
			Refresh();

        assert(this->VALI());
    }

	void WinInst::OnMenu_TestAddRight(wxCommandEvent& event)
    {
		if (this->Dock(L"notepad", this->root, DockDir::Right, -1) != nullptr) // TODO: Figure out how to get rid of the wchar_t cast
			Refresh();

        assert(this->VALI());
    }

    void WinInst::OnMenu_CreateDebugSetup(wxCommandEvent& event)
    {
        this->Dock(L"notepad", this->root, DockDir::Top, -1);
        this->Dock(L"notepad", this->root, DockDir::Left, -1);
        this->Dock(L"notepad", this->root, DockDir::Bottom, -1);
        this->Dock(L"notepad", this->root, DockDir::Right, -1);
        this->Dock(L"notepad", this->root, DockDir::Top, -1);
    }

    void WinInst::OnMenu_CreateDebugTabSetup(wxCommandEvent& event)
    {
		this->Dock(L"notepad", this->root, DockDir::Top, -1);
		Node* botNode = this->Dock(L"notepad", this->root, DockDir::Bottom, -1);
		this->Dock(L"notepad", botNode, DockDir::Into, -1);
    }

	void WinInst::OnMenu_CloseMode_Destroy(wxCommandEvent& event)
    {
        this->closeMode = CloseMode::DestroyAll;
        this->RefreshCloseModeMenus();
    }

	void WinInst::OnMenu_CloseMode_Release(wxCommandEvent& event)
    {
        this->closeMode = CloseMode::ReleaseAll;
        this->RefreshCloseModeMenus();
    }

	void WinInst::OnMenu_CloseMode_Close(wxCommandEvent& event)
    {
        this->closeMode = CloseMode::BlockingCloseAll;
        this->RefreshCloseModeMenus();
    }

	void WinInst::OnMenu_ReleaseAll(wxCommandEvent& event)
    {
        this->ReleaseAll();
    }

	void WinInst::OnMenu_DetachAll(wxCommandEvent& event)
    {
		this->DetachAll();
    }

	void WinInst::OnMenu_CloseAll(wxCommandEvent& event)
    {
		this->NonblockingCloseAll();
    }

    bool WinInst::VALI()
    {
        int processesFound = 0;
        int hwndsFound = 0;
        std::set<Node*> seenNodes;
        std::set<HWND> seenHwnds;
        std::set<HANDLE> seenProcesses;

        if(root != nullptr)
        {
            assert(root->parent == nullptr);

            std::queue<Node*> toScan;
            toScan.push(this->root);

            while(!toScan.empty())
            {
                Node* node = toScan.front();
                toScan.pop();

                assert(!seenNodes.contains(node));
                seenNodes.insert(node);

                node->VALI(false);
                if(node->winHandle != nullptr)
                {
                    ++hwndsFound;

                    assert(!seenHwnds.contains(node->winHandle));
                    seenHwnds.insert(node->winHandle);
                }
                if(node->process != nullptr)
                { 
                    ++processesFound;

                    assert(!seenProcesses.contains(node->process));
                    seenProcesses.insert(node->process);
                }

                for(Node* child : node->children)
                    toScan.push(child);
            }
        }
        assert(processesFound == this->processToNodes.size());
        assert(hwndsFound == this->hwndToNodes.size());
        return true;
    }
}