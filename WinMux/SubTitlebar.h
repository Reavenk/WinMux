#pragma once

#include "Defines.h"
#include <wx/wx.h>

namespace WinMux
{
	class SubTitlebar : public wxPanel
	{
	private:
		enum class Feature
		{
			Null = -1,
			Text = 0,
			SysIcon,
			PulldownButton,
			MinMaxButton,
			CloseButton,
			Totalnum
		};
		enum IDs
		{
			MENU_CloseWindow,
			MENU_DetachWindow,
			MENU_ReleaseWindow,
			MENU_DebugTabClone
		};

	private:
		WinInst* winOwner;
		Node* node;

		/// <summary>
		///		When left click dragging, what feature was originally clicked.
		/// </summary>
		Feature currentClickedDown;

		/// <summary>
		///		What feature is the mouse currently hovering over.
		/// </summary>
		Feature currentHoverFeature;

		/// <summary>
		///		When left clicked down, the original spot that was clicked.
		/// </summary>
		wxPoint originalDownPos;

		bool isHoveredOverBar = false;

	public:
		SubTitlebar(WinInst* parent, Node* node);

	public:
		void OnPaintEvent(wxPaintEvent& event);
		void OnMouseEnterEvent(wxMouseEvent& evt);
		void OnMouseLeaveEvent(wxMouseEvent& evt);
		void OnMouseMotionEvent(wxMouseEvent& evt);
		void OnMouseLeftDownEvent(wxMouseEvent& evt);
		void OnMouseLeftUpEvent(wxMouseEvent& evt);
		void OnMouseLeftDoubleClickEvent(wxMouseEvent& evt);
		void OnMouseCaptureChangedEvent(wxMouseCaptureChangedEvent& evt);
		void OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& evt);

		void OnMenu_CloseWindow(wxCommandEvent& evt);
		void OnMenu_DetachWindow(wxCommandEvent& evt);
		void OnMenu_ReleaseWindow(wxCommandEvent& evt);
		void OnMenu_DebugTabClone(wxCommandEvent& evt);

	private:
		const Context& GetAppContext();

		wxRect GetFeatureRgn(const wxRect& entireRgn, Feature f, const Context& ctx);
		wxRect GetFeatureRgn(const wxRect& entireRgn, Feature f);
		Feature GetFeature(const wxPoint& pt);

		Feature HandleMouseTooltip(wxPoint pos);

	protected:
		DECLARE_EVENT_TABLE()
	};
}