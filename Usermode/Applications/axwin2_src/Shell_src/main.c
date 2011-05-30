/*
 * Acess2 GUI Test App
 * - By John Hodge (thePowersGang)
 */
#include <axwin2/axwin.h>

// === CONSTANTS ===
enum eTerminal_Events
{
	EVENT_NULL,
	EVENT_NEW_TAB,
	EVENT_CLOSE_TAB,
	EVENT_EXIT
};

// === PROTOTYPES ===
 int	main(int argc, char *argv[]);
 int	Global_HandleMessage(tAxWin_Message *Message);
 int	Shell_HandleMessage(tAxWin_Message *Message);

// === GLOBALS ===
tAxWin_Element	*geConsole;

// === CODE ===
int main(int argc, char *argv[])
{
	tAxWin_Element	*menu, *tab;
	AxWin_Register("Terminal", Global_HandleMessage);

	menu = AxWin_AddMenuItem(NULL, "File", 0);
	AxWin_AddMenuItem(menu, "&New Tab\tCtrl-Shift-N", EVENT_NEW_TAB);
	AxWin_AddMenuItem(menu, NULL, 0);
	AxWin_AddMenuItem(menu, "&Close Tab\tCtrl-Shift-W", EVENT_CLOSE_TAB);
	AxWin_AddMenuItem(menu, "E&xit\tAlt-F4", EVENT_EXIT);

	tab = AxWin_CreateTab("root@acess: ~");
	//geConsole = AxWin_CreateElement();
	
	AxWin_MessageLoop();
	
	return 0;
}

/**
 */
int Global_HandleMessage(tAxWin_Message *Message)
{
	return 0;
}

int Shell_HandleMessage(tAxWin_Message *Message)
{
	return 0;
}
