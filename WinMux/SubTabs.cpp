#include "SubTabs.h"
#include <memory>
#include <wx/graphics.h>
#include "Context.h"
#include "Node.h"
#include "WinInst.h"
#include "Utils.h"

namespace WinMux
{
	struct BrushSet
	{
		wxBrush normal;
		wxBrush hovered;
		wxBrush down;

	public:
		BrushSet(const wxBrush& normal, const wxBrush& hovered, const wxBrush& down)
			: normal(normal),
			  hovered(hovered),
			  down(down)
		{}
	};

	BEGIN_EVENT_TABLE(SubTabs, wxWindow)
		EVT_PAINT(					SubTabs::OnPaintEvent)
		EVT_ENTER_WINDOW(			SubTabs::OnMouseEnterEvent)
		EVT_LEAVE_WINDOW(			SubTabs::OnMouseLeaveEvent)
		EVT_MOTION(					SubTabs::OnMouseMotionEvent)
		EVT_LEFT_DOWN(				SubTabs::OnMouseLeftDownEvent)
		EVT_LEFT_UP(				SubTabs::OnMouseLeftUpEvent)
		EVT_RIGHT_DOWN(				SubTabs::OnMouseRightDownEvent)
		EVT_RIGHT_UP(				SubTabs::OnMouseRightUpEvent)
		EVT_MOUSE_CAPTURE_CHANGED(	SubTabs::OnMouseCaptureChangedEvent)
		EVT_MOUSE_CAPTURE_LOST(		SubTabs::OnMouseCaptureLostEvent)
	END_EVENT_TABLE()

	SubTabs::SubTabs(WinInst* parent, Node* node)
		: wxWindow(parent, wxID_ANY),
		  winOwner(parent),
		  node(node)
	{}

	void SubTabs::OnPaintEvent(wxPaintEvent& evt)
	{
		wxPaintDC dc(this);
		std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create( dc )); // Does this need to be heap allocated ptr?
		assert(this->node->type == NodeType::Tabs);

		const Context& ctx = this->winOwner->GetAppContext();

		//	BACKGROUND FILL
		//////////////////////////////////////////////////
		wxRect clientRect = this->GetClientRect();
		dc.SetBrush(ToWxColor(ctx.color_tabsBG));
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.DrawRectangle(clientRect);

		//	UTILITIES
		//////////////////////////////////////////////////
		struct $_
		{
			static void DrawTabBack(
				Node* winNode,
				wxDC& dc,
				wxGraphicsContext* gc, 
				int floorHeight,
				const wxRect& tabRect,
				int rounding,
				Node* hovered,
				Node* clickCandidate,
				const BrushSet& btnBrushes,
				const Context& ctx)
			{
				int tabRect_xmax = tabRect.GetRight();
				int tabRect_ymax = tabRect.GetBottom();
				double floorY = tabRect_ymax - floorHeight;
				double midX = (tabRect.x + tabRect_xmax)/2.0;

				wxGraphicsPath path = gc->CreatePath();
				path.MoveToPoint(tabRect.x, floorY);
				path.AddArcToPoint(tabRect.x, tabRect.y, midX, tabRect.y, rounding);
				path.AddArcToPoint(tabRect_xmax, tabRect.y, tabRect_xmax, floorY, rounding);
				path.AddLineToPoint(tabRect_xmax, floorY);

				if(winNode == clickCandidate)
					gc->SetBrush(btnBrushes.down);
				else if(winNode == hovered)
					gc->SetBrush(btnBrushes.hovered);
				else
					gc->SetBrush(btnBrushes.normal);

				gc->FillPath(path);
				gc->DrawPath(path);

				DrawTabContents(winNode, dc, tabRect, ctx);
			}
			static void DrawTabFront(
				Node* winNode,
				wxDC& dc,
				wxGraphicsContext* gc, 
				const wxRect& clientRect,
				int floorHeight, 
				const wxRect& tabRect,
				int rounding,
				Node* hovered,
				const BrushSet& btnBrushes,
				const Context& ctx)
			{
				int tabRect_xmax = tabRect.GetRight();
				int tabRect_ymax = tabRect.GetBottom();
				double floorY = tabRect_ymax - floorHeight;
				double midX = (tabRect.x + tabRect_xmax) / 2.0;
				int leftest = clientRect.x;
				int rightest = clientRect.GetRight();

				wxGraphicsPath outlinePath = gc->CreatePath();
				outlinePath.MoveToPoint(leftest, floorY);
				outlinePath.AddLineToPoint(tabRect.x, floorY);
				outlinePath.AddArcToPoint(tabRect.x, tabRect.y, midX, tabRect.y, rounding);
				outlinePath.AddArcToPoint(tabRect_xmax, tabRect.y, tabRect_xmax, floorY, rounding);
				outlinePath.AddLineToPoint(tabRect_xmax, floorY);
				outlinePath.AddLineToPoint(rightest, floorY);

				wxGraphicsPath fillPath = outlinePath;
				const int aaComp = 2; // Compensate for anti-alias by doing a little overdrawing on the bottom edge
				fillPath.AddLineToPoint(rightest,	tabRect_ymax + aaComp);
				fillPath.AddLineToPoint(leftest,	tabRect_ymax + aaComp);

				if (winNode == hovered)
					gc->SetBrush(btnBrushes.hovered);
				else
					gc->SetBrush(btnBrushes.normal);

				gc->FillPath(fillPath);
				gc->DrawPath(outlinePath);

				DrawTabContents(winNode, dc, tabRect, ctx);
			}

