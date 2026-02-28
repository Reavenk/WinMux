#include "SubTitlebar.h"

#include <wx/graphics.h>
#include "App.h"
#include "Context.h"
#include "Node.h"
#include "Utils.h"
#include "WinInst.h"

namespace WinMux
{
	BEGIN_EVENT_TABLE(SubTitlebar, wxPanel)
		EVT_PAINT(SubTitlebar::OnPaintEvent)
		EVT_ENTER_WINDOW(SubTitlebar::OnMouseEnterEvent)
		EVT_LEAVE_WINDOW(SubTitlebar::OnMouseLeaveEvent)
		EVT_MOTION(SubTitlebar::OnMouseMotionEvent)
		EVT_LEFT_DOWN(SubTitlebar::OnMouseLeftDownEvent)
		EVT_LEFT_UP(SubTitlebar::OnMouseLeftUpEvent)
		EVT_LEFT_DCLICK(SubTitlebar::OnMouseLeftDoubleClickEvent)
		EVT_MOUSE_CAPTURE_CHANGED(SubTitlebar::OnMouseCaptureChangedEvent)
		EVT_MOUSE_CAPTURE_LOST(SubTitlebar::OnMouseCaptureLostEvent)


		EVT_MENU(MENU_RenameTitle,		SubTitlebar::OnMenu_RenameTitle)
		EVT_MENU(MENU_ResetTitle,		SubTitlebar::OnMenu_ResetTitle)

		EVT_MENU(MENU_CloseWindow,		SubTitlebar::OnMenu_CloseWindow)
		EVT_MENU(MENU_DetachWindow,		SubTitlebar::OnMenu_DetachWindow)
		EVT_MENU(MENU_ReleaseWindow,	SubTitlebar::OnMenu_ReleaseWindow)
		EVT_MENU(MENU_DebugTabClone,	SubTitlebar::OnMenu_DebugTabClone)
	END_EVENT_TABLE()

	SubTitlebar::SubTitlebar(WinInst* parent, Node* node)
	: wxPanel(parent, wxID_ANY),
	  winOwner(parent),
	  node(node)
	{}

	void DrawPathClose(const wxRect& rgn, wxGraphicsContext* gc, double btnRad)
	{
		double cx = rgn.x + rgn.width / 2.0;
		double cy = rgn.y + rgn.height / 2.0;
		gc->SetPen(*wxBLACK_PEN);
		wxGraphicsPath path = gc->CreatePath();
		double crossRad = btnRad * 0.4;
		path.MoveToPoint(cx - crossRad, cy - crossRad);
		path.AddLineToPoint(cx + crossRad, cy + crossRad);
		//
		path.MoveToPoint(cx - crossRad, cy + crossRad);
		path.AddLineToPoint(cx + crossRad, cy - crossRad);
		gc->StrokePath(path);
	}

	void DrawPathPulldown(const wxRect& rgn, wxGraphicsContext* gc, double btnRad)
	{
		double cx = rgn.x + rgn.width / 2.0;
		double cy = rgn.y + rgn.height / 2.0;
		gc->SetPen(*wxBLACK_PEN);
		wxGraphicsPath path = gc->CreatePath();
		double crossRad = btnRad * 0.4;
		double vertOffs = btnRad * 0.2;
		path.MoveToPoint(	cx - crossRad,	cy - vertOffs);
		path.AddLineToPoint(cx,				cy + vertOffs);
		path.AddLineToPoint(cx + crossRad,	cy - vertOffs);
		gc->StrokePath(path);
	}

	void DrawPathMinMax(const wxRect& rgn, wxGraphicsContext* gc, double btnRad, bool max)
	{
		double cx = rgn.x + rgn.width / 2.0;
		double cy = rgn.y + rgn.height / 2.0;
		gc->SetPen(*wxBLACK_PEN);
		wxGraphicsPath path = gc->CreatePath();
		double hRad = btnRad * 0.4;
		double vRad = btnRad * 0.4;
		if(max)
		{
			path.AddRectangle(cx - hRad, cy - vRad, hRad * 2.0, vRad * 2.0);
		}
		else
		{
			path.MoveToPoint(cx - hRad, cy);
			path.AddLineToPoint(cx + hRad, cy);
		}
		gc->StrokePath(path);
	}

