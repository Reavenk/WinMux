#include "App.h"
#include "WinInst.h"
#include "Utils.h"
#include "TaskbarIcon.h"
#include "wx/artprov.h"
#include <WinUser.h>

namespace WinMux
{
    wxDEFINE_EVENT(DRAGWIN_START,       DragWinNoticeEvent);
    wxDEFINE_EVENT(DRAGWIN_END,         DragWinNoticeEvent);
    wxDEFINE_EVENT(DRAGWIN_MOTION,      DragWinNoticeEvent);
    wxDEFINE_EVENT(DRAGWIN_CANCELLED,   DragWinNoticeEvent);

    BEGIN_EVENT_TABLE(App, wxApp)
        EVT_TIMER(App::CommandIDs::CID_TimerHandleSubEvents, App::OnTimer_HandleSubEvents)
        EVT_DRAGWIN_START(wxID_ANY,     App::OnDragWin_Started)
        EVT_DRAGWIN_END(wxID_ANY,       App::OnDragWin_Ended)
        EVT_DRAGWIN_MOTION(wxID_ANY,    App::OnDragWin_Motion)
        EVT_DRAGWIN_CANCELLED(wxID_ANY, App::OnDragWin_Cancelled)
    END_EVENT_TABLE()

    App& App::GetAppInst()
    {
        return (App&)wxGetApp();
    }

    App::App()
        : handleQueuedSubEventsTimer(this, CID_TimerHandleSubEvents)        
    {}

