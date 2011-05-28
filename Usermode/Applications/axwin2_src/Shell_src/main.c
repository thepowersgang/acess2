/*
 * Acess2 GUI Test App
 * - By John Hodge (thePowersGang)
 */
#include <axwin2/axwin.h>

// === PROTOTYPES ===
 int	main(int argc, char *argv[]);
 int	Menubar_HandleMessage(tAxWin_Message *Message);

// === GLOBALS ===
tAxWin_Element	geConsole;

// === CODE ===
int main(int argc, char *argv[])
{
	AxWin_Register("Terminal");
	
	//geConsole = AxWin_CreateElement();
	
	AxWin_MessageLoop();
	
	return 0;
}

/**
 */
int Menubar_HandleMessage(tAxWin_Message *Message)
{
	return 0;
}