	void SubTitlebar::OnPaintEvent(wxPaintEvent& event)
	{
		wxPaintDC dc(this);
		wxGraphicsContext *gc = wxGraphicsContext::Create( dc );

		const Context& ctx = this->winOwner->GetAppContext();
		const Color3& bgFillColor = ctx.btncols_WinTitle.GetColor(
			HasCapture() && this->currentHoverFeature == Feature::Text,
			this->isHoveredOverBar);
		//
		wxBrush brush(ToWxColor(bgFillColor));
		dc.SetBrush(brush);
		
		wxRect wxr = this->GetClientRect();
		gc->SetPen(*wxTRANSPARENT_PEN);
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.DrawRectangle(wxr);

		if(this->node->winHandle)
		{
			const Context& ctx = this->GetAppContext();
			wxRect clientRect = this->GetClientRect();
			// System icon
			//////////////////////////////////////////////////
			wxRect sysRgn = this->GetFeatureRgn(clientRect, Feature::SysIcon, ctx);
			HICON hIcon = (HICON)::GetClassLongPtr(this->node->winHandle, GCLP_HICON);
			::DrawIconEx(
				dc.GetHDC(), 
				sysRgn.x + (sysRgn.width - ctx.titleSysBoxIconDim) / 2, 
				sysRgn.y + (sysRgn.height - ctx.titleSysBoxIconDim) / 2,
				hIcon, 
				ctx.titleSysBoxIconDim, 
				ctx.titleSysBoxIconDim, 
				0, 
				NULL, 
				DI_NORMAL);

			// Title
			//////////////////////////////////////////////////
			wxRect titleRgn = this->GetFeatureRgn(clientRect, Feature::Text, ctx);
			wxFont titleFont(ctx.titleFontSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_LIGHT, false);
			dc.SetFont(titleFont);
			wxString titleText = this->node->GetTitlebarDisplayString();
			wxSize fontPixelSize = titleFont.GetPixelSize();
			dc.DrawText(
				titleText, 
				wxPoint(
					titleRgn.x + ctx.titleTextLeftPadding, 
					titleRgn.y + (titleRgn.height - fontPixelSize.y)/2));

			// Pulldown commands
			//////////////////////////////////////////////////
			const Color3 pulldownColor = ctx.btncols_TitleBtn.GetColor(
				this->currentClickedDown == Feature::PulldownButton,
				this->currentHoverFeature == Feature::PulldownButton);

			wxRect winOptsRgn = this->GetFeatureRgn(clientRect, Feature::PulldownButton, ctx);
			//dc.DrawRectangle(winOptsRgn);
			wxBrush btnPullDownBrush(ToWxColor(pulldownColor));
			//dc.SetBrush(blueBrush);
			gc->SetBrush(btnPullDownBrush);
			wxGraphicsPath pathPulldown = gc->CreatePath();
			pathPulldown.AddCircle( 
				winOptsRgn.x + winOptsRgn.width / 2.0, 
				winOptsRgn.y + winOptsRgn.height / 2.0, 
				ctx.titleBoxButtonIconDim / 2.0 );
			gc->FillPath(pathPulldown);

			DrawPathPulldown(winOptsRgn, gc, winOptsRgn.width / 2.0);

			// MinMax commands
			//////////////////////////////////////////////////
			const Color3 minMaxColor = ctx.btncols_TitleBtn.GetColor(
				this->currentClickedDown == Feature::MinMaxButton,
				this->currentHoverFeature == Feature::MinMaxButton);

			wxRect winMinMaxRgn = GetFeatureRgn(clientRect, Feature::MinMaxButton, ctx);
			wxBrush btnMinMaxBrush(ToWxColor(minMaxColor));
			gc->SetBrush(btnMinMaxBrush);
			wxGraphicsPath pathMinMax = gc->CreatePath();
			pathMinMax.AddCircle(
				winMinMaxRgn.x + winMinMaxRgn.width / 2.0,
				winMinMaxRgn.y + winMinMaxRgn.height / 2.0,
				ctx.titleBoxButtonIconDim / 2.0);
			gc->FillPath(pathMinMax);

			bool isMaxed = this->winOwner->IsMaximized(this->node);
			DrawPathMinMax(winMinMaxRgn, gc, winMinMaxRgn.width / 2.0, !isMaxed);

			//	Close button
			//////////////////////////////////////////////////
			const Color3 closeColor = ctx.btncols_CloseBtn.GetColor(
				this->currentClickedDown == Feature::CloseButton,
				this->currentHoverFeature == Feature::CloseButton);
			//
			wxBrush closeBtnBrush(ToWxColor(closeColor));
			wxRect closeBtnRgn = GetFeatureRgn(clientRect, Feature::CloseButton, ctx);
			wxGraphicsPath closeBtnPath = gc->CreatePath();
			closeBtnPath.AddCircle(
				closeBtnRgn.x + closeBtnRgn.width / 2.0,
				closeBtnRgn.y + closeBtnRgn.height / 2.0,
				ctx.titleBoxButtonIconDim / 2.0);
			gc->SetBrush(closeBtnBrush);
			gc->FillPath(closeBtnPath);

			DrawPathClose(closeBtnRgn, gc, closeBtnRgn.width/2.0);
		}
		else if(this->node->process != nullptr)
		{
			dc.DrawText("Booting...", 0, 0);
		}
		delete gc;
	}

