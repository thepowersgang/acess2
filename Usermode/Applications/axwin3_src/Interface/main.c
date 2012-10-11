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

#define SIDEBAR_WIDTH	40
#define RUN_WIDTH	200
#define RUN_HEIGHT	60

// === PROTOTYPES ===
void	create_sidebar(void);
void	create_mainmenu(void);
void	create_run_dialog(void);
void	mainmenu_run_dialog(void *unused);
void	mainmenu_app_terminal(void *unused);
void	mainmenu_app_textedit(void *unused);

// === GLOBALS ===
tHWND	gSidebar;
tAxWin3_Widget	*gSidebarRoot;
tHWND	gSystemMenu;
tHWND	gRunDialog;
tAxWin3_Widget	*gRunInput;
 int	giScreenWidth;
 int	giScreenHeight;
char	**gEnvion;

// === CODE ===
int systembutton_fire(tAxWin3_Widget *Widget)
{
	_SysDebug("SystemButton pressed");
	AxWin3_Menu_ShowAt(gSystemMenu, SIDEBAR_WIDTH, 0);
	return 0;
}

int main(int argc, char *argv[], char **envp)
{
	gEnvion = envp;
	// Connect to AxWin3 Server
	AxWin3_Connect(NULL);

	// TODO: Register to be told when the display layout changes
	AxWin3_GetDisplayDims(0, NULL, NULL, &giScreenWidth, &giScreenHeight);
	
	create_sidebar();
	create_mainmenu();
	create_run_dialog();
	
	AxWin3_RegisterAction(gSidebar, "Interface>Run", (tAxWin3_HotkeyCallback)mainmenu_run_dialog);
	AxWin3_RegisterAction(gSidebar, "Interface>Terminal", (tAxWin3_HotkeyCallback)mainmenu_app_terminal);
	AxWin3_RegisterAction(gSidebar, "Interface>TextEdit", (tAxWin3_HotkeyCallback)mainmenu_app_textedit);

	// Idle loop
	AxWin3_MainLoop();
	
	return 0;
}

void create_sidebar(void)
{
	tAxWin3_Widget	*btn, *txt, *ele;

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
	ele = AxWin3_Widget_AddWidget(gSidebarRoot,
		ELETYPE_BOX,
		ELEFLAG_VERTICAL|ELEFLAG_ALIGN_CENTER|ELEFLAG_NOSTRETCH,
		"Version/Time"
		);
	txt = AxWin3_Widget_AddWidget(ele, ELETYPE_TEXT, ELEFLAG_NOSTRETCH, "Version String");
	AxWin3_Widget_SetSize(txt, 20);
	AxWin3_Widget_SetText(txt, "3.0");

	// Turn off decorations
	AxWin3_DecorateWindow(gSidebar, 0);

	// Show!
	AxWin3_ShowWindow(gSidebar, 1);	
	
}

void mainmenu_app_textedit(void *unused)
{
	const char	*args[] = {"ate",NULL};
//	_SysDebug("TODO: Launch text editor");
	_SysSpawn("/Acess/Apps/AxWin/3.0/ate", args, (const char **)gEnvion, 0, NULL);
}

void mainmenu_app_terminal(void *unused)
{
	_SysDebug("TODO: Launch terminal emulator");
}

void mainmenu_run_dialog(void *unused)
{
	AxWin3_ShowWindow(gRunDialog, 1);
	AxWin3_FocusWindow(gRunDialog);
}

void create_mainmenu(void)
{
	gSystemMenu = AxWin3_Menu_Create(NULL);
	
	AxWin3_Menu_AddItem(gSystemMenu, "Text &Editor\tWin+E", mainmenu_app_textedit, NULL, 0, NULL);
	AxWin3_Menu_AddItem(gSystemMenu, "&Terminal Emulator\tWin+T", mainmenu_app_terminal, NULL, 0, NULL);
	AxWin3_Menu_AddItem(gSystemMenu, NULL, NULL, NULL, 0, NULL);
	AxWin3_Menu_AddItem(gSystemMenu, "&Run\tWin+R", mainmenu_run_dialog, NULL, 0, NULL);
}

// --------------------------------------------------------------------
// "Run" Dialog box
// --------------------------------------------------------------------
int run_dorun(tAxWin3_Widget *unused)
{
	_SysDebug("DoRun pressed");
	char *cmd = AxWin3_Widget_GetText(gRunInput);
	_SysDebug("Command string '%s'", cmd);
	AxWin3_ShowWindow(gRunDialog, 0);
	return 0;
}

int run_close(tAxWin3_Widget *unused)
{
	_SysDebug("Run diaglog closed");
	AxWin3_ShowWindow(gRunDialog, 0);
	return 0;
}

tAxWin3_Widget *make_textbutton(tAxWin3_Widget *Parent, const char *Label, tAxWin3_Widget_FireCb handler)
{
	tAxWin3_Widget	*ret, *txt;
	ret = AxWin3_Widget_AddWidget(Parent, ELETYPE_BUTTON, ELEFLAG_ALIGN_CENTER, "_btn");
	AxWin3_Widget_SetFireHandler(ret, handler);
	AxWin3_Widget_AddWidget(ret, ELETYPE_NONE, 0, "_spacer1");
	txt = AxWin3_Widget_AddWidget(ret, ELETYPE_TEXT, ELEFLAG_NOSTRETCH|ELEFLAG_NOEXPAND, "_txt");
	AxWin3_Widget_SetText(txt, Label);
	AxWin3_Widget_AddWidget(ret, ELETYPE_NONE, 0, "_spacer2");
	return ret;
}

void create_run_dialog(void)
{
	tAxWin3_Widget	*root, *box;
	
	gRunDialog = AxWin3_Widget_CreateWindow(NULL, RUN_WIDTH, RUN_HEIGHT, ELEFLAG_VERTICAL);
	AxWin3_SetWindowTitle(gRunDialog, "Run Program...");
	AxWin3_MoveWindow(gRunDialog, giScreenWidth/2-RUN_WIDTH/2, giScreenHeight/2-RUN_HEIGHT/2);

	root = AxWin3_Widget_GetRoot(gRunDialog);

	gRunInput = AxWin3_Widget_AddWidget(root, ELETYPE_TEXTINPUT, ELEFLAG_NOSTRETCH, "Input");
	AxWin3_Widget_SetFireHandler(gRunInput, run_dorun);
	
	box = AxWin3_Widget_AddWidget(root, ELETYPE_BOX, ELEFLAG_ALIGN_CENTER, "Button Area");
	make_textbutton(box, "Ok", run_dorun);
	make_textbutton(box, "Cancel", run_close);
}