			static void DrawTabContents(Node* winNode, wxDC& dc, const wxRect& tabRect, const Context& ctx)
			{
				assert(winNode->type == NodeType::Window);

				wxRect textRgn = GetTabFeature(tabRect, TabFeature::Text, ctx);
				int extentX, extenty;
				wxString titlebarText = winNode->GetTitlebarDisplayString();
				dc.GetTextExtent(titlebarText, &extentX, &extenty);
				dc.DrawText(titlebarText, textRgn.x, textRgn.y + (textRgn.height - extenty)/2);
			}
		};

		//	BACK TABS
		//////////////////////////////////////////////////
		BrushSet backTabBrushes( // TODO: To utility function
			wxBrush(ToWxColor(ctx.btncols_TabsUnsel.normal)),
			wxBrush(ToWxColor(ctx.btncols_TabsUnsel.hovered)),
			wxBrush(ToWxColor(ctx.btncols_TabsUnsel.pressed)));

		const size_t nodeCt = this->node->children.size();
		gc->SetPen(*wxBLACK_PEN);

		int activeIdx = this->node->activeTab;
		for(int i = 0; i < nodeCt; ++i)
		{
			if(i == activeIdx)
			{
				// active node is drawn on a painted layer on top
				continue;
			}
			wxRect tabRgn = this->GetTabRect(clientRect, i);
			$_::DrawTabBack(
				this->node->children[i], 
				dc,
				gc.get(), 
				5, 
				tabRgn, 
				5,
				this->currentHovered,
				this->clickCandidate,
				backTabBrushes,
				ctx);
		}

		//	FRONT TAB
		//////////////////////////////////////////////////
		BrushSet activeTabBrushes(
			wxBrush(ToWxColor(ctx.btncol_Tabs.normal)),
			wxBrush(ToWxColor(ctx.btncol_Tabs.hovered)),
			wxBrush(ToWxColor(ctx.btncol_Tabs.pressed)));