	wxRect SubTitlebar::GetFeatureRgn(const wxRect& entireRgn, Feature f)
	{
		const Context& ctx = this->GetAppContext();
		return this->GetFeatureRgn(entireRgn, f, ctx);
	}

	wxRect SubTitlebar::GetFeatureRgn(const wxRect& entireRgn, Feature f, const Context& ctx)
	{
		switch (f)
		{
		case Feature::Text:
			return
				wxRect(
					entireRgn.x + ctx.titleBoxRegionWidth,
					entireRgn.y,
					entireRgn.width - 4 * ctx.titleBoxRegionWidth,
					entireRgn.height);

		case Feature::SysIcon:
			return
				wxRect(
					entireRgn.x,
					entireRgn.y,
					ctx.titleBoxRegionWidth,
					entireRgn.height);

		case Feature::PulldownButton:
			return
				wxRect(
					entireRgn.x + entireRgn.width - 3 * ctx.titleBoxRegionWidth,
					entireRgn.y,
					ctx.titleBoxRegionWidth,
					entireRgn.height);

		case Feature::MinMaxButton:
			return
				wxRect(
					entireRgn.x + entireRgn.width - 2 * ctx.titleBoxRegionWidth,
					entireRgn.y,
					ctx.titleBoxRegionWidth,
					entireRgn.height);

		case Feature::CloseButton:
			return
				wxRect(
					entireRgn.x + entireRgn.width - 1 * ctx.titleBoxRegionWidth,
					entireRgn.y,
					ctx.titleBoxRegionWidth,
					entireRgn.height);
		}
		return wxRect(-1, -1, -1, -1); // Null;
	}

	SubTitlebar::Feature SubTitlebar::GetFeature(const wxPoint & pt)
	{
		wxRect clientRgn = this->GetClientSize();

		for(
			Feature f = (Feature)0; 
			f < Feature::Totalnum; 
			f = (Feature)((int)f + 1))
		{
			wxRect rgn = this->GetFeatureRgn(clientRgn, f);
			if(rgn.Contains(pt))
				return f;
		}
		return Feature::Null;
	}

