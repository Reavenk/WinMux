#pragma once
#include <mutex>
#include <queue>
#include <set>
#include "Defines.h"
#include "Context.h"
#include "OverlapDropDst.h"
#include <wx/wx.h>
#include <wx/snglinst.h>

namespace WinMux
{
    enum class WindowDragPreviewState
    {
        Void,
        HasPreviewOly
    };

    struct SystemSubscribedEvent
    {
        HWND hwnd;
        DWORD event;

        SystemSubscribedEvent(HWND hwnd, DWORD event)
            : hwnd(hwnd),
              event(event)
        {}
    };

	wxDECLARE_EVENT(DRAGWIN_START,      DragWinNoticeEvent);
    wxDECLARE_EVENT(DRAGWIN_END,        DragWinNoticeEvent);
    wxDECLARE_EVENT(DRAGWIN_MOTION,     DragWinNoticeEvent);
    wxDECLARE_EVENT(DRAGWIN_CANCELLED,  DragWinNoticeEvent);
	

    class App : public wxApp
    {
        enum CommandIDs
        {
            CID_TimerHandleSubEvents
        };
	public:
		static App& GetAppInst();

    public:
        App();

    private:
        /// <summary>
        /// !TODO:
        /// </summary>
        Context context;

        /// <summary>
        /// !TODO:
        /// </summary>
        std::vector<WinInst*> winInsts;
        std::set<HWND> winInstRootHwnds; //!TODO: Dictionary with winInsts?
        std::mutex winRootHwndsMutex;

        /// <summary>
        /// !TODO:
        /// </summary>
        TaskbarIcon* taskbarIcon;

        wxSingleInstanceChecker* singleInstChecker = nullptr;

        CloseMode defaultCloseMode = CloseMode::ReleaseAll;

        ////////////////////////////////////////////////////////////////////////////////
        //
        //  SYSTEM EVENT HOOKS
        //
        ////////////////////////////////////////////////////////////////////////////////
    private: // !TODO: Shuffle code in this section to appropriate places
        // Event subscription stuff
        // 
        // Doc of possible events
        // https://learn.microsoft.com/en-us/windows/win32/winauto/event-constants
        HWINEVENTHOOK hook_titleChange = nullptr;
        HWINEVENTHOOK hook_windowClosed = nullptr;
        HWINEVENTHOOK hook_dragWindow = nullptr;

        std::mutex evtSubMutex;
        std::queue<SystemSubscribedEvent> queueSubedEvts;
        wxTimer handleQueuedSubEventsTimer;

        void SwapWithSubbedEvents(std::queue<SystemSubscribedEvent>& dst);
        void QueueSubbedEvent(HWND hwnd, DWORD event);
        void FlagSubEventsDirty();

        // hook callbacks
        // 
        // Current convention right now is to have a unique callback per event,
        // even if they end up being simple and the same duplicate code. This
        // can be reevaluated later.
		static VOID Hook_OnTitleChanged(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime);
        static VOID Hook_OnWindowClosed(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime);
        static VOID Hook_OnWindowDragEvts(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime);

		/// <summary>
		///     Handler for message-queued hook events.
		/// </summary>
		void OnTimer_HandleSubEvents(wxTimerEvent& evt);

        ////////////////////////////////////////////////////////////////////////////////
        //
        //  HOOKING INTO WINDOW DRAGGING SYSTEM
        // 
        ////////////////////////////////////////////////////////////////////////////////
    private:
        // Hooking into window dragging system
        std::mutex dropWindowMutex;
        HHOOK lowLevelMouseHook = nullptr;
        WindowDragPreviewState winDragPreviewState;
        OverlayDropDstPreview* olyDropPreview = nullptr;

		static LRESULT CALLBACK LowLevelMouseProc(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam);
        HWND hwndBeingDragged = nullptr;
        // Handlers for the drag event that we pass to ourselves so we can handle
        // them in our thread, at non-critical times, without worry of corrupting
        // the OS's mouse handling.
		void OnDragWin_Started(DragWinNoticeEvent& event);
		void OnDragWin_Ended(DragWinNoticeEvent& event);
        void OnDragWin_Motion(DragWinNoticeEvent& event);
		void OnDragWin_Cancelled(DragWinNoticeEvent& event);

    private:
        ////////////////////////////////////////////////////////////////////////////////
        //
        //  WINDOW DROP PREVIEW
        //
        ////////////////////////////////////////////////////////////////////////////////
        void HideDropPreview();
        void CalculateDropPreview(const wxPoint& globalMousePos, HWND hwndIgnoreTopLevel);
        OverlapDropDst OverlayDropAll(const wxPoint& globalMousePos, HWND hwndIgnoreTopLevel);

        ////////////////////////////////////////////////////////////////////////////////
        //
        //  WINDOW MANAGEMENT
        //
        ////////////////////////////////////////////////////////////////////////////////
    public:
        WinInst* SpawnInst();

        inline const Context& GetLayoutContext() {return context;}

        bool FindHWND(HWND hwnd, OUT WinInst*& ownerInst, OUT Node*& ownerNode);
        WinInst* FindWinInstManagingNode(Node* node);
        bool IsManagingHWND(HWND hwnd);

    public:
        void OnWinInstClose(WinInst* inst);

        void ReleaseAllAndCloseAll();
        void CloseAll();

        ////////////////////////////////////////////////////////////////////////////////
        //
        //  wxWidgets FUNCTIONALITY
        // 
        ////////////////////////////////////////////////////////////////////////////////
    public:
        virtual bool OnInit() override;
        virtual int OnExit() override;

    protected:
        DECLARE_EVENT_TABLE()
    };

    // https://wiki.wxwidgets.org/Custom_Events
	class DragWinNoticeEvent : public wxCommandEvent
	{
	public:
		wxPoint mousePos;
		HWND hwnd;
	public:
		DragWinNoticeEvent(wxEventType commandType, const wxPoint mousePos, HWND hwnd);
		DragWinNoticeEvent(const DragWinNoticeEvent& event); // You *must* copy here the data to be transported
		wxEvent* Clone() const; // Required for sending with wxPostEvent()
	};

    class OverlayDropDstPreview : public wxFrame // TODO: Move into its own .h/.cpp
    {
    public:
        OverlayDropDstPreview(wxApp* appOwner);
    };
}

typedef void (wxEvtHandler::* DrawWinEventFunction)(WinMux::DragWinNoticeEvent&);
#define DrawWinEventHandler(func)       wxEVENT_HANDLER_CAST(DrawWinEventFunction, func)                    
#define EVT_DRAGWIN_START(id, func)     wx__DECLARE_EVT1(DRAGWIN_START,     id, DrawWinEventHandler(func))
#define EVT_DRAGWIN_END(id, func)       wx__DECLARE_EVT1(DRAGWIN_END,       id, DrawWinEventHandler(func))
#define EVT_DRAGWIN_MOTION(id, func)    wx__DECLARE_EVT1(DRAGWIN_MOTION,    id, DrawWinEventHandler(func))
#define EVT_DRAGWIN_CANCELLED(id, func) wx__DECLARE_EVT1(DRAGWIN_CANCELLED, id, DrawWinEventHandler(func))

DECLARE_APP ( WinMux::App );