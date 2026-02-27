#include "TaskbarIcon.h"
#include "App.h"
#include "resource.h"

namespace WinMux
{
	BEGIN_EVENT_TABLE(TaskbarIcon, wxTaskBarIcon)
		//EVT_UPDATE_UI(PU_CHECKMARK, MyTaskBarIcon::OnMenuUICheckmark)
		//EVT_TASKBAR_LEFT_DCLICK(MyTaskBarIcon::OnLeftButtonDClick)
		EVT_MENU(MENU_Exit,			TaskbarIcon::OnMenu_Exit)
		EVT_MENU(MENU_ReleaseAll,	TaskbarIcon::OnMenu_ReleaseAll)
		EVT_MENU(MENU_CloseAll,		TaskbarIcon::OnMenu_CloseAll)
		EVT_MENU(MENU_NewWinMux,	TaskbarIcon::OnMenu_NewWinMux)
	END_EVENT_TABLE()

	TaskbarIcon::TaskbarIcon(App* app) 
		: wxTaskBarIcon(),
		  app(app)
	{
		HICON hIcon = (HICON)::LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 0, 0, LR_LOADTRANSPARENT);
		this->icon.CreateFromHICON(hIcon);
		this->SetIcon(this->icon);
	}

	TaskbarIcon::~TaskbarIcon()
	{}

	wxMenu* TaskbarIcon::CreatePopupMenu()
	{
		wxMenu* menu = new wxMenu;
		menu->Append(MENU_NewWinMux,	"New Blank WinMux");
		menu->AppendSeparator();
		menu->Append(MENU_ReleaseAll,	"Release All");
		menu->Append(MENU_CloseAll,		"Close All");
		menu->AppendSeparator();
		menu->Append(MENU_Exit,			"Exit");
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

	void TaskbarIcon::OnMenu_ReleaseAll(wxCommandEvent& evt)
	{
		this->app->ReleaseAllAndCloseAll();
	}

	void TaskbarIcon::OnMenu_CloseAll(wxCommandEvent& evt)
	{
		this->app->CloseAll();
	}
}