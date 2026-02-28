#pragma once
#include <wx/wx.h>
#include <set>
#include <map>
#include "Defines.h"
#include "OverlapDropDst.h"
#include "Sash.h"

namespace WinMux
{
	enum DirtyFlags
	{
		Layout			= 1 << 0,
		Sashes			= 1 << 1,
		FreshMaxChange	= 1 << 2,
		All = ~0
	};

	enum class MouseDragState
	{
		Void,
		DraggingSash
	};

	struct AwaitingHWNDData
	{
		Node* node;
		wxLongLong spawnTime;
		bool awaitedForIdleInput;
		bool invalid;

		AwaitingHWNDData(){}
		AwaitingHWNDData(Node* node, wxLongLong spawnTime)
			: node(node), 
			  spawnTime(spawnTime), 
			awaitedForIdleInput(false), 
			invalid(false)
		{}
	};

	/// <summary>
	///		Given a DockDir, return the various properties of what
	///		kind of container it's for, and if a lower or higher
	///		value (i.e., should it be left/above a reference Node
	///		or right/below the reference Node.
	/// </summary>
	/// <param name="dir">
	///		The direction to get the properties of.
	/// </param>
	/// <param name="type">
	///		Output parameter.
	///		What Node grained type is the direction relevant for.
	/// </param>
	/// <param name="lower">
	///		Output parameter.
	///		If true, when inserted, the direction should be lower (to the left, or lower).
	///		If false, when inserted, the direction should be higher (to the right, or higher).
	/// </param>
	/// <returns>
	///		True if the output values are usable.
	///		False if the input direction isn't a grain direction.
	/// </returns>
	bool GetDockDirGrainStats(DockDir dir, OUT NodeType& type, OUT bool& lower);

	/// <summary>
	///		The windowed app that has a collection of other
	///		external managed windows.
	/// </summary>
	class WinInst : public wxFrame
	{
	public:
		enum
		{
			ID_Base_Reserved = 0,
			TIMER_WaitForGUI,
			TIMER_RefreshLayout,
			MENU_TestAddTop,
			MENU_TestAddLeft,
			MENU_TestAddBottom,
			MENU_TestAddRight,
			MENU_CreateDebugSetup,
			MENU_CreateDebugTabSetup,

			MENU_Rename,

			MENU_CloseMode_DestroyAll,
			MENU_Close_ReleaseAll,
			MENU_Close_BlockingCloseAll,

			MENU_ReleaseAll,
			MENU_DetachAll,
			MENU_CloseAll,
		};
		enum class DragCursor
		{
			None,
			Horiz,
			Vert
		};

	private:
		/// <summary>
		///		The root of the entire layout graph.
		/// </summary>
		Node* root;

		/// <summary>
		///		Cached reference to the main application instance
		///		that this window is a part of.
		/// </summary>
		App* appOwner;

		/// <summary>
		///		A collection of all Nodes in the hierarchy of the root.
		/// </summary>
		std::set<Node*> nodes;

		std::map<SashKey, Sash*> sashes;

		MouseDragState dragState = MouseDragState::Void;
		Sash* draggedSash;
		wxPoint dragSashOffset;

		// Note that windows will either be maps to 
		// hwndToNodes OR processToNodes, BUT NEVER BOTH.

		/// <summary>
		///		A mapping of all managed windows being displayed
		///		in this WinIst - to the Node that represents them.
		/// </summary>
		std::map<HWND, Node*> hwndToNodes;

		/// <summary>
		///		A mapping of all managed window processes that we've
		///		booted for docking but are still waiting for that app's
		///		GUI (toplevel HWND) to initialize.
		/// </summary>
		std::map<HANDLE, AwaitingHWNDData> processToNodes;

		wxTimer awaitGUITimer;
		wxTimer refreshLayoutTimer;

		int dirtyFlags = 0;

		CloseMode closeMode = CloseMode::ReleaseAll;
		wxMenuItem* menuClose_Destroy	= nullptr;
		wxMenuItem* menuClose_Release	= nullptr;
		wxMenuItem* menuClose_Close		= nullptr;

