#pragma once
#include "Defines.h"
#include <wx/wx.h>
#include <wx/taskbar.h>

namespace WinMux
{
	class TaskbarIcon : public wxTaskBarIcon
	{
	public:
		enum IDs
		{
			MENU_Exit,
			MENU_ReleaseAll,
			MENU_CloseAll,
			MENU_NewWinMux,
		};

	private:
		App* app;

	public:
		TaskbarIcon(App* app);
		~TaskbarIcon();

		wxIcon icon;

		virtual wxMenu* CreatePopupMenu() override;

	public:
		void OnMenu_Exit(wxCommandEvent& evt);
		void OnMenu_ReleaseAll(wxCommandEvent& evt);
		void OnMenu_CloseAll(wxCommandEvent& evt);
		void OnMenu_NewWinMux(wxCommandEvent& evt);

	protected:
		DECLARE_EVENT_TABLE();
	};
}