		gc->SetBrush(wxBrush(ToWxColor(ctx.btncols_WinTitle.normal)));
		wxRect tabFrontRgn = this->GetTabRect(clientRect, activeIdx);
		$_::DrawTabFront(
			this->node->children[activeIdx],
			dc,
			gc.get(), 
			clientRect, 
			5, 
			tabFrontRgn, 
			5,
			this->currentHovered,
			activeTabBrushes,
			ctx);
		
	}

	wxRect SubTabs::GetTabRect(const wxRect clientRgn, int tabId)
	{
		size_t nodeCt = this->node->children.size();
		assert(tabId >= 0 && tabId < nodeCt);
		int pxLeft =	(int)(((float)(tabId + 0)/(float)nodeCt) * (float)clientRgn.width);
		int pxRight =	(int)(((float)(tabId + 1)/(float)nodeCt) * (float)clientRgn.width);
		return wxRect(pxLeft, clientRgn.y, pxRight - pxLeft, clientRgn.height);
	}

	wxRect SubTabs::GetTabFeature(const wxRect tabRgn, TabFeature feature, const Context& ctx)
	{
		switch(feature)
		{
		case TabFeature::Text:
			// TODO: Add Context values and use them
			return wxRect(tabRgn.x + 5, tabRgn.y + 5, tabRgn.width - 10, tabRgn.height - 10);
			break;
		}
		return tabRgn;
	}

	int SubTabs::GetTabIndexAt(const wxPoint& pos)
	{
		wxRect clientRect = this->GetClientRect();
		for (size_t i = 0; i < this->node->children.size(); ++i)
		{
			wxRect tabRgn = this->GetTabRect(clientRect, i);
			if (tabRgn.Contains(pos))
			{
				return i;
			}
		}
		return -1;
	}

	Node* SubTabs::GetTabNodeAt(const wxPoint& pos)
	{
		int idx = this->GetTabIndexAt(pos);
		if(idx == -1)
			return nullptr;

		return this->node->children[idx];
	}

	void SubTabs::SetHoveredTabNode(Node* node)
	{
		if(this->currentHovered != node)
		{
			this->currentHovered = node;
			this->Refresh();
		}
	}

	void SubTabs::SetClickCandidateTabNode(Node* node)
	{
		if (this->clickCandidate != node)
		{
			this->clickCandidate = node;
			this->Refresh();
		}
	}

	void SubTabs::OnMouseEnterEvent(wxMouseEvent& evt)
	{
		Node* hoveredTab = GetTabNodeAt(evt.GetPosition());
		this->SetHoveredTabNode(hoveredTab);
	}

	void SubTabs::OnMouseLeaveEvent(wxMouseEvent& evt)
	{
		this->SetHoveredTabNode(nullptr);
	}

	void SubTabs::OnMouseMotionEvent(wxMouseEvent& evt)
	{
		Node* hoveredTab = GetTabNodeAt(evt.GetPosition());
		this->SetHoveredTabNode(hoveredTab);

		if(this->HasCapture() && this->clickCandidate != nullptr)
		{
			const Context& ctx = this->winOwner->GetAppContext();
			wxPoint mousePos = evt.GetPosition();
			wxPoint offs = this->originalClickedPos - mousePos;
			float dist = sqrt(offs.x * offs.x + offs.y * offs.y);
			if(dist >= ctx.minDragDist)
			{
				Node* winToPop = this->clickCandidate;
				this->SetClickCandidateTabNode(nullptr);

				// Copy-pasta'd from SubTitlebar::OnMouseMotionEvent - may want to 
				// unify their code paths together.
				HWND origHwnd = winToPop->winHandle;
				wxPoint origScreenPos = this->ClientToScreen(wxPoint(0, 0));
				this->ReleaseMouse();
				App* app = this->winOwner->GetAppOwner();
				// TODO: What about loading processes?
				this->winOwner->ReleaseManagedWindow(winToPop);
				::SetWindowPos(origHwnd, HWND_TOP, origScreenPos.x, origScreenPos.y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
				wxPoint cursorPos = wxGetMousePosition();
				::PostMessage(origHwnd, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(cursorPos.x, cursorPos.y));
			}
		}
	}

	void SubTabs::OnMouseLeftDownEvent(wxMouseEvent& evt)
	{
		Node* hoveredOn = this->GetTabNodeAt(evt.GetPosition());
		this->SetHoveredTabNode(hoveredOn);
		this->SetClickCandidateTabNode(hoveredOn);
		this->originalClickedPos = evt.GetPosition();
		this->CaptureMouse();
	}

	void SubTabs::OnMouseLeftUpEvent(wxMouseEvent& evt)
	{
		if(this->HasCapture())
		{ 
			// Store because it will get wiped on ReleaseMouse()
			Node* origClickCandidate = this->clickCandidate; 
			this->ReleaseMouse();

			if(origClickCandidate != nullptr)
			{ 
				int idxOver = this->GetTabIndexAt(evt.GetPosition());
				if (idxOver != -1)
				{
					if( this->node->children[idxOver] == origClickCandidate && 
						idxOver != this->node->activeTab)
					{
						const Context& ctx = this->winOwner->GetAppContext();
						this->node->activeTab = idxOver;
						this->node->ApplyCachedlayout(ctx);
						this->Refresh();
					}
				}
			}
		}
		this->SetClickCandidateTabNode(nullptr);
	}

	void SubTabs::OnMouseRightDownEvent(wxMouseEvent& evt)
	{}

	void SubTabs::OnMouseRightUpEvent(wxMouseEvent& evt)
	{}

	void SubTabs::OnMouseCaptureChangedEvent(wxMouseCaptureChangedEvent& evt)
	{
		this->SetClickCandidateTabNode(nullptr);
	}

	void SubTabs::OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& evt)
	{
		this->SetClickCandidateTabNode(nullptr);
	}
}