		wxMenuItem* menuRename			= nullptr;
		wxMenuItem* menuResetName		= nullptr;

		Node* maximizedWinNode = nullptr;
		bool directDestroy = false;

		wxString customTitle;

	private:
		void RegisterNode(Node* node, bool cacheOrigWinProperties = true);
		void CacheOrigHWNDProperties(Node* node);
		void ReparentNodeHWNDIntoLayout(Node* node);

		/// <summary>
		///		Remove a Node of type Window from the layout hierarchy,
		///		and ensure:
		///		- the layout is left in a correct topology
		///		- the process record is properly maintained
		///		- nodes record is properly maintained
		/// 
		///		This function is only responsible for the layout and
		///		layout-based UI stuff. The actual HWND and PROCESS
		///		resources are expected to be managed outside this function.
		/// </summary>
		/// <param name="winNode"></param>
		/// <param name="deleteNode"></param>
		/// <returns></returns>
		bool RemoveWinNode(Node* winNode, bool deleteNode = true);
		void CleanContainerNodeIfEmpty(Node* containerNode);

		virtual bool MSWTranslateMessage(WXMSG *msg) override;

	public:
		WinInst(App* appOwner, CloseMode closeMode);
		~WinInst();

		/// <summary>
		///		Create a new node in the graph data structure of docked
		///		windows.
		/// </summary>
		/// <param name="hwnd">
		///		The managed window that's being inserted.
		/// </param>
		/// <param name="where">
		///		The existing Node used as a reference point of where to
		///		create/insert the new Node.
		/// </param>
		/// <param name="dir">
		///		The relative location from the reference point to create
		///		the new Node.
		/// </param>
		/// <param name="idx">
		///		Only used if DockDir is Into, and the destination is
		///		a grained container (horizontal/vertical).
		/// </param>
		/// <returns>
		///		The created Node that represents the managed Window's
		///		position.
		/// </returns>
		Node* Dock(HWND hwnd, HANDLE process, Node* where, DockDir dir, int idx);
		Node* Dock(const wchar_t* process, Node* where, DockDir dir, int idx);

		const Context& GetAppContext();
		App* GetAppOwner();

		bool HasNode(Node* node);
		bool IsManagingHwnd(HWND hwnd);
		Node* FindNodeForHWND(HWND hwnd);

		void ApplyCachedLayout(Node* node);

		Sash* ProcessMousePointForSash(const wxPoint& pos);
		void StopDraggingSash();
		
		// NOTE: We don't have return values for these removal 
		// functions because in some cases it's not up to us to 
		// actually close the layout, we just dispatch requests.
		//
		// Instead, we have asserts(), and that's it. Beyond that, 
		// we just assume the program in the correct state to 
		// honor the request, and that the request or action 
		// happened successfully. Or if there's an error, it's 
		// handled gracefully in a way that outside observers 
		// don't need information on the results 
		// (i.e., the return value).

		/// <summary>
		///		Request the Window Node to be closed.
		///		This sends a close message to the managed HWND, 
		///		and that application may veto the request (such as 
		///		popping up a "Are you sure?" dialog and the using 
		///		clicking to cancel close).
		/// 
		///		Note for HWND Windows, we don't actually close them 
		///		here, if they accept our request and close, we 
		///		intercept the EVENT_OBJECT_DESTROY event hook later 
		///		and THAT will remove Node from the layout.
		/// </summary>
		/// <param name="winNode">
		///		The Window Node to close.
		/// </param>
		void CloseManagedWindow(Node* winNode);

		void CloseAndForgetManagedWindow(Node* winNode);

		/// <summary>
		///		
		/// </summary>
		/// <param name="winNode"></param>
		/// <returns></returns>
		void DestroyManagedWindow(Node* winNode);

		/// <summary>
		///		Request to close a window via the window HWND.
		///		See notes for CloseManagedWindow(Node* winNode) 
		///		for more details.
		/// </summary>
		void CloseManagedWindow(HWND hwnd);