	SubTitlebar::Feature SubTitlebar::HandleMouseTooltip(wxPoint pos)
	{
		// TODO: Handle if the HWND isn't available
		Feature f = this->GetFeature(pos);
		switch(f)
		{
		default:
		case Feature::Null:
		case Feature::Text:
			this->UnsetToolTip();
			break;
		case Feature::SysIcon:
			this->SetToolTip("System");
			break;
		case Feature::PulldownButton:
			this->SetToolTip("Commands");
			break;
		case Feature::CloseButton:
			this->SetToolTip("Close");
			break;
		}
		return f;
	}

	void SubTitlebar::OnMouseEnterEvent(wxMouseEvent& evt)
	{
		this->HandleMouseTooltip(evt.GetPosition());

		this->isHoveredOverBar = true;
		this->Refresh();
	}

	void SubTitlebar::OnMouseLeaveEvent(wxMouseEvent& evt)
	{
		this->UnsetToolTip();
		this->currentClickedDown = Feature::Null;
		this->currentHoverFeature = Feature::Null;
		this->isHoveredOverBar = false;
		this->Refresh();
	}

	void SubTitlebar::OnMouseMotionEvent(wxMouseEvent& evt)
	{
		bool refresh = false;
		Feature featureMouseOver = this->HandleMouseTooltip(evt.GetPosition());
		if(featureMouseOver != this->currentHoverFeature)
			refresh = true;

		this->currentHoverFeature = featureMouseOver;

		if(HasCapture())
		{ 
			const Context& ctx = this->GetAppContext();
			switch(currentClickedDown)
			{
			case Feature::Text:
				{
					wxPoint clientMousePos = evt.GetPosition();
					wxPoint offs = clientMousePos - this->originalDownPos;
					float dist = sqrt(offs.x * offs.x + offs.y * offs.y);
					if(dist >= ctx.minDragDist)
					{
						HWND origHwnd = this->node->winHandle;
						wxPoint origScreenPos = this->ClientToScreen(wxPoint(0, 0));
						this->ReleaseMouse();
						App* app = this->winOwner->GetAppOwner();
						// TODO: What about loading processes?
						this->winOwner->ReleaseManagedWindow(this->node);
						::SetWindowPos(origHwnd, HWND_TOP, origScreenPos.x, origScreenPos.y, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE);
						wxPoint cursorPos = wxGetMousePosition();
						PostMessage(origHwnd, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(cursorPos.x, cursorPos.y));
					}
				}
				break;
			case Feature::CloseButton:
			case Feature::PulldownButton:
				if (featureMouseOver != this->currentClickedDown)
				{ 
					this->currentClickedDown = Feature::Null;
					refresh = true;
				}
				break;
			}
		}
		if(refresh)
			this->Refresh();
	}

	void SubTitlebar::OnMouseLeftDownEvent(wxMouseEvent& evt)
	{
		wxPoint mousePos = evt.GetPosition();
		Feature f = this->GetFeature(mousePos);
		this->currentClickedDown = f;

		switch (f)
		{
		default:
		case Feature::Null:
			break;
		case Feature::Text:
			{
				originalDownPos = mousePos;
				CaptureMouse();
			}
			break;
		case Feature::SysIcon:
		case Feature::PulldownButton:
		case Feature::MinMaxButton:
		case Feature::CloseButton:
			this->CaptureMouse();
			break;
		}
		this->Refresh();
	}

