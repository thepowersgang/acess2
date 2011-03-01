/*
 * Acess2 GUI Test App
 * - By John Hodge (thePowersGang)
 */
#include <axwin/axwin.h>

// === PROTOTYPES ===
 int	main(int argc, char *argv[]);
 int	Menubar_HandleMessage(tAxWin_Message *Message);

// === GLOBALS ===
tAxWin_Handle	ghMenubarWindow;

// === CODE ===
int main(int argc, char *argv[])
{
	AxWin_Register("Terminal");
	
	AxWin_MessageLoop();
	
	return 0;
}

/**
 */
int Menubar_HandleMessage(tAxWin_Message *Message)
{
	return 0;
}
