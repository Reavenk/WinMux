#include "TaskbarIcon.h"
#include "App.h"

namespace WinMux
{
	BEGIN_EVENT_TABLE(TaskbarIcon, wxTaskBarIcon)
		//EVT_UPDATE_UI(PU_CHECKMARK, MyTaskBarIcon::OnMenuUICheckmark)
		//EVT_TASKBAR_LEFT_DCLICK(MyTaskBarIcon::OnLeftButtonDClick)
		EVT_MENU(MENU_Exit, TaskbarIcon::OnMenu_Exit)
		EVT_MENU(MENU_NewWinMux, TaskbarIcon::OnMenu_NewWinMux)
	END_EVENT_TABLE()

	TaskbarIcon::TaskbarIcon(App* app) 
		: wxTaskBarIcon(),
		  app(app)
	{
	}

	wxMenu* TaskbarIcon::CreatePopupMenu()
	{
		wxMenu* menu = new wxMenu;
		menu->Append(MENU_NewWinMux, "New Blank WinMux");
		menu->AppendSeparator();
		menu->Append(MENU_Exit,		"Exit");
		return menu;
	}

	void TaskbarIcon::OnMenu_NewWinMux(wxCommandEvent& evt)
	{
		app->SpawnInst();
	}

	void TaskbarIcon::OnMenu_Exit(wxCommandEvent& evt)
	{
		app->Exit();
	}
}