#pragma once
#include "Defines.h"
#include <wx/wx.h>

namespace WinMux
{
	class SubTabs : public wxWindow
	{
	private:
		Node* node;
		WinInst* winOwner;

		Node* clickCandidate = nullptr;
		wxPoint originalClickedPos;
		Node* currentHovered = nullptr;

	public:
		SubTabs(WinInst* parent, Node* node);

	private:
		wxRect GetTabRect(const wxRect clientRgn, int tabId);
		static wxRect GetTabFeature(const wxRect tabRgn, TabFeature feature, const Context& ctx);
		int GetTabIndexAt(const wxPoint& pos);
		Node* GetTabNodeAt(const wxPoint& pos);

		void SetHoveredTabNode(Node* node);
		void SetClickCandidateTabNode(Node* node);

	public:
		void OnPaintEvent(wxPaintEvent& evt);
		void OnMouseEnterEvent(wxMouseEvent& evt);
		void OnMouseLeaveEvent(wxMouseEvent& evt);
		void OnMouseMotionEvent(wxMouseEvent& evt);
		void OnMouseLeftDownEvent(wxMouseEvent& evt);
		void OnMouseLeftUpEvent(wxMouseEvent& evt);
		void OnMouseRightDownEvent(wxMouseEvent& evt);
		void OnMouseRightUpEvent(wxMouseEvent& evt);
		void OnMouseCaptureChangedEvent(wxMouseCaptureChangedEvent& evt);
		void OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& evt);

	protected:
		DECLARE_EVENT_TABLE();
	};
}