		/// <summary>
		///		Close a Window Node that's a process that we're
		///		waiting for the UI.
		///		
		///		Note that in order to stop it before we have a 
		///		reference to the UI's HWND available, we kill the 
		///		process.
		/// </summary>
		/// <param name="process">
		///		The process to kill.
		/// </param>
		void CancelManagedWindow(HANDLE process);

		/// <summary>
		///		Remove a Window node Node from the layout and spawn 
		///		it as the root of a new WinInst.
		/// </summary>
		WinInst* DetachManagedWindow(Node* winNode);

		/// <summary>
		///		Remove a WinNode from the layout and return it back
		///		to the desktop.
		/// </summary>
		void ReleaseManagedWindow(Node* winNode);

		void InjectExistingWinNodeAsRoot(Node* winNode);

		OverlapDropDst GetPreviewDropOverlayDst(const wxPoint& pt);

		inline bool HasMaximizedWindow() const { return this->maximizedWinNode != nullptr; }
		inline bool IsMaximized(Node* n)  const {return this->maximizedWinNode == n; }
		void SetWinNodeMaximized(Node* winNode);

		void DirectDestroy();

		bool IsEmptyLayout();

	private:
		Node* _innerDock(HWND hwnd, HANDLE process, Node* where, DockDir dir, int idx);

		void RegenerateSashes(Node* node, std::map<SashKey, Sash*>& sashes);
		void DeleteAllSashes();
		bool UpdateSash(Node*a, Node*b);
		void UpdateContainerSashes(Node* containerNode, bool recurse = true);
		void SetDragCursor(DragCursor cursor);

		void RefreshCloseModeMenus();

		void RefreshLayout();
		void RefreshLayout(bool layout, bool sashes, bool freshMax);
		void FlagDirty(bool layout = true, bool sashes = true, bool freshMax = false);

		void SetTitle(const wxString& newTitle);

	public:
		void OnWinEvent_TitleChanged(HWND hwnd);
		void OnWinEvent_WindowClosed(HWND hwnd);

		void ReleaseAll();
		void SendCloseAndForgetAll();
		void BlockingCloseAll();
		void NonblockingCloseAll();
		void DetachAll();

	public:
		void OnClose(wxCloseEvent& event);
		void OnDestroy(wxWindowDestroyEvent& event);
		void OnSizeEvent(wxSizeEvent& event);

		void OnPaintEvent(wxPaintEvent& event);
		void OnKeyDownEvent(wxKeyEvent& event);
		void OnMouseMotionEvent(wxMouseEvent& event);
		void OnMouseLeftDownEvent(wxMouseEvent& event);
		void OnMouseLeftUpEvent(wxMouseEvent& event);
		void OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event);
		void OnMouseCaptureChangedEvent(wxMouseCaptureChangedEvent& event);
		void OnMouseEnterEvent(wxMouseEvent& event);
		void OnMouseLeaveEvent(wxMouseEvent& event);
		void OnTimerEvent_WaitGUIProcess(wxTimerEvent& event);
		void OnTimerEvent_RefreshLayout(wxTimerEvent& event);

		void OnExit(wxCommandEvent& event);
		void OnAbout(wxCommandEvent& event);

		void OnMenu_TestAddTop(wxCommandEvent& event);
		void OnMenu_TestAddLeft(wxCommandEvent& event);
		void OnMenu_TestAddBottom(wxCommandEvent& event);
		void OnMenu_TestAddRight(wxCommandEvent& event);
		void OnMenu_CreateDebugSetup(wxCommandEvent& event);
		void OnMenu_CreateDebugTabSetup(wxCommandEvent& event);

		void OnMenu_Rename(wxCommandEvent& event);

		void OnMenu_CloseMode_Destroy(wxCommandEvent& event);
		void OnMenu_CloseMode_Release(wxCommandEvent& event);
		void OnMenu_CloseMode_Close(wxCommandEvent& event);

		void OnMenu_ReleaseAll(wxCommandEvent& event);
		void OnMenu_DetachAll(wxCommandEvent& event);
		void OnMenu_CloseAll(wxCommandEvent& event);
	public:
		bool VALI();

	protected:
		DECLARE_EVENT_TABLE()
	};
}