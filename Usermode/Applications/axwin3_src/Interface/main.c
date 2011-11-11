/*
 * Acess2 GUI v3 User Interface
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Interface core
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <axwin3/axwin.h>
#include <axwin3/widget.h>
#include <axwin3/menu.h>

#define SIDEBAR_WIDTH	36

// === PROTOTYPES ===
void	create_sidebar(void);
void	create_mainmenu(void);

// === GLOBALS ===
tHWND	gSidebar;
tHWND	gSystemMenu;
tAxWin3_Widget	*gSidebarRoot;
 int	giScreenWidth;
 int	giScreenHeight;

// === CODE ===
int systembutton_fire(tAxWin3_Widget *Widget)
{
	_SysDebug("SystemButton pressed");
	AxWin3_Menu_ShowAt(gSystemMenu, SIDEBAR_WIDTH, 0);
	return 0;
}

int main(int argc, char *argv[])
{
	// Connect to AxWin3 Server
	AxWin3_Connect(NULL);
	
	create_sidebar();
	create_mainmenu();
	
	// Idle loop
	AxWin3_MainLoop();
	
	return 0;
}

void create_sidebar(void)
{
	tAxWin3_Widget	*btn, *txt, *ele;

	// TODO: Register to be told when the display layout changes
	AxWin3_GetDisplayDims(0, NULL, NULL, &giScreenWidth, &giScreenHeight);

	// Create sidebar
	gSidebar = AxWin3_Widget_CreateWindow(NULL, SIDEBAR_WIDTH, giScreenHeight, ELEFLAG_VERTICAL);
	AxWin3_MoveWindow(gSidebar, 0, 0);
	gSidebarRoot = AxWin3_Widget_GetRoot(gSidebar);	

	// - Main menu
	btn = AxWin3_Widget_AddWidget(gSidebarRoot, ELETYPE_BUTTON, ELEFLAG_NOSTRETCH, "SystemButton");
	AxWin3_Widget_SetSize(btn, SIDEBAR_WIDTH);
	AxWin3_Widget_SetFireHandler(btn, systembutton_fire);
	txt = AxWin3_Widget_AddWidget(btn, ELETYPE_IMAGE, 0, "SystemLogo");
	AxWin3_Widget_SetText(txt, "file:///Acess/Apps/AxWin/3.0/AcessLogoSmall.sif");
	
	// - Plain <hr/> style spacer
	ele = AxWin3_Widget_AddWidget(gSidebarRoot, ELETYPE_SPACER, ELEFLAG_NOSTRETCH, "SideBar Spacer Top");
	AxWin3_Widget_SetSize(ele, 4);

	// TODO: Program list
	ele = AxWin3_Widget_AddWidget(gSidebarRoot, ELETYPE_BOX, ELEFLAG_VERTICAL, "ProgramList");

	// - Plain <hr/> style spacer
	ele = AxWin3_Widget_AddWidget(gSidebarRoot, ELETYPE_SPACER, ELEFLAG_NOSTRETCH, "SideBar Spacer Top");
	AxWin3_Widget_SetSize(ele, 4);

	// > Version/Time
	ele = AxWin3_Widget_AddWidget(gSidebarRoot, ELETYPE_BOX, ELEFLAG_VERTICAL|ELEFLAG_NOSTRETCH, "Version/Time");
	txt = AxWin3_Widget_AddWidget(ele, ELETYPE_TEXT, ELEFLAG_NOSTRETCH, "Version String");
	AxWin3_Widget_SetSize(txt, 20);
	AxWin3_Widget_SetText(txt, "3.0");

	// Show!
	AxWin3_ShowWindow(gSidebar, 1);	
	
}

void mainmenu_app_textedit(void *unused)
{
	_SysDebug("TODO: Launch text editor");
}

void mainmenu_run_dialog(void *unused)
{
	_SysDebug("TODO: Show run dialog");
}

void create_mainmenu(void)
{
	gSystemMenu = AxWin3_Menu_Create(NULL);
	
	AxWin3_Menu_AddItem(gSystemMenu, "Text &Editor\tWin+E", mainmenu_app_textedit, NULL, 0, NULL);
	AxWin3_Menu_AddItem(gSystemMenu, NULL, NULL, NULL, 0, NULL);
	AxWin3_Menu_AddItem(gSystemMenu, "Run\tWin+R", mainmenu_run_dialog, NULL, 0, NULL);
}