	void SubTitlebar::OnMouseLeftUpEvent(wxMouseEvent& evt)
	{
		wxPoint mousePos = evt.GetPosition();
		Feature f = this->HandleMouseTooltip(mousePos);
		if(f != this->currentClickedDown)
			this->currentClickedDown = Feature::Null;

		this->Refresh();

		if(this->HasCapture())
		{
			// We can't reset this later because an action may close this window,
			// which deletes this titlebar, which makes "this" unusable right 
			// after handling the click.
			Feature clickedFeature = this->currentClickedDown;
			this->currentClickedDown = Feature::Null;

			this->ReleaseMouse();
			switch(clickedFeature)
			{
			case Feature::SysIcon:
				if(this->node->winHandle != nullptr)
				{
					int x, y;
					this->DoGetScreenPosition(&x, &y);;
					wxRect clientRgn = this->GetClientRect();
					wxRect sysIconRgn = this->GetFeatureRgn(clientRgn, Feature::SysIcon);
					HMENU sysMenu = ::GetSystemMenu(this->node->winHandle, FALSE);
					if (sysMenu != nullptr)
					{ 
						// The Node's HWND isn't going to automatically work as the owner.
						// So we own the menu, but track with TPM_RETURNCMD - and send the
						// system menu message manually afterwards.
						int retval = 
							::TrackPopupMenu(
								sysMenu, 
								TPM_LEFTALIGN|TPM_RETURNCMD, 
								x + sysIconRgn.x, 
								y + sysIconRgn.GetBottom(), 
								0, 
								this->GetHWND(),
								nullptr);

						if(retval != 0)
						{
							wxPoint mousePos = wxGetMousePosition();
							::SendMessage(this->node->winHandle, WM_SYSCOMMAND, retval, MAKELPARAM(mousePos.x, mousePos.y));
						}
					}
				}
				break;

			case Feature::PulldownButton:
				{
					wxMenu menu;
					menu.Append(MENU_RenameTitle,		"Rename Title");
					if(!this->node->customTitle.IsEmpty())
						menu.Append(MENU_ResetTitle,	"Reset Title");
					menu.AppendSeparator();
					menu.Append(MENU_DetachWindow,		"Detach");
					menu.Append(MENU_ReleaseWindow,		"Release");
					menu.AppendSeparator();
					menu.Append(MENU_CloseWindow,		"Close");
					menu.Append(MENU_DebugTabClone,		"Clone");
					wxRect btnRgn = this->GetFeatureRgn(this->GetClientRect(), Feature::PulldownButton);
					this->DoPopupMenu(&menu, btnRgn.x, btnRgn.y + btnRgn.height);
				}
				break;

			case Feature::MinMaxButton:
				{
					this->winOwner->SetWinNodeMaximized(this->node);
				}
				break;

			case Feature::CloseButton:
				winOwner->CloseManagedWindow(this->node);
				break;
			}
		}
		//Refresh();
	}

	void SubTitlebar::OnMouseLeftDoubleClickEvent(wxMouseEvent& evt)
	{
		wxPoint mousePos = evt.GetPosition();
		Feature f = this->HandleMouseTooltip(mousePos);
		if(f == Feature::Text)
			this->winOwner->SetWinNodeMaximized(this->node);
	}

	void SubTitlebar::OnMouseCaptureChangedEvent(wxMouseCaptureChangedEvent& evt)
	{}

	void SubTitlebar::OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& evt)
	{}

	void SubTitlebar::OnMenu_RenameTitle(wxCommandEvent& evt)
	{
		// TODO: Register cancel
		wxString newTitle = wxGetTextFromUser("Title", "Rename the window title", this->node->customTitle, this);
		if(newTitle == this->node->customTitle)
			return;

		this->node->customTitle = newTitle;
		this->node->RefreshTitlebarAndTab();
	}

	void SubTitlebar::OnMenu_ResetTitle(wxCommandEvent& evt)
	{
		if(this->node->customTitle.IsEmpty())
			return;

		this->node->customTitle = wxString();
		this->node->RefreshTitlebarAndTab();
	}

	void SubTitlebar::OnMenu_CloseWindow(wxCommandEvent& evt)
	{
		winOwner->CloseManagedWindow(this->node);
	}

	void SubTitlebar::OnMenu_DetachWindow(wxCommandEvent& evt)
	{
		winOwner->DetachManagedWindow(this->node);
	}

	void SubTitlebar::OnMenu_ReleaseWindow(wxCommandEvent& evt)
	{
		winOwner->ReleaseManagedWindow(this->node);
	}

	const Context& SubTitlebar::GetAppContext()
	{
		return this->winOwner->GetAppContext();
	}

	void SubTitlebar::OnMenu_DebugTabClone(wxCommandEvent& evt)
	{
		this->winOwner->Dock(L"notepad", this->node, DockDir::Into, -1);
	}
}