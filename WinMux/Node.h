#pragma once
#include "Defines.h"
#include <vector>
#include <wx/wx.h>

namespace WinMux
{
	struct Node
	{
	public:
		static int debugCtr;

	//private:
	public:
		int debugID;

	public:
		NodeType type = NodeType::Err;
		Node* parent = nullptr;
		std::vector<Node*> children;

		wxPoint pos;
		wxSize size;
		wxSize minSize;

		float prop = 0.0f;

		// Tab Node Stuff
		SubTabs* tabsBar = nullptr;

		// Windows Node Stuff
		HWND winHandle = nullptr;
		HANDLE process = nullptr;
		SubTitlebar* titlebar = nullptr;
		int activeTab = 0;

		bool cachedHWNDProperties = false;
		wxSize origSize;
		LONG origStyle;
		LONG origExStyle;
		wxString cachedAppTitle;
		wxString customTitle;

	public:
		Node();
		Node(NodeType type, Node* parent, float prop);
		~Node();
		static Node* MakeWinNode(Node* parent, float prop, HWND hwnd, HANDLE process);
		static Node* MakeSplitContainerNode(NodeType containerType, Node* parent, float prop, Node* left, Node* right, bool swap);

		void CalculateMinSizeRecursive(const Context& ctx);
		void CacheChildProportions(bool recurse);
		void NormalizeChildProportions();
		float AvgChildProp();
		void CacheCalculatedSizeRecursive(const wxPoint& pos, const wxSize& size, const Context& ctx);
		
		void ApplyCachedlayout(const Context& ctx);
		void ApplyNodeLayout(const wxPoint& pos, const wxSize& size, const Context& ctx);

		int GetChildIndex(Node* child);
		bool SwapChild(Node* src, Node* dst, bool deparentSrc = true);
		bool InsertAsChild(Node* toInsert, Node* existingChild, Insertion insertSpot);
		bool InsertAsChild(Node* toInsert, int insertIdx);
		bool ClampTabId();

		float ChildPropTakenUpByAllExcept(Node* a, Node* b);

		wxString GetWindowString();
		bool CacheWindowString(bool refreshTitlebar);
		const wxString& GetTitlebarDisplayString();
		void RefreshTitlebarAndTab();

		void CleanSupportingUI();

		bool VALI(bool recurse);
		bool VALI_IsFilledWinNode();
	};
}