	bool App::OnInit()
	{
		singleInstChecker = new wxSingleInstanceChecker();
		// https://docs.wxwidgets.org/trunk/classwx_single_instance_checker.html
		if (singleInstChecker->IsAnotherRunning())
		{
			wxLogError(_("Another program instance is already running, aborting."));
			delete singleInstChecker; // OnExit() won't be called if we return false
			singleInstChecker = nullptr;
			return false;
		}

		if (!wxApp::OnInit())
			return false;

		if (!wxTaskBarIcon::IsAvailable())
		{
			wxMessageBox
			(
				"There appears to be no system tray support in your current environment. This sample may not behave as expected.",
				"Warning",
				wxOK | wxICON_EXCLAMATION
			);
		}

		this->SpawnInst();

		this->taskbarIcon = new TaskbarIcon(this);
        //// TODO: Change tutorial text for taskbar icon
		//if (!this->taskbarIcon->SetIcon(wxArtProvider::GetBitmapBundle(wxART_WX_LOGO, wxART_OTHER, wxSize(32, 32)),
		//	"wxTaskBarIcon Sample\n"
		//	"With a very, very, very, very\n"
		//	"long tooltip whose length is\n"
		//	"greater than 64 characters."))
		//{
		//	wxLogError("Could not set icon.");
		//}
		//this->taskbarIcon->ShowBalloon("Testing", "Testing testing");

		hook_titleChange    = ::SetWinEventHook(EVENT_OBJECT_NAMECHANGE, EVENT_OBJECT_NAMECHANGE, nullptr, Hook_OnTitleChanged, 0, 0, 0);
		hook_windowClosed   = ::SetWinEventHook(EVENT_OBJECT_DESTROY, EVENT_OBJECT_DESTROY, nullptr, Hook_OnWindowClosed, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
		hook_dragWindow     = ::SetWinEventHook(EVENT_SYSTEM_MOVESIZESTART, EVENT_SYSTEM_MOVESIZEEND, nullptr, Hook_OnWindowDragEvts, 0, 0, WINEVENT_OUTOFCONTEXT);

		return true;
	}

	int App::OnExit()
	{
        if(this->olyDropPreview != nullptr)
        {
            delete this->olyDropPreview;
            this->olyDropPreview = nullptr;
        }

		assert(hook_titleChange != nullptr);
		UnhookWinEvent(hook_titleChange);
		assert(hook_windowClosed != nullptr);
		UnhookWinEvent(hook_windowClosed);
		assert(hook_dragWindow != nullptr);
		UnhookWinEvent(hook_dragWindow);

		{
			std::lock_guard mutexScope(this->dropWindowMutex);
			if (this->lowLevelMouseHook != nullptr)
			{
				UnhookWindowsHookEx(this->lowLevelMouseHook);
				this->lowLevelMouseHook = nullptr;
			}
		}


		delete taskbarIcon;
		delete singleInstChecker;
		return wxApp::OnExit();
	}

    void App::HideDropPreview()
    {
        if(this->olyDropPreview != nullptr)
            this->olyDropPreview->Hide();
    }

    void App::CalculateDropPreview(const wxPoint& globalMousePos, HWND hwndIgnoreTopLevel)
    {
        OverlapDropDst dropDst = OverlayDropAll(globalMousePos, hwndIgnoreTopLevel);
        if(!dropDst.active)
        {
			HideDropPreview();
        }
        else
        {
			if (this->olyDropPreview == nullptr)
				this->olyDropPreview = new OverlayDropDstPreview(this);

            wxPoint globalPos = dropDst.win->ClientToScreen(dropDst.previewLoc.GetPosition());
            wxRect globalRect(globalPos.x, globalPos.y, dropDst.previewLoc.width, dropDst.previewLoc.height);

            
            this->olyDropPreview->SetSize(globalRect);
            if(globalRect.Contains(globalMousePos))
            { 
                this->olyDropPreview->SetBackgroundColour(ToWxColor(this->context.previewHovered));
                this->olyDropPreview->Refresh();
            }
            else
            { 
                this->olyDropPreview->SetBackgroundColour(ToWxColor(this->context.previewUnhovered));
                this->olyDropPreview->Refresh();
            }

            this->olyDropPreview->Show();

        }
    }

    OverlapDropDst App::OverlayDropAll(const wxPoint& globalMousePos, HWND hwndIgnoreTopLevel)
    {
		wxPoint mousePos = wxGetMousePosition();

        POINT posAsPt{globalMousePos.x, globalMousePos.y};
		for (HWND hwnd = ::GetTopWindow(nullptr);
			hwnd != nullptr;
			hwnd = GetWindow(hwnd, GW_HWNDNEXT))
		{
            if(this->olyDropPreview != nullptr && hwnd == this->olyDropPreview->GetHWND())
                continue;
                
            // probably ignoring the window we're dragging, because of course that
            // would occlude
            if(hwnd == hwndIgnoreTopLevel)
                continue;

            HWND toplevel = GetAncestor(hwnd, GA_ROOT);
            if(toplevel != hwnd || !IsWindowVisible(toplevel))
                continue;

            // Only checking windows that are overlapping or occluding the
            // mouse position.
			RECT rc{};
			if (!GetWindowRect(hwnd, &rc)) 
                continue;
		
            wxRect wxr(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
            if(!wxr.Contains(globalMousePos))
                continue;

            // If a normal window, they're blocking our drop.
            {
                std::lock_guard mutexGuard(this->winRootHwndsMutex);
                if(!this->winInstRootHwnds.contains(toplevel))
                    continue;
                    //return OverlapDropDst::Inactive();
            }
            for(WinInst* wi : this->winInsts)
            { 
                if(wi->GetHWND() != toplevel)
                    continue;

                // First WinInst directly below is what we're going to drop on to.
                wxPoint clientMousePos = wi->ScreenToClient(globalMousePos);
                return wi->GetPreviewDropOverlayDst(clientMousePos);
            }
            // Illegal state would only happen if winInstRootHwnds and winInsts
            // are desynced.
            ASSERT_REACHED_ILLEGAL_POINT();
		    return OverlapDropDst::Inactive();
		}
        return OverlapDropDst::Inactive();
    }

	WinInst* App::SpawnInst()
	{
		WinInst* newWinInst = new WinInst(this, this->defaultCloseMode);
		this->winInsts.push_back(newWinInst);
        {
            std::lock_guard mutexScope(this->winRootHwndsMutex);
            this->winInstRootHwnds.insert(newWinInst->GetHWND());
        }
		newWinInst->Show(true);
		return newWinInst;
        //!TODO: App::VALI()
	}

	bool App::FindHWND(HWND hwnd, OUT WinInst*& ownerInst, OUT Node*& ownerNode)
	{
		for (WinInst* wi : this->winInsts)
		{
			Node* find = wi->FindNodeForHWND(hwnd);
			if (find != nullptr)
			{
				ownerNode = find;
				ownerInst = wi;
				return true;
			}
		}
		ownerInst = nullptr;
		ownerNode = nullptr;
		return false;
	}

    void App::OnTimer_HandleSubEvents(wxTimerEvent & evt)
    {
        std::queue<SystemSubscribedEvent> toProcess;
        this->SwapWithSubbedEvents(toProcess);

        // Note that we don't handle any low-level stuff here. We just
        // verify if the event is something we're managing, and then
        // dispatch to the WinInst that has the HWND.
        while(!toProcess.empty())
        {
            try
            {
                SystemSubscribedEvent winEvt = toProcess.front();
                toProcess.pop();

                switch(winEvt.event)
                {
                case EVENT_OBJECT_NAMECHANGE:
                    {
                        WinInst* win;
                        Node* filler;
                        if(this->FindHWND(winEvt.hwnd, OUT win, OUT filler))
                        {
                            assert(win != nullptr);
                            win->OnWinEvent_TitleChanged(winEvt.hwnd);
                        }
                    }
                    break;

                case EVENT_OBJECT_DESTROY:
                    {
					    WinInst* win;
					    Node* filler;
					    if (this->FindHWND(winEvt.hwnd, OUT win, OUT filler))
					    {
						    assert(win != nullptr);
						    win->OnWinEvent_WindowClosed(winEvt.hwnd);
					    }
                    }
                    break;
                }
            }
            catch(const std::exception& /*ex*/)
            {}
        }
    }

	void App::OnDragWin_Started(DragWinNoticeEvent& event)
    {
        this->hwndBeingDragged = event.hwnd;
        this->CalculateDropPreview(event.mousePos, event.hwnd);
    }

	void App::OnDragWin_Ended(DragWinNoticeEvent& event)
    {
		this->HideDropPreview();

        OverlapDropDst dropDst = this->OverlayDropAll(event.mousePos, hwndBeingDragged);
        // dropDst.VALI()
        assert(event.hwnd != nullptr);
        if(dropDst.active)
        {
            wxPoint globalRectPos = dropDst.win->ClientToScreen(dropDst.previewLoc.GetPosition());
            wxRect globalRect(globalRectPos, dropDst.previewLoc.GetSize());
            if(globalRect.Contains(event.mousePos))
                dropDst.win->Dock(event.hwnd, nullptr, dropDst.node, dropDst.dockDir, 0);
        }
        this->hwndBeingDragged = nullptr;
    }

    void App::OnDragWin_Motion(DragWinNoticeEvent & event)
    {
        this->CalculateDropPreview(event.mousePos, this->hwndBeingDragged);
    }

	void App::OnDragWin_Cancelled(DragWinNoticeEvent& event)
    {
        hwndBeingDragged = nullptr;
        this->HideDropPreview();
    }

    WinInst* App::FindWinInstManagingNode(Node* node)
    {
        for(WinInst* wi: this->winInsts)
        {
            if(wi->HasNode(node))
                return wi;
        }
        return nullptr;
    }

    bool App::IsManagingHWND(HWND hwnd)
    {
        for(WinInst* wi: this->winInsts)
        {
            if(wi->IsManagingHwnd(hwnd))
                return true;
        }
        return false;
    }

    void App::OnWinInstClose(WinInst* inst)
    {
        std::lock_guard mutexScope(this->winRootHwndsMutex);

        int origSize = this->winInsts.size();
        auto itRem = std::remove(this->winInsts.begin(), this->winInsts.end(), inst);
        this->winInsts.erase(itRem, this->winInsts.end());
        assert(origSize != this->winInsts.size());
        this->winInstRootHwnds.erase(inst->GetHWND());
        // !TODO: App::VALI()
    }

	void App::ReleaseAllAndCloseAll()
    {
        std::vector<WinInst*> cpy;
        {
            std::lock_guard mutexScope(this->winRootHwndsMutex);
            cpy = this->winInsts;
        }
        for(WinInst* toClose : cpy)
        {
            toClose->ReleaseAll();
            toClose->DirectDestroy();
        }
    }

	void App::CloseAll()
    {
		std::vector<WinInst*> cpy;
		{
			std::lock_guard mutexScope(this->winRootHwndsMutex);
			cpy = this->winInsts;
		}
		for (WinInst* toClose : cpy)
		{
			toClose->BlockingCloseAll();
            if(toClose->IsEmptyLayout())
                toClose->DirectDestroy();
		}
    }

	void App::SwapWithSubbedEvents(std::queue<SystemSubscribedEvent>& dst)
    {
        evtSubMutex.lock();
        try
        {
            std::swap(dst, this->queueSubedEvts);
        }
        catch(const std::exception& /*ex*/)
        {}
        evtSubMutex.unlock();
    }

	void App::QueueSubbedEvent(HWND hwnd, DWORD event)
    {
		this->evtSubMutex.lock();
		try
		{
            this->queueSubedEvts.push(
                SystemSubscribedEvent(hwnd, event));
		}
		catch (const std::exception& /*ex*/)
		{}
		this->evtSubMutex.unlock();

        this->FlagSubEventsDirty();
    }

    void App::FlagSubEventsDirty()
    {
        if(!this->handleQueuedSubEventsTimer.IsRunning())
            this->handleQueuedSubEventsTimer.Start(0, true);
    }

	VOID App::Hook_OnTitleChanged(
        HWINEVENTHOOK hWinEventHook, 
        DWORD event, 
        HWND hwnd, 
        LONG idObject, 
        LONG idChild, 
        DWORD idEventThread, 
        DWORD dwmsEventTime)
    {
        App& app = App::GetAppInst();
        app.QueueSubbedEvent(hwnd, event);
    }

	VOID App::Hook_OnWindowClosed(
        HWINEVENTHOOK hWinEventHook, 
        DWORD event, 
        HWND hwnd, 
        LONG idObject, 
        LONG idChild, 
        DWORD idEventThread, 
        DWORD dwmsEventTime)
    {
        if(idObject != OBJID_WINDOW)
            return;

		App& app = App::GetAppInst();
		app.QueueSubbedEvent(hwnd, event);
    }

    VOID App::Hook_OnWindowDragEvts(
        HWINEVENTHOOK hWinEventHook, 
        DWORD event, 
        HWND hwnd, 
        LONG idObject, 
        LONG idChild, 
        DWORD idEventThread, 
        DWORD dwmsEventTime)
    {
        bool isTopLevel = GetAncestor(hwnd, GA_ROOT) == hwnd;
        if(isTopLevel && event == EVENT_SYSTEM_MOVESIZESTART)
        {
            wxPoint mousePos = wxGetMousePosition();
            LONG mousePosParam = MAKELONG(mousePos.x, mousePos.y);
            LRESULT result = SendMessage(hwnd, WM_NCHITTEST, 0, mousePosParam);

            bool doStart = false;
            bool doCancel = false;
            bool rejectSelf = false; // If true, window belongs to our app
            App& app = App::GetAppInst();
            {
                std::lock_guard mutexScope(app.winRootHwndsMutex);
                if(app.winInstRootHwnds.contains(hwnd))
                    rejectSelf = true;
            }
            {
                std::lock_guard<std::mutex> mutexScope(app.dropWindowMutex);
                if (!rejectSelf && result == HTCAPTION)
                {
                    if(app.lowLevelMouseHook == nullptr)
                    {
                        app.lowLevelMouseHook = SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, nullptr, 0);
                        doStart = true;
                    }
                }
                if(!doStart && app.lowLevelMouseHook != nullptr)
                {
                    // Dunno if we would ever hit this, but just for sanity
					UnhookWindowsHookEx(app.lowLevelMouseHook);
					app.lowLevelMouseHook = nullptr;
                    doCancel = true;
                }
            }
            if(doStart)
            {
				wxPoint mousePos = wxGetMousePosition();
				DragWinNoticeEvent dragEvt(DRAGWIN_START, mousePos, hwnd);
				wxPostEvent(&app, dragEvt);
            }
            else if(doCancel)
            {
				wxPoint mousePos = wxGetMousePosition();
				DragWinNoticeEvent dragEvt(DRAGWIN_CANCELLED, mousePos, hwnd);
				wxPostEvent(&app, dragEvt);
            }
        }
        else if(event == EVENT_SYSTEM_MOVESIZEEND)
        {
            App& app = App::GetAppInst();
            bool doEnd = false;
            {
                std::lock_guard<std::mutex> mutexScope(app.dropWindowMutex);
                if(app.lowLevelMouseHook != nullptr)
                {
                    UnhookWindowsHookEx(app.lowLevelMouseHook);
                    app.lowLevelMouseHook = nullptr;
                    doEnd = true;
                }
            }
            if(doEnd)
            {
                wxPoint mousePos = wxGetMousePosition();
				DragWinNoticeEvent dragEvt(DRAGWIN_END, mousePos, hwnd);
				wxPostEvent(&app, dragEvt);
            }
        }
    }

    LRESULT CALLBACK App::LowLevelMouseProc(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam)
    {
        App& app = App::GetAppInst();

        if(nCode == HC_ACTION)
        {
            if(wParam == WM_MOUSEMOVE)
            {
                // Queue handling this message to outside the 
                // low-level mouse callback.
                wxPoint mousePos;
                HWND hwnd = nullptr;
                {
                    std::lock_guard mutexScope(app.dropWindowMutex);
                    MSLLHOOKSTRUCT* data = (MSLLHOOKSTRUCT*)lParam;
                    mousePos = wxPoint(data->pt.x, data->pt.y);
                }
				DragWinNoticeEvent dragEvt(DRAGWIN_MOTION, mousePos, nullptr);
                wxPostEvent(&app, dragEvt);
            }
        }
        return CallNextHookEx(app.lowLevelMouseHook, nCode, wParam, lParam);
    }

    DragWinNoticeEvent::DragWinNoticeEvent(wxEventType commandType, const wxPoint mousePos, HWND hwnd)
		: wxCommandEvent(commandType),
          mousePos(mousePos),
          hwnd(hwnd)
	{}

    DragWinNoticeEvent::DragWinNoticeEvent(const DragWinNoticeEvent& event)
		: wxCommandEvent(event),
		mousePos(event.mousePos),
		hwnd(event.hwnd)
	{}

	wxEvent* DragWinNoticeEvent::Clone() const 
    { 
        return new DragWinNoticeEvent(*this); 
    }

    OverlayDropDstPreview::OverlayDropDstPreview(wxApp* appOwner)
        : wxFrame(nullptr, wxID_ANY, "", wxDefaultPosition, wxSize(100, 100), wxFRAME_NO_TASKBAR|wxSTAY_ON_TOP)
    {
        // Custom extra flags, or else (for some unknown reason) it will 
        // constantly steal focus when being shown - but ONLY AFTER a 
        // window has been docked before on a window.
        HWND selfHwnd = this->GetHWND();
		LONG ex = ::GetWindowLong(selfHwnd, GWL_EXSTYLE);
		::SetWindowLong(selfHwnd, GWL_EXSTYLE, ex | WS_EX_NOACTIVATE);

        this->SetTransparent(128);
    }
}